#!/usr/bin/env python3
"""Static integrity check for the world maps in ../world.cpp.

Verifies what the firmware trusts blindly:
  * 16 rooms of exactly 12x12, legal charset, walled corners;
  * border openings only at row/col 5, and both sides of every shared
    edge agree (open<->open, door<->door, wall<->wall) — world-border
    edges are fully walled;
  * doors only on the edges named in DOOR_EDGES, and enough keys exist;
  * exactly one '@'; expected item/boss counts;
  * flood fill from '@' (doors passable) reaches every item, enemy and
    the boss; with doors CLOSED the dungeon must be sealed and both keys
    plus the sword still reachable (no soft lock);
  * warns about isolated 1-px walls (charlieplex ghosting worst case).

Exit code 0 = world is sound.
"""
import re
import sys
from pathlib import Path

W = H = 12
GRID = 4
ROOMS = GRID * GRID
DOOR_EDGES = {3, 7}  # vertical edges, named by the room on the north side
EXPECT = {'K': 2, 'H': 3, 'S': 1, 'T': 1, 'B': 1, '@': 1}
CHARSET = set('#.DKHSTEB@')

src = (Path(__file__).parent.parent / 'world.cpp').read_text()

rooms = []
for m in re.finditer(r'R(\d\d)\[\]\s+PROGMEM\s*=([^;]+);', src):
    idx = int(m.group(1))
    body = ''.join(re.findall(r'"([^"]*)"', m.group(2)))
    rooms.append((idx, body))
rooms.sort()

errors, warnings = [], []

if [i for i, _ in rooms] != list(range(ROOMS)):
    errors.append(f'expected rooms 0..15, found {[i for i, _ in rooms]}')

grid = {}
for idx, body in rooms:
    if len(body) != W * H:
        errors.append(f'room {idx}: {len(body)} chars, want {W*H}')
        continue
    bad = set(body) - CHARSET
    if bad:
        errors.append(f'room {idx}: illegal chars {bad}')
    grid[idx] = [list(body[y * W:(y + 1) * W]) for y in range(H)]

def cell(r, x, y):
    return grid[r][y][x]

# --- borders and edge agreement -------------------------------------
for r in grid:
    rows, cols = grid[r], list(zip(*grid[r]))
    for name, line in (('N', rows[0]), ('S', rows[H - 1]),
                       ('W', cols[0]), ('E', cols[W - 1])):
        for i, c in enumerate(line):
            if i == 5:
                if c not in '#.D':
                    errors.append(f'room {r} {name} edge pos 5: {c!r}')
            elif c != '#':
                errors.append(f'room {r} {name} edge pos {i}: {c!r} (only pos 5 may open)')

def edge_char(r, side):
    return {'N': cell(r, 5, 0), 'S': cell(r, 5, H - 1),
            'W': cell(r, 0, 5), 'E': cell(r, W - 1, 5)}[side]

for r in grid:
    row, col = divmod(r, GRID)
    for side, dr, dc, opp in (('N', -1, 0, 'S'), ('W', 0, -1, 'E')):
        nr, nc = row + dr, col + dc
        me = edge_char(r, side)
        if not (0 <= nr < GRID and 0 <= nc < GRID):
            if me != '#':
                errors.append(f'room {r} {side} edge opens off the world')
            continue
        other = edge_char(nr * GRID + nc, opp)
        if me != other:
            errors.append(f'edge mismatch rooms {nr*GRID+nc}<->{r}: {other!r} vs {me!r}')

# --- doors ----------------------------------------------------------
doors_found = set()
for r in grid:
    for y in range(H):
        for x in range(W):
            if cell(r, x, y) == 'D':
                if y == 0:
                    doors_found.add(r - GRID)
                elif y == H - 1:
                    doors_found.add(r)
                else:
                    errors.append(f'room {r}: D at ({x},{y}) not on a N/S edge')
                if x != 5:
                    errors.append(f'room {r}: D at ({x},{y}) not at pos 5')
if doors_found != DOOR_EDGES:
    errors.append(f'door edges {doors_found} != DOOR_EDGES {DOOR_EDGES}')

# --- counts ---------------------------------------------------------
counts = {}
for _, body in rooms:
    for c in body:
        counts[c] = counts.get(c, 0) + 1
for c, n in EXPECT.items():
    if counts.get(c, 0) != n:
        errors.append(f'expected {n} of {c!r}, found {counts.get(c, 0)}')
if counts.get('K', 0) < len(DOOR_EDGES):
    errors.append('fewer keys than doors')

# --- reachability ---------------------------------------------------
def flood(doors_open):
    passable = '.DKHSTEB@' if doors_open else '.KHSTEB@'
    start = next((r, x, y) for r in grid for y in range(H) for x in range(W)
                 if cell(r, x, y) == '@')
    seen, stack = {start}, [start]
    while stack:
        r, x, y = stack.pop()
        row, col = divmod(r, GRID)
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny, nr = x + dx, y + dy, r
            if nx < 0 and col > 0: nr, nx = r - 1, W - 1
            elif nx >= W and col < GRID - 1: nr, nx = r + 1, 0
            elif ny < 0 and row > 0: nr, ny = r - GRID, H - 1
            elif ny >= H and row < GRID - 1: nr, ny = r + GRID, 0
            if not (0 <= nx < W and 0 <= ny < H):
                continue
            if cell(nr, nx, ny) not in passable:
                continue
            if (nr, nx, ny) not in seen:
                seen.add((nr, nx, ny))
                stack.append((nr, nx, ny))
    return seen

reach_open = flood(True)
for r in grid:
    for y in range(H):
        for x in range(W):
            if cell(r, x, y) in 'KHSTEB' and (r, x, y) not in reach_open:
                errors.append(f'unreachable {cell(r,x,y)!r} in room {r} at ({x},{y})')

reach_closed = flood(False)
for r, x, y, what in [(r, x, y, cell(r, x, y)) for r in grid
                      for y in range(H) for x in range(W)
                      if cell(r, x, y) in 'KS']:
    if (r, x, y) not in reach_closed:
        errors.append(f'soft lock: {what!r} in room {r} needs a door to reach')
for r in (3, 7):  # the dungeon must actually be locked
    if any((r, x, y) in reach_closed for y in range(H) for x in range(W)):
        errors.append(f'dungeon room {r} reachable without opening a door')

# --- charlieplex hygiene (warn only) --------------------------------
for r in grid:
    for y in range(1, H - 1):
        for x in range(1, W - 1):
            if cell(r, x, y) == '#' and all(
                    cell(r, x + dx, y + dy) != '#'
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                warnings.append(f'room {r}: lone wall pixel at ({x},{y})')

for w in warnings:
    print('WARN ', w)
for e in errors:
    print('ERROR', e)
print(f'{len(rooms)} rooms, {counts.get("E",0)} enemy spawns, '
      f'{len(reach_open)} cells reachable — '
      + ('WORLD OK' if not errors else f'{len(errors)} ERRORS'))
sys.exit(1 if errors else 0)
