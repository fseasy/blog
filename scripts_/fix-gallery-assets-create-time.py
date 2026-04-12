"""
Prompt:
  写一个 Python 脚本：
  功能：修复照片、视频的创建时间戳，同时将时间戳设置为文件的 create, update time
  逻辑：
    给定一个文件夹或者文件；如果是文件夹，枚举所有内部的的照片、视频资源
    对每个资源：从 exif 或者 metadata 或者 文件名中读取创建时间
    先从文件名去解析时间：支持解析的文件名的范式有如下：
    IMG_20250130_185031-32生日蛋糕  20250130_185031 数字这块就是时间戳 2. VID_20250816_112632-风吹麦浪 同理 3. video_20250813_150029-wapta-手机都进水了 同理 4. MVIMG_20250315_182817-baking卷卷 同理 5. MEITU_20250505_225834-by-好好 同理，但是多了一位毫秒 6. mmexport1762036651534-seal-seal 这里 mmexport 后面的 1762036651534 UTC timestamp.
    还有写文件名不是这个范式的，就不能解析出时间
    如何解析不出来，就从 exif/metadata 解析出时间
    如果时间很可疑：年份是 now 的年份
    就跳过这个文件，并把文件路径（相对 root 路径）记录到 $output/fix_failed_list.txt 中
    额外的点：文件名里的时间如果是本地时间串，就尝试从媒体里读取 GPS 信息获取时区，从而得到 UTC 时间；如果时间戳带时区，就解析时区
    最后的时间，用 UTC 表示
    将解析出的时间，写回到文件的 exif/metadata 以及文件的 create, update time 里，保证他们全部同步
    写回操作是 inplace 的！
  输入参数：1. 要处理的源文件夹或者文件
  代码风格：
    合理封装：把时间获取、时间修正的逻辑封装到类里; 和其他文件处理等逻辑分开
    python 3.12 现代语法和 typing, pathlib
    英文注释，仅含关键部分即可

Dependency:
- PYTHON:
  uv add timezonefinder tzdata tqdm
- System:
  exiftool: https://exiftool.org/ (the most powerful tool, without python pkg replacement)
"""

import json
import logging
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
from zoneinfo import ZoneInfo
from tqdm import tqdm

try:
  from timezonefinder import TimezoneFinder
except ImportError:
  TimezoneFinder = None
  logging.warning("timezonefinder not installed. GPS timezone resolution will be skipped.")

# Valid media extensions to process
VIDEO_LOWER_EXTS = {".mp4", ".mov", ".avi", ".webm"}
IMG_LOWER_EXTS = {".jpg", ".jpeg", ".png", ".webp", ".heic"}
SUPPORTED_EXTS = VIDEO_LOWER_EXTS | IMG_LOWER_EXTS


class MediaTimeExtractor:
  """Handles extraction of time and GPS data from filenames and media metadata."""

  def __init__(self):
    # Regex for patterns like: IMG_20250130_185031, MEITU_20250505_2258341
    # Captures Group 1: YYYYMMDD, Group 2: HHMMSS
    self.pattern_datetime = re.compile(r"(?:IMG|VID|video|MVIMG|MEITU)_(\d{8})_(\d{6})\d*")

    # Regex for mmexport UTC timestamps
    self.pattern_unix = re.compile(r"mmexport(\d{13})")

    self.tf = TimezoneFinder() if TimezoneFinder else None

  def _run_exiftool_json(self, filepath: Path, extra_args: list[str]) -> dict[str, Any]:
    """Runs exiftool and returns the parsed JSON output."""
    cmd = ["exiftool", "-j", "-G"] + extra_args + [str(filepath)]
    try:
      result = subprocess.run(cmd, capture_output=True, text=True, check=True)
      data = json.loads(result.stdout)
      return data[0] if data else {}
    except (subprocess.CalledProcessError, json.JSONDecodeError):
      return {}

  def extract_from_filename(self, filepath: Path) -> datetime | None:
    """Tries to parse the timestamp from the filename."""
    name = filepath.name

    # 1. Try standard pattern (Local time, needs timezone resolution later)
    if match := self.pattern_datetime.search(name):
      date_str, time_str = match.groups()
      try:
        # Returns a naive datetime (assuming local time initially)
        return datetime.strptime(f"{date_str}{time_str}", "%Y%m%d%H%M%S")
      except ValueError:
        pass

    # 2. Try mmexport pattern (UTC Unix timestamp in milliseconds)
    if match := self.pattern_unix.search(name):
      ts_ms = int(match.group(1))
      return datetime.fromtimestamp(ts_ms / 1000.0, tz=timezone.utc)

    return None

  def get_gps_coordinates(self, filepath: Path) -> tuple[float, float] | None:
    """Extracts GPS coordinates as decimal degrees."""
    # -c "%+.6f" forces decimal degree formatting
    data = self._run_exiftool_json(filepath, ["-c", "%+.6f", "-GPSLatitude", "-GPSLongitude"])
    try:
      lat = float(str(data.get("EXIF:GPSLatitude", "")).strip("+"))
      lon = float(str(data.get("EXIF:GPSLongitude", "")).strip("+"))
      return lat, lon
    except ValueError:
      return None

  def extract_from_metadata(self, filepath: Path) -> datetime | None:
    """Extracts the oldest available original timestamp from EXIF/Metadata."""
    data = self._run_exiftool_json(filepath, ["-DateTimeOriginal", "-CreateDate"])

    # ExifTool date format: YYYY:MM:DD HH:MM:SS
    date_str = (
      data.get("EXIF:DateTimeOriginal")
      or data.get("EXIF:CreateDate")
      or data.get("QuickTime:CreateDate")
      or data.get("Item:DateTimeOriginal")
    )

    if date_str and isinstance(date_str, str):
      try:
        # Parse naive time from metadata
        # Note: Some metadata contains timezone (e.g. +08:00), we handle base string first
        base_time = date_str[:19]
        return datetime.strptime(base_time, "%Y:%m:%d %H:%M:%S")
      except ValueError:
        pass
    return None

  def resolve_to_utc(self, dt: datetime, filepath: Path) -> datetime:
    """Converts a naive datetime to UTC by checking GPS for timezone, or using system local."""
    if dt.tzinfo is not None:
      return dt.astimezone(timezone.utc)

    # Attempt GPS Timezone Resolution
    if self.tf:
      gps = self.get_gps_coordinates(filepath)
      if gps:
        lat, lon = gps
        tz_name = self.tf.timezone_at(lat=lat, lng=lon)
        if tz_name:
          tz = ZoneInfo(tz_name)
          return dt.replace(tzinfo=tz).astimezone(timezone.utc)

    # Fallback: Assume it's system local time and convert to UTC
    return dt.astimezone().astimezone(timezone.utc)


class MediaTimeFixer:
  """Handles the in-place modification of file metadata and OS timestamps."""

  @staticmethod
  def apply_time(filepath: Path, dt_utc: datetime) -> bool:
    """
    Applies the UTC time to EXIF, QuickTime metadata, and OS Create/Update times.
    Exiftool will handle the in-place modification (-overwrite_original).
    """
    # Format for exiftool: YYYY:MM:DD HH:MM:SSZ
    time_str = dt_utc.strftime("%Y:%m:%d %H:%M:%SZ")

    cmd = [
      "exiftool",
      "-overwrite_original",
      "-m",  # Ignore minor errors
      f"-AllDates={time_str}",  # Updates standard EXIF/IPTC/XMP dates
      f"-FileCreateDate={time_str}",  # OS Creation time
      f"-FileModifyDate={time_str}",  # OS Modification time
      str(filepath),
    ]

    try:
      subprocess.run(cmd, capture_output=True, check=True)
      return True
    except subprocess.CalledProcessError as e:
      logging.error(f"ExifTool failed on {filepath.name}: {e.stderr.decode('utf-8', errors='ignore')}")
      return False


class MediaProcessor:
  """Orchestrates the discovery and processing of media files."""

  def __init__(self, target_path: str):
    self.target_path = Path(target_path).resolve()
    self.extractor = MediaTimeExtractor()
    self.fixer = MediaTimeFixer()
    self.current_year = datetime.now(timezone.utc).year

    # Internal list to hold failure/suspicious records
    self.suspicious_records: list[str] = []

    self.base_dir = self.target_path if self.target_path.is_dir() else self.target_path.parent
    self.suspicious_list_path = self.base_dir / "suspicious_files.txt"

  def process_file(self, filepath: Path):
    """Core logic to process a single file."""
    if filepath.suffix.lower() not in SUPPORTED_EXTS:
      return

    # 1. Try Filename extraction
    dt = self.extractor.extract_from_filename(filepath)
    from_filename = dt is not None

    # 2. Fallback to Metadata extraction
    if not dt:
      dt = self.extractor.extract_from_metadata(filepath)

    # 3. Handle total failure
    if not dt:
      self.log_failure(filepath, "Could not parse time from filename or metadata")
      return

    # 4. Resolve to UTC
    dt_utc = self.extractor.resolve_to_utc(dt, filepath)

    # 5. Check for suspicious year
    if self.is_suspicious(dt_utc):
      self.log_failure(filepath, f"Suspicious time detected (Year {self.current_year})")
      return

    # 6. Apply the fix
    success = self.fixer.apply_time(filepath, dt_utc)
    if not success:
      self.log_failure(filepath, "ExifTool failed to write metadata/OS time")
      return

    # 7. Rename file with appropriate time prefix for uniformity
    self._rename_file_with_time_prefix(filepath, dt_utc)

  def run_auto(self):
    """Mode 1: Automatically enumerate and process all files."""
    if self.target_path.is_file():
      self.process_file(self.target_path)
    elif self.target_path.is_dir():
      pbar = tqdm(unit="files", desc="Auto Fixing")
      for filepath in self.target_path.rglob("*"):
        if filepath.is_file():
          self.process_file(filepath)
          pbar.update(1)
      pbar.close()
    else:
      logging.error(f"Invalid path: {self.target_path}")

    self._flush_suspicious_log()

  def run_manual(self, map_file_path: str):
    """Mode 2: Manually fix files based on a mapping text file."""
    map_path = Path(map_file_path)
    if not map_path.exists():
      logging.error(f"Map file not found: {map_file_path}")
      return

    with open(map_path, "r", encoding="utf-8") as f:
      lines = f.readlines()

    pbar = tqdm(total=len(lines), unit="files", desc="Manual Fixing")
    for line in lines:
      line = line.strip()
      if not line or "|" not in line:
        pbar.update(1)
        continue

      parts = line.split("|", 1)
      old_rel = parts[0].strip()
      new_rel = parts[1].strip()

      old_abs = self.base_dir / old_rel
      new_abs = self.base_dir / new_rel

      target_to_process = None

      if old_abs.exists():
        # Make sure parent directory exists before renaming
        new_abs.parent.mkdir(parents=True, exist_ok=True)
        try:
          old_abs.rename(new_abs)
          target_to_process = new_abs
        except OSError as e:
          self.log_failure(old_abs, f"Failed to rename: {e}")
      elif new_abs.exists():
        # Might have been renamed in a previous run
        target_to_process = new_abs
      else:
        self.log_failure(old_abs, "File not found for renaming (Original & Target missing)")

      if target_to_process:
        self.process_file(target_to_process)

      pbar.update(1)

    pbar.close()
    self._flush_suspicious_log()

  def is_suspicious(self, dt: datetime) -> bool:
    """Checks if the parsed year is suspiciously exactly the current year."""
    return dt.year == self.current_year

  def log_failure(self, filepath: Path, reason: str):
    """Accumulates failed or skipped relative paths in memory."""
    try:
      rel_path = filepath.relative_to(self.base_dir)
    except ValueError:
      rel_path = filepath  # fallback if not relative to base_dir
    self.suspicious_records.append(f"{rel_path} | {reason}")

  def _flush_suspicious_log(self):
    """Writes accumulated logs to file and prints summary."""
    count = len(self.suspicious_records)
    if count > 0:
      with open(self.suspicious_list_path, "w", encoding="utf-8") as f:
        for record in self.suspicious_records:
          f.write(f"{record}\n")
      print(f"\n[!] Finished. Found {count} suspicious or failed files.")
      print(f"[!] Please check details in: {self.suspicious_list_path}")
    else:
      print("\n[+] All files processed successfully. No suspicious files found.")
      # Clean up old suspicious log if everything is perfect now
      if self.suspicious_list_path.exists():
        self.suspicious_list_path.unlink()

  def _rename_file_with_time_prefix(self, filepath: Path, dt_utc: datetime):
    """Renames the file with a time-based prefix based on file type, replacing existing timestamp if present."""
    suffix = filepath.suffix.lower()
    if suffix in VIDEO_LOWER_EXTS:
      prefix = "VID_"
    elif suffix in IMG_LOWER_EXTS:
      prefix = "IMG_"
    else:
      return  # Should not happen as we check SUPPORTED_EXTS earlier

    name = filepath.name
    new_time_str = dt_utc.strftime("%Y%m%d_%H%M%S")
    normed_prefix = f"{prefix}{new_time_str}"

    # Check if filename already has a timestamp pattern to replace
    if match := self.extractor.pattern_datetime.search(name):
      # Replace the existing timestamp with the new one
      new_name = name.replace(match.group(0), normed_prefix)
    elif match := self.extractor.pattern_unix.search(name):
      # For mmexport, replace with standard prefix
      new_name = name.replace(match.group(0), normed_prefix)
    else:
      # No existing timestamp, add prefix
      new_name = f"{normed_prefix}-{name}"

    if new_name == name:
      return
    new_filepath = filepath.with_name(new_name)
    try:
      filepath.rename(new_filepath)
    except OSError as e:
      self.log_failure(filepath, f"Failed to rename file with time prefix: {e}")


if __name__ == "__main__":
  import argparse

  parser = argparse.ArgumentParser(description="Fix Media Timestamps (EXIF and OS).")
  subparsers = parser.add_subparsers(dest="command", required=True, help="Processing modes")

  # --- AUTO MODE PARSER ---
  parser_auto = subparsers.add_parser("auto", help="Automatically scan and fix all files in a directory.")
  parser_auto.add_argument("--path", "-p", type=str, required=True, help="Target media file or directory path")

  # --- MANUAL MODE PARSER ---
  MANUAL_HELP_TEXT = """
Manually fix files based on a mapping text file.

Map File Format Requirements:
----------------------------------
1. One file mapping per line.
2. Separated by a pipe character '|'
3. Format: <old_relative_path> | <new_relative_path>

Example map file (suspicious_files.txt):
daycare/挖沙.mp4 | daycare/VID_20250816_112632-挖沙.mp4
fall/彩虹.jpg    | fall/IMG_20241010_100000-彩虹.jpg
  """
  parser_manual = subparsers.add_parser(
    "manual",
    help="Rename and fix files based on a manual created name-mapping file.",
    description=MANUAL_HELP_TEXT,
    formatter_class=argparse.RawTextHelpFormatter,
  )
  parser_manual.add_argument("--path", "-p", type=str, required=True, help="Base path to the target media directory")
  parser_manual.add_argument("--map-file", "-m", type=str, required=True, help="Path to the manual mapping txt file")

  args = parser.parse_args()

  processor = MediaProcessor(args.path)

  if args.command == "auto":
    print("\n>>> Starting AUTO fix mode...")
    processor.run_auto()
  elif args.command == "manual":
    print("\n>>> Starting MANUAL fix mode...")
    processor.run_manual(args.map_file)
