#!/bin/sh
# Copy the shared KeyChainino 144 engine into every sketch that uses it.
#
#   ./sync.sh          copy, and list the files that changed
#   ./sync.sh --check  only compare; exit 1 if any sketch is out of date
#
# Arduino only compiles files that sit in the sketch folder itself, so the
# engine is not an Arduino library: this folder is the one place to edit,
# and every sketch carries its own copy. input.* stays per-sketch (each
# game tunes its own button grammar).

cd "$(dirname "$0")" || exit 1

FILES="board.h display.h display.cpp gfx.h gfx.cpp power.h power.cpp"

# sketches next to this folder (must exist)
SKETCHES="
../KeyChainino_v144_multigame
../KeyChainino_v144_pictures
../KeyChainino_v144_quest
../KeyChainino_v144_snake
../KeyChainino_v144_bubble
"

stale=0
for s in $SKETCHES; do
  if [ ! -d "$s" ]; then
    echo "missing sketch: $s" >&2
    stale=1
    continue
  fi
  for f in $FILES; do
    if ! cmp -s "$f" "$s/$f"; then
      stale=1
      if [ "$1" = "--check" ]; then
        echo "out of date: $s/$f"
      else
        cp "$f" "$s/$f" && echo "updated: $s/$f"
      fi
    fi
  done
done

if [ "$1" = "--check" ]; then
  [ $stale -eq 0 ] && echo "all sketches match the library"
  exit $stale
fi
exit 0
