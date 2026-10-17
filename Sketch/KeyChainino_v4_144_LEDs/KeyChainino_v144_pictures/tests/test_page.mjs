/*
 * test_page.mjs — the web page's core (the <script id="core"> block of
 * upload/keychainino-pictures.html) against a simulated USBasp +
 * ATmega328P, then the chip it produced booted on the AVR simulator:
 * page -> programmer -> flash/EEPROM -> firmware -> LEDs, end to end.
 *
 *   node tests/test_page.mjs        (run tools/build.mjs first)
 *
 * The chip model follows firmware/upload/V144/tests/test_sequence.mjs
 * (SCK vs clock, fuse latching, usbasp page logic), plus EEPROM access.
 */
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import assert from 'node:assert/strict';
import { KeyChaininoBoard, MATRIX } from '../../../../../simulator/src/core.mjs';

const HERE = path.dirname(new URL(import.meta.url).pathname);
const PAGE = path.resolve(HERE, '../upload/keychainino-pictures.html');
const html = fs.readFileSync(PAGE, 'utf8');
const src = html.match(/<script id="core">([\s\S]*?)<\/script>/)[1];

/* ── simulated chip + programmer ────────────────────────────────────── */
const SCK_HZ = { 0: 375000, 5: 8000, 7: 32000, 8: 93750, 9: 187500, 10: 375000, 11: 750000 };
const PAGE_SZ = 128;

class Fake328P {
  constructor(o = {}) {
    this.flash = new Uint8Array(32768).fill(0xFF);
    this.eeprom = new Uint8Array(1024).fill(0xFF);
    this.lfuse = o.lfuse ?? 0x62; this.hfuse = o.hfuse ?? 0xD9; this.efuse = o.efuse ?? 0xFF;
    this.sig = [0x1E, 0x95, 0x0F];
    this.dead = !!o.dead;
    this.noReadEeprom = !!o.noReadEeprom;
    this.dropWrites = o.dropWrites ?? 0;   // swallow the first N EEPROM writes (flaky contact)
    this.sck = SCK_HZ[0];
    this.prog = false;
    this.page = new Uint8Array(PAGE_SZ).fill(0xFF);
    this.addr = 0; this.counter = 0; this.pageSize = PAGE_SZ;
    this.trace = []; this.eeWrites = [];
  }
  get fcpu() { return (this.lfuse & 0x80) ? 8e6 : 1e6; }
  enterProg() {
    if (this.dead || this.sck > this.fcpu / 4) { this.prog = false; return false; }
    this.prog = true; this.trace.push(`enterProg @${this.sck}`); return true;
  }
  flushPage() { const base = (this.addr - 1) & ~(PAGE_SZ - 1); this.flash.set(this.page, base); this.page.fill(0xFF); }
  spi(a, b, c, d) {
    if (!this.prog) return [0, 0, 0, 0];
    const ea = ((b << 8) | c) & 1023;
    if (a === 0x30) return [0, 0, 0, this.sig[c & 3]];
    if (a === 0x50 && b === 0x00) return [0, 0, 0, this.lfuse];
    if (a === 0x58 && b === 0x08) return [0, 0, 0, this.hfuse];
    if (a === 0x50 && b === 0x08) return [0, 0, 0, this.efuse];
    if (a === 0xF0) return [0, 0, 0, 0];
    if (a === 0xAC && b === 0x80) {
      this.flash.fill(0xFF);
      if (this.hfuse & 0x08) this.eeprom.fill(0xFF);  // EESAVE unprogrammed
      this.trace.push('chipErase'); return [0, 0, 0, 0];
    }
    if (a === 0xAC && b === 0xA0) { this.lfuse = d; this.trace.push('lfuse'); return [0, 0, 0, 0]; }
    if (a === 0xAC && b === 0xA8) { this.hfuse = d; return [0, 0, 0, 0]; }
    if (a === 0xAC && b === 0xA4) { this.efuse = d; return [0, 0, 0, 0]; }
    if (a === 0xA0) return [0, 0, 0, this.eeprom[ea]];
    if (a === 0xC0) {
      this.eeWrites.push(ea);
      if (this.dropWrites > 0) { this.dropWrites--; return [0, 0, 0, 0]; }
      this.eeprom[ea] = d; return [0, 0, 0, 0];
    }
    throw new Error(`unhandled ISP ${[a, b, c, d].map(v => v.toString(16))}`);
  }
}

function makeDevice(chip) {
  const ok = bytes => { const b = Buffer.from(bytes); return { status: 'ok', data: new DataView(b.buffer, b.byteOffset, b.byteLength) }; };
  return {
    vendorId: 0x16C0, productId: 0x05DC, productName: 'USBasp (simulated)', configuration: {},
    open: async () => {}, selectConfiguration: async () => {}, claimInterface: async () => {},
    controlTransferIn: async (s, len) => {
      const cmd = [s.value & 0xFF, s.value >> 8, s.index & 0xFF, s.index >> 8];
      switch (s.request) {
        case 10: chip.sck = SCK_HZ[cmd[0]]; return ok([0]);
        case 1: return ok([]);
        case 2: chip.prog = false; chip.trace.push('release'); return ok([]);
        case 5: return ok([chip.enterProg() ? 0 : 1]);
        case 3: return ok(chip.spi(...cmd));
        case 4: chip.readFlashBytes = (chip.readFlashBytes ?? 0) + len; return ok(chip.flash.subarray(s.value, s.value + len));
        case 7:
          chip.readEepromBytes = (chip.readEepromBytes ?? 0) + len;
          if (chip.noReadEeprom) throw new Error('stall');
          return ok(chip.prog ? chip.eeprom.subarray(s.value, s.value + len) : new Uint8Array(len));
        default: throw new Error(`unknown IN function ${s.request}`);
      }
    },
    controlTransferOut: async (s, data) => {
      if (s.request !== 6) throw new Error(`unknown OUT function ${s.request}`);
      const flags = (s.index >> 8) & 0x0F;
      chip.pageSize = (s.index & 0xFF) + (((s.index >> 8) & 0xF0) << 4);
      chip.addr = s.value;
      if (flags & 1) chip.counter = chip.pageSize;
      for (const byte of data) {
        chip.page[chip.addr & (PAGE_SZ - 1)] = byte; chip.addr++; chip.counter--;
        if (chip.counter === 0) { chip.flushPage(); chip.counter = chip.pageSize; }
      }
      if ((flags & 2) && chip.counter !== chip.pageSize) chip.flushPage();
      return { status: 'ok', bytesWritten: data.length };
    },
  };
}

async function page(chip) {
  const lines = [];
  const ctx = vm.createContext({
    performance, console, Date, Math, JSON, Uint8Array, Array,
    setTimeout: fn => setImmediate(fn),     // ISP delays: no need to really wait
    atob: s => Buffer.from(s, 'base64').toString('binary'),
  });
  vm.runInContext(src + `
    hooks.log = (m, c) => globalThis.__log(m, c);
    globalThis.T = { attach, updateAlbum, installFirmware, inspect, albumWith, albumWithout,
                     albumWithoutDrawing, albumWithBuiltin, albumEdited, composeEeprom, parseAlbum,
                     FIRMWARE, firmwareImage, buildIdSpan, ALBUM, LIBRARY, ON_DEVICE_AT_START };`, ctx);
  ctx.__log = (m, c) => lines.push(`${c ?? ''} ${m}`);
  await ctx.T.attach(makeDevice(chip));
  return { T: ctx.T, lines };
}

/* ── boot the chip's flash + EEPROM on the AVR simulator ────────────── */
function boot(chip, ms = 4500) {
  const words = new Uint16Array(16384);
  for (let i = 0; i < words.length; i++) words[i] = chip.flash[2 * i] | (chip.flash[2 * i + 1] << 8);
  const board = new KeyChaininoBoard(words, chip.eeprom);
  board.runMillis(ms);           // rise + its hello animation
  board.takeFrame();
  board.runMillis(300);
  const d = board.takeFrame().duty;
  return Array.from({ length: 12 }, (_, y) => {
    let w = 0;
    for (let x = 0; x < 12; x++) if (d[x * MATRIX + y] * 13 > 0.5) w |= 1 << x;
    return w;
  });
}
const art = rows => rows.map(w => [...Array(12)].map((_, x) => ((w >> x) & 1 ? '#' : '.')).join('')).join('\n');

/* ── drawings ───────────────────────────────────────────────────────── */
const box = [0xFFF, 0x801, 0x801, 0x9F9, 0x909, 0x909, 0x909, 0x909, 0x9F9, 0x801, 0x801, 0xFFF];
const diag = Array.from({ length: 12 }, (_, y) => 1 << y);
const plus = Array.from({ length: 12 }, (_, y) => (y === 5 || y === 6 ? 0xFFF : 0x060));
const nth = n => Array.from({ length: 12 }, (_, y) => ((n * 37 + y * 11) * 2654435761) >>> 20 & 0xFFF);

let pass = 0;
const check = async (name, fn) => { await fn(); console.log(`  ok  ${name}`); pass++; };

console.log('\nVirgin KeyChainino (factory chip): install, then three sends');
{
  const chip = new Fake328P();
  const { T, lines } = await page(chip);
  assert.ok(T.FIRMWARE, 'firmware not embedded — run tools/build.mjs');
  let r = await T.updateAlbum(a => T.albumWith(a, box));
  await check('a send asks for the Pictures program first', () => {
    assert.equal(r.needInstall, true); assert.equal(r.olderPictures, false);
    assert.ok(chip.flash.every(b => b === 0xFF), 'nothing may be written before the grown-up says yes');
  });
  await T.installFirmware();
  const { image, used } = T.firmwareImage();
  await check('install: flash matches the image, fuses 0xE2/0xD7/0xFF', () => {
    assert.deepEqual(Buffer.from(chip.flash.subarray(0, used)), Buffer.from(image.subarray(0, used)));
    assert.deepEqual([chip.lfuse, chip.hfuse, chip.efuse], [0xE2, 0xD7, 0xFF]);
  });
  await check('install: an album is started in EEPROM, no drawings, the default pictures chosen', () => {
    assert.deepEqual([...chip.eeprom.subarray(0, 6)], [0x4B, 0x43, 0x50, 2, 0, 0]);
    assert.deepEqual([...T.parseAlbum(chip.eeprom).builtins], [...T.ON_DEVICE_AT_START].sort((a, b) => a - b));
  });

  r = await T.updateAlbum(a => T.albumWith(a, box));
  await check('first drawing lands in slot 0, selected', () => {
    assert.equal(r.album.drawings.length, 1);
    assert.deepEqual([...r.album.drawings[0]], box);
    assert.equal(chip.eeprom[5], 0);
  });
  await check('the KeyChainino boots showing that drawing', () => {
    const shown = boot(chip);
    assert.deepEqual(shown, box, '\n' + art(shown));
  });

  chip.eeWrites = [];
  r = await T.updateAlbum(a => T.albumWith(a, diag));
  await check('second drawing: only its bytes and the count are written', () => {
    assert.ok(r.written <= 26, `wrote ${r.written} bytes`);
    const head = chip.eeWrites.filter(a => a < 0x10), body = chip.eeWrites.filter(a => a >= 0x10);
    assert.ok(Math.min(...head) > 0 && chip.eeWrites.indexOf(head[0]) > chip.eeWrites.indexOf(body.at(-1)),
      'the header must be written after the picture');
  });
  await check('...and it is the one shown (newest first)', () => assert.deepEqual(boot(chip), diag));

  r = await T.updateAlbum(a => T.albumWith(a, box));
  await check('sending a drawing that is already there re-selects it, no new slot', () => {
    assert.equal(r.album.drawings.length, 2); assert.equal(r.info.existed, true); assert.equal(chip.eeprom[5], 1);
    assert.deepEqual(boot(chip), box);
  });

  r = await T.updateAlbum(a => T.albumWithout(a, 0));
  await check('deleting the newest drawing leaves the other', () => {
    assert.equal(r.album.drawings.length, 1); assert.deepEqual([...r.album.drawings[0]], box);
  });
  r = await T.updateAlbum(a => T.albumWith(a, diag));
  r = await T.updateAlbum(a => T.albumWith(a, plus));
  r = await T.updateAlbum(a => T.albumWithoutDrawing(a, diag));
  await check('removing a drawing by its picture takes out exactly that one', () => {
    assert.deepEqual(Array.from(r.album.drawings, d => [...d]), [box, plus]);
    assert.equal(r.info.missing, false);
    assert.equal(chip.eeprom[4], 2);
    assert.deepEqual(boot(chip), plus);   // the KeyChainino restarts on the newest
  });
  r = await T.updateAlbum(a => T.albumWithoutDrawing(a, diag));
  await check('removing one that is already gone (stale list) changes nothing', () => {
    assert.equal(r.info.missing, true); assert.equal(r.written, 0);
    assert.deepEqual(Array.from(r.album.drawings, d => [...d]), [box, plus]);
  });
  await check('the log never reports an error', () => assert.ok(!lines.some(l => l.startsWith('e ')), lines.join('\n')));
}

console.log('\nA KeyChainino running the multigame');
{
  const chip = new Fake328P({ lfuse: 0xE2, hfuse: 0xD7 });
  chip.flash.fill(0x0C, 0, 24000);                 // some other program
  chip.eeprom.set([0x53, 0x03, 0x2C, 0x01], 0);   // its high scores
  const { T } = await page(chip);
  let r = await T.updateAlbum(a => T.albumWith(a, plus));
  await check('needs the Pictures program, no "just send" offered', () => {
    assert.equal(r.needInstall, true); assert.equal(r.olderPictures, false);
  });
  await T.installFirmware();
  r = await T.updateAlbum(a => T.albumWith(a, plus));
  await check('after install the old save data is replaced by the album', () => {
    assert.deepEqual([...chip.eeprom.subarray(0, 5)], [0x4B, 0x43, 0x50, 2, 1]);
    assert.deepEqual(boot(chip), plus);
  });
}

console.log('\nAn older Pictures program with drawings already on it');
{
  const chip = new Fake328P({ lfuse: 0xE2, hfuse: 0xD7 });
  const { T } = await page(chip);
  await T.installFirmware();
  await T.updateAlbum(a => T.albumWith(a, box));
  chip.flash[T.buildIdSpan()[0] + 12] ^= 0xFF;                          // "a different build"
  const before = Buffer.from(chip.eeprom);
  let r = await T.updateAlbum(a => T.albumWith(a, diag));
  await check('a send says "update needed" and writes nothing', () => {
    assert.equal(r.needInstall, true); assert.equal(r.olderPictures, true);
    assert.ok(before.equals(Buffer.from(chip.eeprom)));
  });
  await T.installFirmware();
  r = await T.updateAlbum(a => T.albumWith(a, diag));
  await check('after the update the drawings are still there and the send goes through', () => {
    assert.equal(chip.eeprom[4], 2);
    assert.deepEqual(boot(chip), diag);
  });
}

console.log('\nAn album from the previous layout (version 1, no choice)');
{
  const chip = new Fake328P({ lfuse: 0xE2, hfuse: 0xD7 });
  const { T } = await page(chip);
  await T.installFirmware();
  chip.eeprom.fill(0xFF);
  chip.eeprom.set([0x4B, 0x43, 0x50, 1, 1, 0], 0);           // v1: one drawing
  box.forEach((w, y) => { chip.eeprom[0x10 + y * 2] = w & 0xFF; chip.eeprom[0x11 + y * 2] = w >> 8; });
  const r = await T.updateAlbum(a => T.albumWith(a, plus));
  await check('a send upgrades it to version 2: drawing kept, default pictures chosen', () => {
    const a = T.parseAlbum(chip.eeprom);
    assert.equal(chip.eeprom[3], 2);
    assert.deepEqual(Array.from(a.drawings, d => [...d]), [box, plus]);
    assert.deepEqual([...a.builtins], [...T.ON_DEVICE_AT_START].sort((x, y) => x - y));
    assert.deepEqual(boot(chip), plus);
  });
}

console.log('\nChoosing the built-in pictures');
{
  const chip = new Fake328P({ lfuse: 0xE2, hfuse: 0xD7 });
  const { T } = await page(chip);
  await T.installFirmware();
  const uid = id => T.LIBRARY.find(st => st.id === id).uid;
  const art = id => [...T.LIBRARY.find(st => st.id === id).frames[0]];
  await T.updateAlbum(a => T.albumWith(a, box));
  let r = await T.updateAlbum(a => T.albumWithBuiltin(a, uid('lion')));
  await check('a picture sent unchanged goes on as a built-in, not as a drawing', () => {
    assert.equal(r.album.drawings.length, 1);
    assert.ok(r.album.builtins.includes(uid('lion')));
    assert.equal(r.info.existed, false);
  });
  await check('...and the KeyChainino restarts on it (it is selected)', () => assert.deepEqual(boot(chip), art('lion')));
  r = await T.updateAlbum(a => T.albumWithBuiltin(a, uid('lion')));
  await check('sending it again just selects it', () => { assert.equal(r.info.existed, true); assert.equal(r.written, 0); });

  r = await T.updateAlbum(a => T.albumEdited(a, { removeDrawings: [box], builtins: [uid('tiger'), uid('robot')] }));
  await check('the editor saves exactly the choice: drawing gone, only tiger and robot', () => {
    assert.equal(r.album.drawings.length, 0);
    assert.deepEqual([...r.album.builtins].sort((a, b) => a - b), [uid('tiger'), uid('robot')].sort((a, b) => a - b));
  });
  await check('...and the KeyChainino shows only those (library order: tiger first)', () => assert.deepEqual(boot(chip), art('tiger')));
}

console.log('\nRemoving drawings from a KeyChainino with an older Pictures build');
{
  const chip = new Fake328P({ lfuse: 0xE2, hfuse: 0xD7 });
  const { T } = await page(chip);
  await T.installFirmware();
  for (const d of [box, diag, plus]) await T.updateAlbum(a => T.albumWith(a, d));
  chip.flash[T.buildIdSpan()[0] + 12] ^= 0xFF;                          // the page now carries a newer build
  const before = Buffer.from(chip.eeprom);
  let r = await T.updateAlbum(a => T.albumWithoutDrawing(a, diag));
  await check('an older build is not touched: the answer is "update needed"', () => {
    assert.equal(r.needInstall, true); assert.equal(r.olderPictures, true);
    assert.ok(before.equals(Buffer.from(chip.eeprom)), 'nothing may be written');
  });
  await T.installFirmware();
  r = await T.updateAlbum(a => T.albumWithoutDrawing(a, diag));
  await check('after updating to the latest version the drawing is removed, the others kept', () => {
    assert.deepEqual(Array.from(T.parseAlbum(chip.eeprom).drawings, d => [...d]), [box, plus]);
  });
}

console.log('\nA send is quick: little to read, one handshake');
{
  const chip = new Fake328P({ lfuse: 0xE2, hfuse: 0xD7 });
  const { T, lines } = await page(chip);
  await T.installFirmware();
  for (const d of [box, diag]) await T.updateAlbum(a => T.albumWith(a, d));
  chip.readFlashBytes = chip.readEepromBytes = 0;
  const handshakes = chip.trace.filter(x => x.startsWith('enterProg')).length;
  const r = await T.updateAlbum(a => T.albumWith(a, plus));
  await check('the program check reads its 26-byte build id, not kilobytes of flash', () =>
    assert.ok(chip.readFlashBytes <= 32, `${chip.readFlashBytes} flash bytes read`));
  await check('only the album bytes in use are read, and only the written ones verified', () =>
    assert.ok(chip.readEepromBytes <= 16 + 3 * 24 + 32 + 32, `${chip.readEepromBytes} EEPROM bytes read`));
  await check('a KeyChainino already known to run at 8 MHz needs a single handshake', () =>
    assert.equal(chip.trace.filter(x => x.startsWith('enterProg')).length - handshakes, 1));
  await check('the log says where the time went', () => assert.ok(lines.some(l => l.includes('timing: connect'))));
  await check('...and the result is right', () => assert.deepEqual(boot(chip), plus));

  /* someone swaps in a factory-fresh chip: the fast try fails, the slow way works */
  chip.lfuse = 0x62; chip.hfuse = 0xD9;
  const r2 = await T.updateAlbum(a => T.albumWith(a, box));
  await check('a factory-fresh chip swapped in still answers (slow handshake fallback)', () => {
    assert.equal(r2.needInstall, true); assert.equal(r2.fusesOk, false);
  });
  void r;
}

console.log('\nA full album');
{
  const chip = new Fake328P({ lfuse: 0xE2, hfuse: 0xD7 });
  const { T } = await page(chip);
  await T.installFirmware();
  let r;
  for (let i = 0; i < 40; i++) r = await T.updateAlbum(a => T.albumWith(a, nth(i)));
  await check('40 drawings fit', () => assert.equal(r.album.drawings.length, 40));
  r = await T.updateAlbum(a => T.albumWith(a, plus));
  await check('the 41st pushes out the oldest', () => {
    assert.equal(r.album.drawings.length, 40); assert.equal(r.info.dropped, 1);
    assert.deepEqual([...r.album.drawings[0]], nth(1)); assert.deepEqual([...r.album.drawings[39]], plus);
    assert.deepEqual(boot(chip), plus);
  });
}

console.log('\nRough edges');
{
  const chip = new Fake328P({ lfuse: 0xE2, hfuse: 0xD7, noReadEeprom: true });
  const { T, lines } = await page(chip);
  await T.installFirmware();
  const r = await T.updateAlbum(a => T.albumWith(a, box));
  await check('a programmer without block EEPROM reads falls back to byte reads', () => {
    assert.equal(r.album.drawings.length, 1);
    assert.ok(lines.some(l => l.includes('byte by byte')));
  });
}
{
  const chip = new Fake328P({ lfuse: 0xE2, hfuse: 0xD7 });
  const { T, lines } = await page(chip);
  await T.installFirmware();
  chip.dropWrites = 3;
  const r = await T.updateAlbum(a => T.albumWith(a, diag));
  await check('bytes lost on a wobbly cable are caught by the read-back and rewritten', () => {
    assert.deepEqual([...r.album.drawings[0]], diag);
    assert.deepEqual([...T.parseAlbum(chip.eeprom).drawings[0]], diag);
    assert.ok(lines.some(l => l.includes('rewriting slowly')));
  });
}
{
  const chip = new Fake328P({ dead: true });
  const { T } = await page(chip);
  await check('no KeyChainino on the cable: a clear error', async () => {
    await assert.rejects(T.updateAlbum(a => T.albumWith(a, box)), /does not answer/);
    assert.equal(chip.trace.at(-1), 'release');
  });
}

console.log(`\n${pass} checks passed\n`);
