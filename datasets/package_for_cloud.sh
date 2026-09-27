#!/usr/bin/env bash
# Package the physical YOLO dataset for upload to Kaggle / Colab / Drive.
# Uses Python's zipfile (no system `zip` binary required). DEFLATED so the
# text labels shrink; JPEGs are already compressed.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

python3 - "$ROOT" <<'PY'
import os, sys, zipfile
root = sys.argv[1]
src  = os.path.join(root, "datasetpig")
out  = os.path.join(root, "TIRpigear-23189-yolo.zip")
assert os.path.isdir(src), "datasetpig/ missing"
if os.path.exists(out):
    os.remove(out)
n = 0
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=1, allowZip64=True) as z:
    for dp, _, fns in os.walk(src):
        for fn in fns:
            fp = os.path.join(dp, fn)
            z.write(fp, os.path.relpath(fp, root))
            n += 1
print(f"files={n} bytes={os.path.getsize(out)} size={os.path.getsize(out)/1e9:.2f} GB")
PY

ls -lh "$ROOT/TIRpigear-23189-yolo.zip"
echo "next: upload to Kaggle (kaggle datasets create -p .) or Drive/Colab -- see CLOUD_TRAINING.md"
