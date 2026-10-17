/*
 * test_sim.mjs — runs the real compiled firmware on the simulator's AVR
 * core and checks what the matrix shows. The simulator lives next to the
 * KeyChainino repo in the workspace (simulator/src/core.mjs, with its
 * node_modules installed).
 *
 *   node tests/test_sim.mjs [hex] [--film out.png]
 *
 * --film also writes a filmstrip of a slide, the wake rise and a few
 * effects, for eyeballing the motion.
 */
import fs from 'node:fs';
import path from 'node:path';
import { parseIntelHex, KeyChaininoBoard, MATRIX } from '../../../../../simulator/src/core.mjs';
import { PICTURES, ON_DEVICE_AT_START } from '../tools/pictures.mjs';
import { Image } from '../tools/png.mjs';

const HERE = path.dirname(new URL(import.meta.url).pathname);
const HEX = process.argv[2]?.endsWith('.hex') ? process.argv[2]
  : path.resolve(HERE, '../../../../../build/pictures/KeyChainino_v144_pictures.ino.hex');
const filmOut = process.argv.includes('--film') ? process.argv[process.argv.indexOf('--film') + 1] : null;
const { words } = parseIntelHex(fs.readFileSync(HEX, 'utf8'));

let failures = 0;
const check = (name, cond, extra = '') => {
  console.log(`${cond ? 'PASS' : 'FAIL'}  ${name}${extra ? '  [' + extra + ']' : ''}`);
  if (!cond) failures++;
};

/* ── helpers ─────────────────────────────────────────────────────────── */

/* per-LED level from duty: 0 off, 1 dim, 3 bright (x13: one line of 13) */
const levels = duty => Array.from(duty, d => (d * 13 > 0.5 ? 3 : d * 13 > 0.05 ? 1 : 0));
const ascii = lv => Array.from({ length: 12 }, (_, y) =>
  Array.from({ length: 12 }, (_, x) => ' +.#'[lv[x * MATRIX + y]]).join('')).join('\n');
const fromArt = rows => {
  const lv = new Array(144).fill(0);
  rows.forEach((r, y) => [...r].forEach((c, x) => { if (c === '#') lv[x * MATRIX + y] = 3; }));
  return lv;
};
const fromWords = ws => {
  const lv = new Array(144).fill(0);
  ws.forEach((w, y) => { for (let x = 0; x < 12; x++) if ((w >> x) & 1) lv[x * MATRIX + y] = 3; });
  return lv;
};
const same = (a, b) => a.every((v, i) => v === b[i]);
const dark = lv => lv.every(v => v === 0);
const pic = id => fromArt(PICTURES.find(p => p.id === id).frames[0]);

function look(board, ms = 60) {
  board.takeFrame();
  board.runMillis(ms);
  return levels(board.takeFrame().duty);
}
function tap(board, which, ms = 80) {
  board.setButton(which, true); board.runMillis(ms);
  board.setButton(which, false); board.runMillis(40);
}
function chord(board, ms = 120) {
  board.setButton('A', true); board.setButton('B', true); board.runMillis(ms);
  board.setButton('A', false); board.setButton('B', false); board.runMillis(40);
}
/* run until the screen holds the same still picture for `quiet` ms */
function settle(board, quiet = 700, limit = 15000) {
  let prev = null, stableFor = 0;
  for (let t = 0; t < limit; t += 50) {
    const lv = look(board, 50);
    if (prev && same(prev, lv)) { stableFor += 50; if (stableFor >= quiet) return lv; } else stableFor = 0;
    prev = lv;
  }
  return prev;
}
const UIDS = JSON.parse(fs.readFileSync(path.resolve(HERE, '../tools/uids.json'), 'utf8'));
/* what a fresh KeyChainino shows: the default built-ins, in library order */
const L = PICTURES.filter(p => ON_DEVICE_AT_START.includes(p.id)).map(p => p.id);
const choiceOf = ids => {
  const b = new Uint8Array(32);
  for (const id of ids) b[UIDS[id] >> 3] |= 1 << (UIDS[id] & 7);
  return b;
};

/* ── 1. first boot, blank EEPROM ─────────────────────────────────────── */
console.log('\nFirst boot (erased EEPROM)');
{
  const board = new KeyChaininoBoard(words);
  /* first boot formats the album first (~40 EEPROM bytes, ~130 ms), then
     the picture rises: catch the first lit frame */
  let early = look(board, 20);
  for (let t = 0; t < 600 && dark(early); t += 20) early = look(board, 20);
  check('wake rise: picture enters from below, not yet home', !same(early, pic('smile')) && !dark(early));
  const lv = settle(board);
  check('settles on the first built-in (smile)', same(lv, pic('smile')), '\n' + ascii(lv));
  const ee = board.eepromBackend.memory;
  check('EEPROM formatted as an empty album, layout version 2', ee[0] === 0x4B && ee[1] === 0x43 && ee[2] === 0x50 && ee[3] === 2 && ee[4] === 0,
    [...ee.subarray(0, 6)].join(','));
  check(`and it chose the default built-ins (${L.join(', ')})`,
    Buffer.from(ee.subarray(0x3E0, 0x400)).equals(Buffer.from(choiceOf(ON_DEVICE_AT_START))));

  tap(board, 'B');
  check(`B -> next picture (${L[1]})`, same(settle(board), pic(L[1])));
  tap(board, 'A');
  check('A -> back to smile', same(settle(board), pic('smile')));
  tap(board, 'A');
  check(`A wraps to the last chosen picture (${L.at(-1)}), not the whole library`, same(settle(board), pic(L.at(-1))));
  tap(board, 'B'); settle(board);
  tap(board, 'B'); settle(board);
  tap(board, 'B');
  const third = settle(board);
  check(`three B taps from the end land on the 3rd picture (${L[2]})`, same(third, pic(L[2])));

  /* the slide: mid-way both pictures share the screen */
  tap(board, 'B', 20);
  board.runMillis(10);
  const mid = look(board, 25);
  check('slide shows a moving frame, not an instant swap',
    !same(mid, pic(L[2])) && !same(mid, pic(L[3])) && !dark(mid));
  /* within 250 ms the slide is over: what shows is the new picture at home
     (its still, or the first frames of the animation it starts on arrival) */
  look(board, 250 - 55);
  const after = look(board, 30);
  const homeFrames = PICTURES.find(p => p.id === L[3]).frames.map(fromArt);
  check('slide is swift: done within 250 ms', homeFrames.some(fr => same(fr, after)), '\n' + ascii(after));
  const arrival = new Set();
  for (let i = 0; i < 30; i++) arrival.add(look(board, 50).join(''));
  check('the picture that arrives plays its animation by itself', arrival.size >= 3, `${arrival.size} distinct frames`);
  settle(board);

  /* A+B animates */
  chord(board);
  const seen = new Set();
  for (let i = 0; i < 40; i++) seen.add(look(board, 50).join(''));
  check('A+B plays an animation (several different frames)', seen.size >= 3, `${seen.size} distinct frames`);
  const back = settle(board);
  check('...and returns to the still picture', same(back, pic(L[3])));

  /* auto-sleep after 30 s */
  board.runMillis(26000);
  check('still awake at ~28 s idle', !dark(look(board, 100)));
  board.runMillis(4000);
  check('asleep (dark) after 30 s idle', dark(look(board, 300)));
  check('remembered its picture in EEPROM', board.eepromBackend.memory[5] === 3, `sel=${board.eepromBackend.memory[5]}`);

  tap(board, 'A', 300);
  check('one button alone does not wake it', dark(look(board, 300)));

  board.setButton('A', true); board.runMillis(60); board.setButton('B', true); board.runMillis(250);
  board.setButton('A', false); board.setButton('B', false);
  const woke = settle(board);
  check('A+B wakes it, showing the last picture again', same(woke, pic(L[3])), '\n' + ascii(woke));
}

/* ── 2. drawings in EEPROM ───────────────────────────────────────────── */
console.log('\nAlbum with two drawings');
const drawA = [0xFFF, 0x801, 0x801, 0x801, 0x801, 0x801, 0x801, 0x801, 0x801, 0x801, 0x801, 0xFFF]; // frame
const drawB = [0x001, 0x002, 0x004, 0x008, 0x010, 0x020, 0x040, 0x080, 0x100, 0x200, 0x400, 0x800]; // diagonal
function albumImage(drawings, selected = 0, chosen = ON_DEVICE_AT_START, version = 2) {
  const ee = new Uint8Array(1024).fill(0xFF);
  ee.set([0x4B, 0x43, 0x50, version, drawings.length, selected], 0);
  drawings.forEach((d, i) => d.forEach((w, y) => {
    ee[0x10 + i * 24 + y * 2] = w & 0xFF; ee[0x10 + i * 24 + y * 2 + 1] = w >> 8;
  }));
  if (version >= 2) ee.set(choiceOf(chosen), 0x3E0);
  return ee;
}
{
  const board = new KeyChaininoBoard(words, albumImage([drawA, drawB]));
  const first = settle(board);
  check('boots on the newest drawing', same(first, fromWords(drawB)), '\n' + ascii(first));
  tap(board, 'B');
  check('B -> the older drawing', same(settle(board), fromWords(drawA)));
  tap(board, 'B');
  check('B -> then the built-ins begin', same(settle(board), pic('smile')));
  tap(board, 'A'); settle(board);

  const effects = [];
  for (let e = 0; e < 8; e++) {
    chord(board);
    const frames = new Set();
    for (let i = 0; i < 30; i++) frames.add(look(board, 40).join(''));
    effects.push(frames.size);
    settle(board, 500);
  }
  check('each A+B on a drawing runs a visible effect', effects.every(n => n >= 3), effects.join(','));
  check('and it always comes home to the drawing', same(settle(board), fromWords(drawA)));
}
{
  const board = new KeyChaininoBoard(words, albumImage([drawA, drawB], 2 + L.length));
  check('a selection index past the end falls back to the first entry', same(settle(board), fromWords(drawB)));
}
{
  const ee = albumImage([drawA]); ee[4] = 77;  // corrupt count
  const board = new KeyChaininoBoard(words, ee);
  check('a corrupt header is reformatted, not trusted', same(settle(board), pic('smile')) && board.eepromBackend.memory[4] === 0);
}

console.log('\nChoosing the built-ins');
{
  const board = new KeyChaininoBoard(words, albumImage([], 0, ['robot', 'lion']));
  check('only the chosen built-ins, in library order: lion first', same(settle(board), pic('lion')));
  tap(board, 'B');
  check('B -> robot', same(settle(board), pic('robot')));
  tap(board, 'B');
  check('B wraps back to lion: nothing else is on it', same(settle(board), pic('lion')));
}
{
  const board = new KeyChaininoBoard(words, albumImage([drawA], 0, ['tiger']));
  settle(board);
  tap(board, 'B');
  check('drawings first, then the chosen built-in', same(settle(board), pic('tiger')));
}
{
  const ee = albumImage([], 0, ['tiger']);
  ee[0x3E0 + 31] = 0x80;                  // uid 255: no such picture in this firmware
  const board = new KeyChaininoBoard(words, ee);
  settle(board);
  tap(board, 'B');
  check('an unknown picture number is ignored', same(settle(board), pic('tiger')));
}
{
  const board = new KeyChaininoBoard(words, albumImage([], 0, []));
  check('nothing chosen and no drawings: it still shows a picture (the first one)', same(settle(board), pic(PICTURES[0].id)));
}
{
  const board = new KeyChaininoBoard(words, albumImage([drawA, drawB], 1, null, 1));   // layout v1
  check('an album from the previous version keeps its drawings', same(settle(board), fromWords(drawB)));
  const ee = board.eepromBackend.memory;
  check('...is upgraded to version 2 with the default built-ins', ee[3] === 2 && ee[4] === 2 &&
    Buffer.from(ee.subarray(0x3E0, 0x400)).equals(Buffer.from(choiceOf(ON_DEVICE_AT_START))));
  tap(board, 'B'); settle(board);
  tap(board, 'B');
  check('...and after the drawings come the defaults', same(settle(board), pic(L[0])));
}

/* ── 3. pocket protection ───────────────────────────────────────────── */
console.log('\nButton stuck down (keychain in a pocket)');
{
  const board = new KeyChaininoBoard(words);
  settle(board);
  board.setButton('A', true);
  board.runMillis(46000);
  check('auto-repeat gives up and it still falls asleep', dark(look(board, 300)));
  board.runMillis(2000);
  check('stays asleep while A remains held', dark(look(board, 300)));
  board.setButton('B', true); board.runMillis(200);
  check('...and A+B cannot wake it until A has been let go', dark(look(board, 200)));
  board.setButton('A', false); board.setButton('B', false); board.runMillis(100);
  chord(board, 200);
  check('after release, A+B wakes it normally', !dark(settle(board)));
}

/* ── film ────────────────────────────────────────────────────────────── */
if (filmOut) {
  const board = new KeyChaininoBoard(words, albumImage([drawB]));
  const strips = [];
  const grab = (n, ms) => { const fr = []; board.takeFrame(); for (let i = 0; i < n; i++) { board.runMillis(ms); fr.push(levels(board.takeFrame().duty)); } return fr; };
  strips.push(grab(14, 22));            // wake rise
  settle(board);
  tap(board, 'B', 20); strips.push(grab(12, 16));   // slide drawing -> smile
  settle(board);
  tap(board, 'A', 20); settle(board);
  for (let e = 0; e < 8; e++) { chord(board, 60); strips.push(grab(14, 80)); settle(board, 400); }
  const C = 5, W = 12 * C + 6, cols = Math.max(...strips.map(s => s.length));
  const img = new Image(cols * W + 6, strips.length * W + 6, [30, 30, 36]);
  strips.forEach((s, r) => s.forEach((lv, k) => {
    img.rect(6 + k * W - 1, 6 + r * W - 1, 12 * C + 2, 12 * C + 2, [12, 12, 14]);
    for (let x = 0; x < 12; x++) for (let y = 0; y < 12; y++) {
      const v = lv[x * 12 + y];
      img.disc(6 + k * W + x * C + C / 2, 6 + r * W + y * C + C / 2, C * 0.45,
        v === 3 ? [255, 60, 90] : v === 1 ? [110, 30, 45] : [40, 38, 40]);
    }
  }));
  fs.writeFileSync(filmOut, img.png());
  console.log(`film: ${filmOut} (rows: wake, slide, hop, jelly, sparkle, wave, flip, beat, burst, spin)`);
}

console.log(`\n${failures ? failures + ' FAILED' : 'all passed'}`);
process.exit(failures ? 1 : 0);
