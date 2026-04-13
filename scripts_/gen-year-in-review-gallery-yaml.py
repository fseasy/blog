#!/usr/bin/env python3
"""Generate gallery yaml data.
Used for _includes/extra_fn/gallery.html
- Logic:
  - preprocess src images: compress & format to .webp/.webm, generate poster img for video.
  - generate yaml data for `gallery.html`
- dependency:
  - pypi: pyyaml, pillow, timezonefinder, tzdata, tqdm, fs-pyutils
  - system:
    - ffmpeg
    - exiftool: https://exiftool.org/
"""

import argparse
import json
import concurrent.futures
import re
import subprocess
from datetime import UTC, datetime
from pathlib import Path
from typing import NotRequired, TypedDict

from PIL import Image
import yaml
from fs_pyutils.lang_basic import import_module_from_path
from tqdm import tqdm
from tqdm.contrib.concurrent import thread_map

IMAGE_EXTS = {".jpg", ".jpeg", ".png", ".webp", ".heic"}
VIDEO_EXTS = {".mp4", ".mov", ".webm", ".avi", ".mkv"}
DT_UTC_FORMAT = "%Y-%m-%d %H:%M:%S"

_module = import_module_from_path("fix_gallery_assets_create_time", "./fix-gallery-assets-create-time.py")
g_media_time_extractor = _module.MediaTimeExtractor()
g_file_time_fixer = _module.MediaTimeFixer()


class GalleryYamlItem(TypedDict):
  relative_path: str
  caption: str
  dt_utc: str
  width: int
  height: int
  poster: NotRequired[str]


GalleryData = dict[str, list[GalleryYamlItem]]


class FormatTransformTaskInput(TypedDict):
  input_abspath: Path
  output_abspath: Path
  poster_output_abspath: NotRequired[Path]
  is_video: bool
  dt_utc: datetime


class FormatTransformTaskOutput(TypedDict):
  input_abspath: Path  # used as the task key
  width: int
  height: int


def process_media(input_path: Path, output_path: Path, scale_filter: str, is_video: bool):
  """Convert and compress media using extreme CPU compression."""
  if is_video:
    cmd = [
      "ffmpeg",
      "-y",
      "-i",
      str(input_path),
      # 将降帧率直接放入滤镜链，12fps
      "-vf",
      f"{scale_filter},fps=15",
      "-c:v",
      "libvpx-vp9",
      "-crf",
      "38",  # 核心：非常高的恒定质量参数，数值越大体积越小、画质越低
      "-b:v",
      "0",  # VP9 CRF 模式必须加上 -b:v 0
      "-cpu-used",
      "2",  # 核心：0-5，越小压缩效率越高但越慢。1 是极致体积和极慢速度的甜点
      "-row-mt",
      "1",  # 核心：开启行级多线程，拯救多核 CPU 编码速度
      "-g",
      "75",  # 增大关键帧间隔 (15fps * 5s = 75)，大幅减小体积
      "-pix_fmt",
      "yuv420p",
      "-c:a",
      "libopus",  # 音频改用 Opus，低码率王者
      "-b:a",
      "64k",  # 64k Opus 音质远好于 64k aac，满足你“音频不能太差”的需求
      str(output_path),
    ]
  else:
    cmd = [
      "ffmpeg",
      "-y",
      "-i",
      str(input_path),
      "-vf",
      f"{scale_filter}",
      "-c:v",
      "libwebp",
      "-q:v",
      "75",  # 质量
      "-compression_level",
      "6",  # 核心：这就是你要的 -m 6，在 ffmpeg 中全名为 compression_level
      "-preset",
      "photo",
      "-an",
      str(output_path),
    ]
  result = subprocess.run(cmd, capture_output=True, text=True)

  if result.returncode != 0:
    print(f"\n❌ Error processing: {input_path}")
    print(f"Command: {' '.join(cmd)}")
    print(f"Stdout: {result.stdout}")
    print(f"Stderr: {result.stderr}")
    raise subprocess.CalledProcessError(result.returncode, cmd, result.stdout, result.stderr)


def generate_poster(input_video: Path, output_poster: Path, scale_filter: str):
  """Extract a frame at 0.1s as WebP poster."""
  cmd = [
    "ffmpeg",
    "-y",
    "-i",
    str(input_video),
    "-ss",
    "00:00:00.100",  # Skip 0.1s to avoid black frames
    "-vframes",
    "1",  # Extract only 1 frame
    "-vf",
    scale_filter,
    "-c:v",
    "libwebp",
    "-q:v",
    "50",
    "-compression_level",
    "6",  # 封面同样使用极慢极致压缩
    str(output_poster),
  ]
  subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)


def do_format_transform(task: FormatTransformTaskInput) -> FormatTransformTaskOutput:
  """Worker function for concurrent execution."""
  file_path = task["input_abspath"]
  out_filepath = task["output_abspath"]
  is_video = task["is_video"]
  dt_utc = task["dt_utc"]
  # 将最大分辨率降至 800x800，可大幅降低体积
  scale_filter = "scale=w='min(800,iw)':h='min(800,ih)':force_original_aspect_ratio=decrease"
  # Process main media
  if not out_filepath.exists():
    process_media(file_path, out_filepath, scale_filter, is_video)
    g_file_time_fixer.apply_time(out_filepath, dt_utc)

  # Process poster for video
  if is_video:
    poster_filepath = task.get("poster_output_abspath")
    assert poster_filepath, "MUST SET poster-output path when input is video"
    if not poster_filepath.exists():
      generate_poster(file_path, poster_filepath, scale_filter)
      g_file_time_fixer.apply_time(poster_filepath, dt_utc)

  if is_video:
    w, h = _get_video_size(out_filepath)
  else:
    w, h = _get_image_size(out_filepath)
  return {"input_abspath": file_path, "width": w, "height": h}


def main(src_dir: Path, dest_dir: Path):
  if not src_dir.exists():
    print(f"Source dir {src_dir} does not exist.")
    return

  gallery_data: GalleryData = {}

  tasks = []
  build_items_dict = {}

  print("Scanning files and extracting metadata...")
  input_abspath2item: dict[Path, GalleryYamlItem] = {}
  for subdir in tqdm(src_dir.iterdir(), unit="dirs"):
    if not subdir.is_dir():
      continue

    subdir_name = subdir.name
    out_subdir = dest_dir / subdir_name
    out_subdir.mkdir(parents=True, exist_ok=True)

    items: list[GalleryYamlItem] = []
    build_items_dict[subdir_name] = items

    for file_path in tqdm(subdir.iterdir(), unit="files", position=1, leave=False):
      ext = file_path.suffix.lower()
      is_video = ext in VIDEO_EXTS
      if not (ext in IMAGE_EXTS or is_video):
        continue
      input_abspath = file_path.resolve()
      dt_utc = g_media_time_extractor.extract(file_path)
      if not dt_utc:
        print(f"File media time is None, use file create-time. Path={file_path}")
        dt_utc = _get_file_utc_time(file_path)

      trimmed_stem = _trim_title(file_path.stem)
      caption = f"{trimmed_stem.replace('-', ' ')} \ndate: {dt_utc.strftime('%Y-%m-%d')}".strip()

      norm_stem = _normalize_filename(file_path.stem)
      out_filename = f"{norm_stem}{'.webm' if is_video else '.webp'}"
      out_filepath = (out_subdir / out_filename).resolve()

      item_data: GalleryYamlItem = {
        "relative_path": f"{subdir_name}/{out_filename}",
        "caption": caption,
        "dt_utc": dt_utc.astimezone(UTC).strftime(DT_UTC_FORMAT),
        "width": 0,
        "height": 0,
      }

      task: FormatTransformTaskInput = {
        "input_abspath": input_abspath,
        "output_abspath": out_filepath,
        "is_video": is_video,
        "dt_utc": dt_utc,
      }

      if is_video:
        poster_filename = f"{norm_stem}_poster.webp"
        poster_filepath = out_subdir / poster_filename
        task["poster_output_abspath"] = poster_filepath.resolve()
        item_data["poster"] = f"{subdir_name}/{poster_filename}"

      items.append(item_data)
      tasks.append(task)
      input_abspath2item[input_abspath] = item_data

  # Phase 2: 并发处理 (VP9属于极重度CPU任务)
  print(f"Found {len(tasks)} media files. Starting processing (CPU Extreme Compression)...")
  # 推荐使用 4-5 个 workers。因为 libvpx-vp9 配合 -row-mt 本身就是多线程运作
  # 10 核 CPU 跑 4 个重度视频编码进程正好吃满且不容易引起线程饥饿
  results = thread_map(do_format_transform, tasks, max_workers=4, desc="Processing")
  for out in results:
    input_abspath2item[out["input_abspath"]].update({"width": out["width"], "height": out["height"]})

  # Phase 3: Build YAML
  print("Generating YAML...")
  for subdir_name, items in build_items_dict.items():
    items.sort(key=lambda x: x["dt_utc"])
    gallery_data[subdir_name] = items

  yaml_path = dest_dir / "gallery_data.yml"
  with open(yaml_path, "w", encoding="utf-8") as f:
    yaml.dump(gallery_data, f, allow_unicode=True, sort_keys=False)

  print(f"\n✅ Done! Data saved to: {yaml_path}")


def _normalize_filename(stem: str) -> str:
  name = re.sub(r"\s+", "-", stem)
  name = re.sub(r"[^\w\-\.\u4e00-\u9fff]", "", name)
  return name


def _trim_title(stem: str) -> str:
  trimmed = re.sub(r"^(?:(?:IMG|DSC|VID|PANO|videoScreenshot)[_A-Z0-9]+-?)+", "", stem, flags=re.IGNORECASE)
  return trimmed if trimmed else stem


def _get_file_utc_time(file_path: str | Path) -> datetime:
  path = Path(file_path)
  stat = path.stat()

  # 获取修改时间（返回的是时间戳，例如 1712880000.0）
  mtime_timestamp = stat.st_mtime
  utc_time = datetime.fromtimestamp(mtime_timestamp, tz=UTC)

  return utc_time


def _get_video_size(path: Path | str) -> tuple[int, int]:
  cmd = [
    "ffprobe",
    "-v",
    "error",
    "-select_streams",
    "v:0",
    "-show_entries",
    "stream=width,height",
    "-of",
    "json",
    str(path),
  ]

  result = subprocess.run(
    cmd,
    capture_output=True,
    text=True,
    check=True,
  )

  data = json.loads(result.stdout)
  s = data["streams"][0]
  return s["width"], s["height"]


def _get_image_size(path: Path | str) -> tuple[int, int]:
  with Image.open(path) as img:
    return img.width, img.height


if __name__ == "__main__":
  import logging

  logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s [%(levelname)s] %(name)s (%(filename)s:%(lineno)d) - %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
  )
  logging.getLogger("PIL").setLevel(logging.WARNING)  # disable PIL DEBUG info
  parser = argparse.ArgumentParser(description="Generate Year-in-Review gallery files.")
  parser.add_argument("--input-dir", "-i", help="path to input resource dir", type=str, required=True)
  parser.add_argument("--output-dir", "-o", type=str, help="path to processed output dir", required=True)
  args = parser.parse_args()
  main(Path(args.input_dir), Path(args.output_dir))
