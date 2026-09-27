# TIRPigEar — Cloud GPU Training (Kaggle / Colab)

Dataset: `TIRPigEar` (TIR pig-ear detection). Unified YOLO root at `datasets/datasetpig/`.
- 23,189 thermal-IR images (320x240), 69,567 labels, 9 farm groups `01..09`.
- Single class: `0 = ear` (YOLO txt, normalized `cx cy w h`).
- Splits: train 18,548 / val 2,316 / test 2,325.

Repo: https://github.com/maweihong/TIRPigEar (best published: YOLOv9m — 97.35% P, 98.1% R, 98.6% mAP50).

---

## 1. Package + upload

```bash
cd /home/joal/Projects/O.I.N.K./datasets
bash package_for_cloud.sh          # -> TIRpigear-23189-yolo.zip (~5.5 GB)
```

### Option A — Kaggle Dataset (recommended, free T4x2, 30 h/wk)
```bash
# one-time: put your token at ~/.kaggle/kaggle.json
pip install kaggle
cd /home/joal/Projects/O.I.N.K./datasets
# set "id" to <your-username>/tirpigear-yolo in dataset-metadata.json
kaggle datasets create -p . --dir-mode zip      # or: kaggle datasets version -p . -m "v1"
```

### Option B — Colab via Google Drive
Upload `TIRpigear-23189-yolo.zip` to Drive, then in the notebook:
```python
from google.colab import drive; drive.mount('/content/drive')
!unzip -q "/content/drive/MyDrive/TIRpigear-23189-yolo.zip" -d /content/data
```

---

## 2. Train (run on the GPU box)

```bash
pip install ultralytics
```

### Kaggle
`/kaggle/input/<slug>/datasetpig/` — edit `data.yaml` `path:` to that (see `data.cloud.yaml`).

### Colab
`/content/data/datasetpig/` — same edit.

### Command
```bash
# real run (mirrors the repo's best config)
yolo detect train \
  model=yolo11l.pt \
  data=<DATASET_ROOT>/data.yaml \
  epochs=200 imgsz=640 batch=32 pretrained=true \
  optimizer=SGD lr0=0.01 lrf=0.01 momentum=0.937 weight_decay=0.0005 \
  project=runs/trainpig name=exp_l2

# quick baselines / smoke tests
yolo detect train model=yolo11n.pt data=<DATASET_ROOT>/data.yaml epochs=50 imgsz=640 batch=64
```

---

## 3. Verify

```bash
yolo detect val \
  model=runs/trainpig/exp_l2/weights/best.pt \
  data=<DATASET_ROOT>/data.yaml
```

Compare mAP50 / P / R against the repo's published numbers. Sanity gate: mAP50 should be well above ~0.90 for a good run; class metrics should show `ear`.

---

## 4. Notes
- Kaggle sessions cap at ~12 h; use `resume=True` or checkpoint per epoch, or start from the last checkpoint next session.
- Keep `imgsz=640`, `batch=32` on a 16 GB T4; drop batch to 16 if OOM.
- The dataset zip is already the YOLO txt format — no conversion needed.
