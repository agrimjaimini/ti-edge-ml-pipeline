import re
import json
from pathlib import Path
from datetime import datetime

# -------- CONFIG --------
INPUT_LOG_FILE = "sample_swt.log"  # <-- your log file
PEOPLE_COUNT = 0  # <-- SET THIS BEFORE EACH RUN
# ------------------------

# Create unique output filename
now = datetime.now().strftime("%Y%m%d_%H%M%S")
OUTPUT_JSON_FILE = f"{PEOPLE_COUNT}people_training_{now}.json"

with open(INPUT_LOG_FILE, "r") as f:
    log_lines = f.readlines()

frames = []
frame_id = None
timestamp = None
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
            if frame_id is not None:
                frame = add_frame(frame_id, timestamp, points)
                frames.append(frame)
            frame_id = int(match.group(1))
            points = []
            timestamp = None  # Reset timestamp for new frame

    match = re.search(r"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2},\d{3})", line)
    if match and "frame-parser" in line:
        try:
            timestamp_str = match.group(1)
            dt = datetime.strptime(timestamp_str, "%Y-%m-%d %H:%M:%S,%f")
            timestamp = int(dt.timestamp() * 1000)
        except:
            timestamp = None

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

# Add the last frame even if it had no points
if frame_id is not None:
    frame = add_frame(frame_id, timestamp, points)
    frames.append(frame)

output_path = Path(OUTPUT_JSON_FILE)
with open(output_path, "w") as f:
    json.dump(frames, f, indent=2)

print(f"Parsed {len(frames)} frames and saved to '{OUTPUT_JSON_FILE}'")