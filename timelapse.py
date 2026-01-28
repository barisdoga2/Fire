import os
import time
from datetime import datetime
import subprocess

BASE_DIR = "controller/logs/photo"
INTERVAL_SEC = 60

while True:
    now = datetime.now()
    date_dir = now.strftime("%d-%m-%y")
    time_name = now.strftime("%H-%M") + ".jpg"

    out_dir = os.path.join(BASE_DIR, date_dir)
    os.makedirs(out_dir, exist_ok=True)

    out_path = os.path.join(out_dir, time_name)

    subprocess.run([
        "fswebcam",
        "-d", "/dev/video0",
        "-r", "1280x720",
        "--no-banner",
        out_path
    ], check=False)

    time.sleep(INTERVAL_SEC)
