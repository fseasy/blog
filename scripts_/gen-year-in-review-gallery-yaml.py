#!/usr/bin/env python3
"""Generate gallery yaml data.
Used for _includes/extra_fn/gallery.html
- Logic:
  - preprocess src images: compress & format to .webp/.webm, generate poster img for video.
  - generate yaml data for `gallery.html`
- dependency:
  - pypi: pyyaml, pillow
  - ffmpeg
"""

import argparse
import json
import os
import re
import shutil
import subprocess
from datetime import UTC, datetime
from pathlib import Path
from typing import NotRequired, TypedDict

import yaml
from PIL import ExifTags, Image

IMAGE_EXTS = {".jpg", ".jpeg", ".png", ".webp", ".heic"}
VIDEO_EXTS = {".mp4", ".mov", ".webm", ".avi", ".mkv"}
DT_UTC_FORMAT = "%Y%m%d %H%M%S"


class GalleryYamlItem(TypedDict):
  relative_path: str
  caption: str
  dt_utc: str
  poster: NotRequired[str]


class GalleryBuildItem(TypedDict):
  relative_path: str
  caption: str
  dt_utc: str
  _dt_utc: datetime
  poster: NotRequired[str]


GalleryData = dict[str, list[GalleryYamlItem]]


EXIF_TAG_TO_ID = {v: k for k, v in ExifTags.TAGS.items()}
EXIF_DATETIME_TAGS = (
  "DateTimeOriginal",
  "DateTimeDigitized",
  "DateTime",
)
VIDEO_DATETIME_KEYS = (
  "creation_time",
  "com.apple.quicktime.creationdate",
  "creationdate",
)


def parse_image_exif_dt_utc(file_path: Path) -> datetime | None:
  """Extract image datetime from EXIF and normalize it to UTC when possible."""
  try:
    with Image.open(file_path) as img:
      exif = img.getexif()
      if not exif:
        return None

      offset = None
      for offset_tag_name in ("OffsetTimeOriginal", "OffsetTimeDigitized", "OffsetTime"):
        offset_tag_id = EXIF_TAG_TO_ID.get(offset_tag_name)
        if offset_tag_id:
          offset = exif.get(offset_tag_id)
          if offset:
            break

      for tag_name in EXIF_DATETIME_TAGS:
        tag_id = EXIF_TAG_TO_ID.get(tag_name)
        if not tag_id:
          continue
        dt_str = exif.get(tag_id)
        if not dt_str:
          continue

        dt = datetime.strptime(dt_str, "%Y:%m:%d %H:%M:%S")
        if offset:
          dt = datetime.fromisoformat(f"{dt.strftime('%Y-%m-%d %H:%M:%S')}{offset}")
          return dt.astimezone(UTC)
        return dt.replace(tzinfo=UTC)
  except Exception:
    return None

  return None


def parse_video_exif_dt_utc(file_path: Path) -> datetime | None:
  """Extract video datetime from ffprobe metadata and normalize it to UTC."""
  try:
    cmd = ["ffprobe", "-v", "quiet", "-print_format", "json", "-show_format", "-show_streams", str(file_path)]
    result = subprocess.run(cmd, capture_output=True, text=True, check=True)
    info = json.loads(result.stdout)
  except Exception:
    return None

  tag_sources = []
  format_tags = info.get("format", {}).get("tags", {})
  if format_tags:
    tag_sources.append(format_tags)

  for stream in info.get("streams", []):
    stream_tags = stream.get("tags", {})
    if stream_tags:
      tag_sources.append(stream_tags)

  for tags in tag_sources:
    for key in VIDEO_DATETIME_KEYS:
      dt_raw = tags.get(key)
      if not dt_raw:
        continue

      dt_normalized = dt_raw.strip().replace("Z", "+00:00")
      try:
        if dt_normalized.endswith(("+00:00", "+08:00")) or re.search(r"[+-]\d{2}:\d{2}$", dt_normalized):
          return datetime.fromisoformat(dt_normalized).astimezone(UTC)
        return datetime.fromisoformat(dt_normalized).replace(tzinfo=UTC)
      except ValueError:
        try:
          return datetime.strptime(dt_raw, "%Y-%m-%dT%H:%M:%S.%fZ").replace(tzinfo=UTC)
        except ValueError:
          try:
            return datetime.strptime(dt_raw, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=UTC)
          except ValueError:
            continue

  return None


def get_dt_utc(file_path: Path, is_video: bool) -> datetime:
  """Extract UTC datetime from metadata, fallback to file mtime."""
  dt = parse_video_exif_dt_utc(file_path) if is_video else parse_image_exif_dt_utc(file_path)
  if dt is not None:
    return dt.astimezone(UTC)
  return datetime.fromtimestamp(file_path.stat().st_mtime, tz=UTC)


def format_dt_utc(dt_utc: datetime) -> str:
  return dt_utc.astimezone(UTC).strftime(DT_UTC_FORMAT)


def set_file_timestamps(file_path: Path, dt_utc: datetime):
  """Best-effort sync output file timestamps with extracted UTC time."""
  ts = dt_utc.astimezone(UTC).timestamp()
  os.utime(file_path, (ts, ts))

  # macOS Finder exposes birth time; set it too when SetFile is available.
  setfile_cmd = shutil.which("SetFile")
  if setfile_cmd:
    dt_local = dt_utc.astimezone()
    dt_for_setfile = dt_local.strftime("%m/%d/%Y %H:%M:%S")
    subprocess.run([setfile_cmd, "-d", dt_for_setfile, "-m", dt_for_setfile, str(file_path)], check=False)


def normalize_filename(stem: str) -> str:
  """Remove spaces, keep alphanumeric, dots, hyphens, underscores and Chinese characters."""
  name = re.sub(r"\s+", "-", stem)
  name = re.sub(r"[^\w\-\.\u4e00-\u9fff]", "", name)
  return name


def trim_title(stem: str) -> str:
  """Remove prefixes like IMG_..., DSC_..."""
  return re.sub(r"^(IMG|DSC|VID|PANO|video)[_A-Z0-9]+-?", "", stem, flags=re.IGNORECASE)


def process_media(input_path: Path, output_path: Path, scale_filter: str, is_video: bool):
  """Convert and compress media."""
  if is_video:
    cmd = [
      "ffmpeg",
      "-y",
      "-i",
      str(input_path),
      "-vf",
      scale_filter,
      "-c:v",
      "libvpx-vp9",
      "-crf",
      "35",
      "-b:v",
      "0",
      "-c:a",
      "libopus",
      str(output_path),
    ]
  else:
    cmd = [
      "ffmpeg",
      "-y",
      "-i",
      str(input_path),
      "-vf",
      scale_filter,
      "-c:v",
      "libwebp",
      "-q:v",
      "80",
      str(output_path),
    ]
  subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)


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
    "80",
    str(output_poster),
  ]
  subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)


def main(src_dir: Path, dest_dir: Path):
  if not src_dir.exists():
    print(f"Source dir {src_dir} does not exist.")
    return

  # Use min() to strictly prevent upscaling while keeping aspect ratio constraint
  scale_filter = "scale=w='min(1280,iw)':h='min(720,ih)':force_original_aspect_ratio=decrease"
  gallery_data: GalleryData = {}
  latest_dt_utc: datetime | None = None

  for subdir in src_dir.iterdir():
    if not subdir.is_dir():
      continue

    subdir_name = subdir.name
    print(f"Processing folder: {subdir_name}")
    out_subdir = dest_dir / subdir_name
    out_subdir.mkdir(parents=True, exist_ok=True)
    items: list[GalleryBuildItem] = []

    for file_path in subdir.iterdir():
      ext = file_path.suffix.lower()
      is_video = ext in VIDEO_EXTS
      if not (ext in IMAGE_EXTS or is_video):
        continue

      print(f"  -> {file_path.name}")

      dt_utc = get_dt_utc(file_path, is_video)
      if latest_dt_utc is None or dt_utc > latest_dt_utc:
        latest_dt_utc = dt_utc
      trimmed_stem = trim_title(file_path.stem)
      caption = f"{trimmed_stem.replace('-', ' ')} {dt_utc.strftime('%Y%m%d %H')}".strip()

      norm_stem = normalize_filename(trimmed_stem)
      out_filename = f"{norm_stem}{'.webm' if is_video else '.webp'}"
      out_filepath = out_subdir / out_filename

      # Process main media
      if not out_filepath.exists():
        process_media(file_path, out_filepath, scale_filter, is_video)
      set_file_timestamps(out_filepath, dt_utc)

      item_data: GalleryBuildItem = {
        "relative_path": f"{subdir_name}/{out_filename}",
        "caption": caption,
        "dt_utc": format_dt_utc(dt_utc),
        "_dt_utc": dt_utc,
      }

      # Process poster for video
      if is_video:
        poster_filename = f"{norm_stem}_poster.webp"
        poster_filepath = out_subdir / poster_filename
        if not poster_filepath.exists():
          generate_poster(file_path, poster_filepath, scale_filter)
        set_file_timestamps(poster_filepath, dt_utc)
        item_data["poster"] = f"{subdir_name}/{poster_filename}"

      items.append(item_data)

    items.sort(key=lambda x: x["_dt_utc"])

    # Clean data for YAML dump
    clean_items: list[GalleryYamlItem] = []
    for item in items:
      clean_item: GalleryYamlItem = {
        "relative_path": item["relative_path"],
        "caption": item["caption"],
        "dt_utc": item["dt_utc"],
      }
      if "poster" in item:
        clean_item["poster"] = item["poster"]
      clean_items.append(clean_item)

    gallery_data[subdir_name] = clean_items

  yaml_path = dest_dir / "gallery_data.yml"
  with open(yaml_path, "w", encoding="utf-8") as f:
    yaml.dump(gallery_data, f, allow_unicode=True, sort_keys=False)
  set_file_timestamps(yaml_path, latest_dt_utc or datetime.now(UTC))

  print(f"\n✅ Done! Data saved to: {yaml_path}")


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--res-dir", type=str, required=True)
  parser.add_argument("--res-processed-dir", type=str, required=True)
  args = parser.parse_args()
  main(Path(args.res_dir), Path(args.res_processed_dir))
