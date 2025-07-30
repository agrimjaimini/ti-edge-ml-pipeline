import re
import json
from pathlib import Path
from datetime import datetime

# -------- CONFIG --------
INPUT_LOG_FILE = "/Users/saahi/Desktop/TI_MMWAVE_PROJ/ti-edge-ml-pipeline/data/logs/fall1.log"
PEOPLE_COUNT = 1

# Start and end times in HH:MM:SS,mmm format (e.g., "18:57:52,747")
START_TIME_STR = "16:36:36,101"
END_TIME_STR   = "17:01:02,879"
# ------------------------

def time_str_to_millis(time_str):
    dt = datetime.strptime(time_str, "%H:%M:%S,%f")
    return dt.hour * 3600000 + dt.minute * 60000 + dt.second * 1000 + dt.microsecond // 1000

START_MS = time_str_to_millis(START_TIME_STR)
END_MS = time_str_to_millis(END_TIME_STR)

now = datetime.now().strftime("%Y%m%d_%H%M%S")
OUTPUT_JSON_FILE = f"{PEOPLE_COUNT}people_training_{now}.json"

with open(INPUT_LOG_FILE, "r") as f:
    log_lines = f.readlines()

frames = []
frame_id = None
timestamp_ms_of_day = None
points = []
point = {}
in_major = False
in_minor = False

def add_frame(frame_id, timestamp, points):
    return {
        "Frame Count: ": frame_id,
        "Time Stamp (ms)": timestamp,
        "Num Points: ": len(points),
        "x_pos": [p["x_c"] for p in points] if points else [],
        "y_pos": [p["y_c"] for p in points] if points else [],
        "z_pos": [p["z_c"] for p in points] if points else [],
        "snr": [p["snr_c"] for p in points] if points else [],
        "noise": [p["noise_c"] for p in points] if points else [],
        "people_count": PEOPLE_COUNT
    }

for line in log_lines:
    line = line.strip()

    if "frameNum" in line:
        match = re.search(r"'frameNum': (\d+)", line)
        if match:
            if frame_id is not None and timestamp_ms_of_day is not None:
                if START_MS <= timestamp_ms_of_day <= END_MS:
                    frame = add_frame(frame_id, timestamp_ms_of_day, points)
                    frames.append(frame)
            frame_id = int(match.group(1))
            points = []
            timestamp_ms_of_day = None

    match = re.search(r"(\d{2}:\d{2}:\d{2},\d{3})", line)
    if match and "frame-parser" in line:
        try:
            time_part = match.group(1)
            timestamp_ms_of_day = time_str_to_millis(time_part)
        except:
            timestamp_ms_of_day = None

    if "majorPoints = ListContainer" in line:
        in_major = True
        in_minor = False
    elif "minorPoints = ListContainer" in line:
        in_major = False
        in_minor = True
    elif "clusterArray" in line:
        in_major = in_minor = False

    if "x_c =" in line:
        point["x_c"] = float(line.split("=")[1].strip())
    elif "y_c =" in line:
        point["y_c"] = float(line.split("=")[1].strip())
    elif "z_c =" in line:
        point["z_c"] = float(line.split("=")[1].strip())
    elif "v_c =" in line:
        point["v_c"] = float(line.split("=")[1].strip())
    elif "snr_c =" in line:
        point["snr_c"] = float(line.split("=")[1].strip())
    elif "noise_c =" in line:
        point["noise_c"] = float(line.split("=")[1].strip())
        points.append(point)
        point = {}

# Final frame
if frame_id is not None and timestamp_ms_of_day is not None:
    if START_MS <= timestamp_ms_of_day <= END_MS:
        frame = add_frame(frame_id, timestamp_ms_of_day, points)
        frames.append(frame)

output_path = Path(OUTPUT_JSON_FILE)
with open(output_path, "w") as f:
    json.dump(frames, f, indent=2)

print(f"Parsed {len(frames)} frames between {START_TIME_STR} and {END_TIME_STR} into '{OUTPUT_JSON_FILE}'")