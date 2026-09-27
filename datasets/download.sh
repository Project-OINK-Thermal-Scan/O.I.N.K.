#!/usr/bin/env bash
# Robust TIRPigEar dataset downloader.
# Uses plain HTTP (curl -L) instead of huggingface_hub's Xet client, which hangs here.
set -uo pipefail
cd "$(dirname "$0")" || exit 1

BASE="https://huggingface.co/datasets/korason154/TIRpigear-23189/resolve/main"

# name|url-encoded|expected-bytes|sha256
FILES=(
  "TIRpigear-23189(1).zip|TIRpigear-23189%281%29.zip|3727094540|37d4e6bd842bf78d7b5d9acd42ccea5412ea3120efaeff11f9b3729e3e0cffd7"
  "TIRpigear-23189(2).zip|TIRpigear-23189%282%29.zip|4744235846|8a43941f355ae68e96fdf7440492bee1fd0558d527b0c898801501c049f41b97"
)

rc=0
for spec in "${FILES[@]}"; do
  IFS='|' read -r name enc size sha <<< "$spec"
  echo "=================== $(date) : $name ==================="
  for attempt in 1 2 3 4 5; do
    curl -L --fail --retry 5 --retry-all-errors --retry-delay 5 --connect-timeout 30 \
         --continue-at - --output "$name" "$BASE/$enc"
    got=$(stat -c %s "$name" 2>/dev/null || echo 0)
    if [ "$got" = "$size" ]; then break; fi
    echo "[warn] attempt $attempt incomplete: $got / $size bytes; resuming..."
    sleep 5
  done

  got=$(stat -c %s "$name" 2>/dev/null || echo 0)
  if [ "$got" != "$size" ]; then
    echo "[FAIL] $name size mismatch: $got != $size"; rc=1; continue
  fi
  echo "[ok] size verified: $name ($got bytes)"
  echo "verifying sha256..."
  actual=$(sha256sum "$name" | awk '{print $1}')
  if [ "$actual" = "$sha" ]; then
    echo "[ok] sha256 verified: $name"
  else
    echo "[FAIL] sha256 mismatch for $name: $actual != $sha"; rc=1
  fi
done

echo "=================== DONE rc=$rc $(date) ==================="
ls -la
exit $rc
