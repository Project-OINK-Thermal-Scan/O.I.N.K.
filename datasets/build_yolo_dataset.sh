#!/usr/bin/env bash
# Build a PHYSICAL (real-file) unified YOLO dataset root for the TIRPigEar data.
# Copies images (dereferenced from the raw parts) and YOLO txt labels, prefixing
# names with the farm group id to avoid cross-group filename collisions.
# Class 0 = "ear".
set -euo pipefail

ROOT="/home/joal/Projects/O.I.N.K./datasets"
OUT="$ROOT/datasetpig"
PARTS=('TIRpigear-23189(1)' 'TIRpigear-23189(2)')

rm -rf "$OUT"
mkdir -p "$OUT/images/train" "$OUT/images/val" "$OUT/images/test" \
         "$OUT/labels/train" "$OUT/labels/val" "$OUT/labels/test"

for part in "${PARTS[@]}"; do
  for grp_path in "$ROOT/$part"/*/; do
    g=$(basename "$grp_path")
    for sd in "$grp_path"*/; do
      sname=$(basename "$sd")
      case "$sname" in
        train-*) split=train ;;
        val-*)   split=val ;;
        test-*)  split=test ;;
        *) continue ;;
      esac
      for img in "$sd"images/*.jpg; do
        [ -e "$img" ] || continue
        b=$(basename "$img" .jpg)
        cp -L -- "$img" "$OUT/images/$split/${g}__${b}.jpg"
        lbl="$sd/txt/$b.txt"
        [ -e "$lbl" ] && cp -L -- "$lbl" "$OUT/labels/$split/${g}__${b}.txt"
      done
    done
  done
done

cat > "$OUT/data.yaml" <<YAML
# TIRPigEar unified dataset (class 0 = ear) -- PHYSICAL COPY
path: $OUT
train: images/train
val: images/val
test: images/test
nc: 1
names:
  0: ear
YAML

echo "=== counts ==="
for s in train val test; do
  printf "%-6s images=%s labels=%s\n" "$s" \
    "$(find "$OUT/images/$s" -type f | wc -l)" \
    "$(find "$OUT/labels/$s" -type f | wc -l)"
done
echo "TOTAL images=$(find "$OUT/images" -type f | wc -l) labels=$(find "$OUT/labels" -type f | wc -l)"
echo "symlinks remaining (should be 0): $(find "$OUT" -type l | wc -l)"
du -sh "$OUT"
