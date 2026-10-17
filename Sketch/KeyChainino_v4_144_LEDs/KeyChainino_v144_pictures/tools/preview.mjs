/*
 * preview.mjs — contact sheet of every built-in picture and its frames,
 * drawn like lit LEDs, for eyeballing the art. One row per picture, in
 * library order (the console prints the order).
 *
 *   node tools/preview.mjs [out.png] [id,id,...]
 */
import fs from 'node:fs';
import path from 'node:path';
import { PICTURES } from './pictures.mjs';
import { Image } from './png.mjs';

const HERE = path.dirname(new URL(import.meta.url).pathname);
const out = process.argv[2] ?? path.join(HERE, 'preview.png');
const only = process.argv[3]?.split(',');
const pics = PICTURES.filter(p => !only || only.includes(p.id));

const CELL = 8, GAP = 10;
const W = 12 * CELL;
const perRow = 2;  // pictures side by side
const maxFrames = Math.max(...pics.map(p => p.frames.length));
const blockW = maxFrames * (W + GAP) + GAP * 3;
const img = new Image(perRow * blockW, Math.ceil(pics.length / perRow) * (W + GAP) + GAP, [32, 35, 42]);

pics.forEach((p, i) => {
  const bx = (i % perRow) * blockW + GAP, by = Math.floor(i / perRow) * (W + GAP) + GAP;
  p.frames.forEach((fr, k) => {
    const fx = bx + k * (W + GAP);
    img.rect(fx - 1, by - 1, W + 2, W + 2, k ? [205, 205, 200] : [250, 250, 246]);
    for (let y = 0; y < 12; y++) for (let x = 0; x < 12; x++) {
      const on = fr[y][x] === '#';
      img.disc(fx + x * CELL + CELL / 2, by + y * CELL + CELL / 2, on ? CELL * 0.45 : CELL * 0.18,
        on ? [235, 40, 70] : [190, 182, 165]);
    }
  });
});

fs.writeFileSync(out, img.png());
console.log(`wrote ${out}`);
pics.forEach((p, i) => console.log(`${String(i).padStart(2)} ${i % perRow ? 'R' : 'L'} ${p.id} — ${p.frames.length} frames, ${p.anim.length} steps`));
