from flask import Flask, Response, render_template_string, redirect, url_for, request, jsonify , session
from datetime import datetime, timedelta
import json
import serial
import threading
import time
import hashlib
import os
from functools import wraps
import cv2
import subprocess

# ---------------- CONFIG ----------------
SERIAL_PORT = "/dev/ttyUSB0"
BAUD = 9600
UI_REFRESH_SEC = 60

# ---------------- STATE ----------------
STATE_LOCK = threading.Lock()

CAMERA_INDEX = 0
CAMERA_WARMUP_FRAMES = 25

LOG_DIR = "./logs"
LOG_INTERVAL_SEC = 10

TEMP_DAY_MIN, TEMP_DAY_MAX = 26.0, 27.0
TEMP_NIGHT_MIN, TEMP_NIGHT_MAX = 19.0, 21.0

HUM_DAY_MIN, HUM_DAY_MAX = 62.5, 67.5
HUM_NIGHT_MIN, HUM_NIGHT_MAX = 65.0, 70.0

SERIAL_STALE_SEC = 10
serial_reset_requested = False

# ---------------- CAMERA QUALITY ----------------
CAM_WIDTH = 3840
CAM_HEIGHT = 2160
CAM_FPS = 30

JPEG_QUALITY_STREAM = 85
JPEG_QUALITY_SNAPSHOT = 95

CAMERA_BACKEND = cv2.CAP_V4L2

state = {
    "lastReceive": None,
    "temperature": None,
    "humidity": None,
    "ppm": None,

    # Current reported relay states (from Arduino)
    "Light": False,
    "Humidifier": False,
    "Exhaust": False,
    "Heater": False,

    # Force commands (None=AUTO, True=FORCE ON, False=FORCE OFF)
    "LightForce": None,
    "HumidifierForce": None,
    "ExhaustForce": None,
    "HeaterForce": None,
    
    
    "lightBeginHour": 6,
    "lightBeginMinute": 30,
    "lightPeriodSeconds": 64800,
    "lightScheduleUpdate": False,
}

DEVICES = ["Light", "Humidifier", "Exhaust", "Heater"]

def log(*a):
    print("[SERVER]", *a, flush=True)

def now():
    return datetime.now()

def age_seconds(ts):
    if ts is None:
        return None
    return int((now() - ts).total_seconds())

# ---------------- SERIAL WORKER ----------------
def open_serial_forever():
    """Try forever until serial opens."""
    while True:
        try:
            s = serial.Serial(SERIAL_PORT, BAUD, timeout=1)
            s.reset_input_buffer()
            time.sleep(2)  # Arduino auto-reset after port open
            log("Serial opened:", SERIAL_PORT)
            return s
        except Exception as e:
            log("Serial open failed:", e, "retrying in 2s...")
            time.sleep(2)

def serial_loop():
    s = open_serial_forever()

    while True:
        global serial_reset_requested

        if serial_reset_requested:
            log("SERIAL: Reset requested, reopening port")
            try:
                s.close()
            except Exception:
                pass
            time.sleep(1)
            s = open_serial_forever()
            with STATE_LOCK:
                state["lastReceive"] = None
            serial_reset_requested = False
            continue
        try:
            line = s.readline().decode(errors="ignore").strip()
            if not line:
                continue

            # Hard framing: ignore garbage / partial
            if not (line.startswith("{") and line.endswith("}")):
                log("RX GARBAGE:", repr(line))
                continue

            data = json.loads(line)

            # Update state from Arduino
            with STATE_LOCK:
                state["lastReceive"] = now()
                
                state["temperature"] = data.get("temperature", state["temperature"])
                state["humidity"] = data.get("humidity", state["humidity"])
                state["ppm"] = data.get("ppm", state["ppm"])
                
                if(state["lightScheduleUpdate"] == False):
                    state["lightBeginHour"] = data.get("lightBeginHour", state["lightBeginHour"])
                    state["lightBeginMinute"] = data.get("lightBeginMinute", state["lightBeginMinute"])
                    state["lightPeriodSeconds"] = data.get("lightPeriodSeconds", state["lightPeriodSeconds"])

                rel = data.get("relays", {})
                for k in DEVICES:
                    if k in rel:
                        state[k] = bool(rel[k])

                # Build reply: forces + epoch
                reply = {k: state[k + "Force"] for k in DEVICES}
                reply["epoch"] = int(time.time())
                
                if(state["lightScheduleUpdate"]):
                    state["lightScheduleUpdate"] = False
                    reply["lightBeginHour"] = state["lightBeginHour"]
                    reply["lightBeginMinute"] = state["lightBeginMinute"]
                    reply["lightPeriodSeconds"] = state["lightPeriodSeconds"]

            out = json.dumps(reply)
            s.write((out + "\n").encode())
            log("RX:", line)
            log("TX:", out)

        except Exception as e:
            log("SERIAL ERROR:", e, "reopening port...")
            try:
                s.close()
            except Exception:
                pass
            time.sleep(1)
            s = open_serial_forever()

# ---------------- FLASK ----------------
app = Flask(__name__)
app.config.update(
    SESSION_COOKIE_SECURE=True,
    SESSION_COOKIE_HTTPONLY=True,
    SESSION_COOKIE_SAMESITE="Strict"
)

app.secret_key = "0b448c5699ad3f812baaa06e15206246015c9efea6ace8bce0c73d3a3b5a0984"

USERS = {
    "admin": hashlib.sha256(b"2q9.0z8").hexdigest()
}

def login_required(f):
    @wraps(f)
    def wrapped(*args, **kwargs):
        if not session.get("logged_in"):
            return redirect(url_for("login"))
        return f(*args, **kwargs)
    return wrapped

LOGIN_HTML = """
<!doctype html>
<html>
  <head>
    <title>Climate Monitor - Login</title>
    <style>
      body {
        font-family: monospace;
        background:#111;
        color:#eee;
        margin: 20px;
      }

      h2, h3 {
        margin-top: 30px;
        margin-bottom: 10px;
      }

      table {
        border-collapse: collapse;
        width: 100%;
        max-width: 900px;
      }

      th, td {
        padding: 8px 10px;
        border-bottom: 1px solid #333;
        text-align: left;
        vertical-align: middle;
      }

      th {
        color: #aaa;
        font-weight: normal;
      }

      .green { color:#0f0; }
      .red   { color:#f33; }
      .muted { color:#888; }

      .actions {
        white-space: nowrap;
      }

      button {
        padding: 4px 10px;
        margin-right: 4px;
        background: #222;
        color: #eee;
        border: 1px solid #444;
        cursor: pointer;
      }

      button:hover {
        background: #333;
      }

      .btn-auto.active { border-color:#0f0; color:#0f0; }
      .btn-force.active { border-color:#f33; color:#f33; }

    </style>
  </head>

  <body>
    <form method="post">
      <input name="username" placeholder="Username" required><br><br>
      <input name="password" type="password" placeholder="Password" required><br><br>
      <button type="submit">Login</button>
    </form>
    {% if error %}<p style="color:red">{{ error }}</p>{% endif %}
  </body>
</html>
"""

@app.route("/login", methods=["GET", "POST"])
def login():
    error = None

    if request.method == "POST":
        username = request.form.get("username", "")
        password = request.form.get("password", "")

        password_hash = hashlib.sha256(password.encode()).hexdigest()

        if USERS.get(username) == password_hash:
            session["logged_in"] = True
            session["user"] = username
            return redirect(url_for("index"))
        else:
            error = "Invalid credentials"

    return render_template_string(LOGIN_HTML, error=error)

@app.route("/logout")
def logout():
    session.clear()
    return redirect(url_for("login"))

@app.before_request
def debug_http():
    log("HTTP", request.method, request.path)

HTML = """
<!doctype html>
<html>
  <head>
    <title>Climate Monitor</title>
    <style>
      body {
        font-family: monospace;
        background:#111;
        color:#eee;
        margin: 20px;
      }

      h2, h3 {
        margin-top: 30px;
        margin-bottom: 10px;
      }

      table {
        border-collapse: collapse;
        width: 100%;
        max-width: 900px;
      }

      th, td {
        padding: 8px 10px;
        border-bottom: 1px solid #333;
        text-align: left;
        vertical-align: middle;
      }

      th {
        color: #aaa;
        font-weight: normal;
      }

      .green { color:#0f0; }
      .red   { color:#f33; }
      .muted { color:#888; }

      .actions {
        white-space: nowrap;
      }

      button {
        padding: 4px 10px;
        margin-right: 4px;
        background: #222;
        color: #eee;
        border: 1px solid #444;
        cursor: pointer;
      }

      button:hover {
        background: #333;
      }

      .btn-auto.active { border-color:#0f0; color:#0f0; }
      .btn-force.active { border-color:#f33; color:#f33; }

    </style>
  </head>

  <body>
    <p>Logged in as: {{session['user']}}</p>
    <h2><center><a href="/">Index</a> - <a href="/logs">Logs</a> - <a href="/live">Live</a> - <a href="/snip">Snip</a> - <a href="/logout">Logout</a></center></h2>

    <br>
    <h2>Climate Control</h2>
    <div class="muted">Auto-refresh: every {{ refresh }}s</div>

    <table>
      <tr><th>Server Time</th><td id="serverTime">{{ server_time }}</td></tr>
      <tr>
        <th>Last Receive</th>
        <td id="lastReceive" class="{{ 'red' if stale else 'green' }}">
          {{ last_receive }}
          {% if last_age is not none %} ({{ last_age }}s ago){% endif %}
        </td>
      </tr>
      <tr>
        <tr>
        <th>Time</th>
        <td>
          <span class="muted">
            ({{ "DAY MODE" if is_day else "NIGHT MODE" }})
          </span>
        </td>
      </tr>
        <th>Temperature</th>
        <td id="temperatureCell" class="{{ 'red' if temp_bad else 'green' }}">
          <span id="temperature">{{ temperature }}</span> °C
          <span class="muted">
            ({{ "DAY" if is_day else "NIGHT" }},
            limits {{ "%.1f"|format(tmin) }} - {{ "%.1f"|format((tmin+tmax)/2.0) }} – {{ "%.1f"|format(tmax) }} °C)
          </span>
        </td>
      </tr>
      <tr>
        <th>Humidity</th>
        <td id="humidityCell" class="{{ 'red' if hum_bad else 'green' }}">
          <span id="humidity">{{ humidity }}</span> %
          <span class="muted">
            ({{ "DAY" if is_day else "NIGHT" }},
            limits {{ "%.1f"|format(hmin) }} - {{ "%.1f"|format((hmin+hmax)/2.0) }} – {{ "%.1f"|format(hmax) }} %)
          </span>
        </td>
      </tr>
      <tr><th>PPM</th><td>{{ ppm }}</td></tr>
    </table>

    <h3>Light Schedule</h3>
    <table>
      <tr>
        <th style="width:30%">Current Start Time</th>
        <td>
          {{ "%02d"|format(lightBeginHour) }}:{{ "%02d"|format(lightBeginMinute) }}
          <span class="muted">(actual)</span>
        </td>
      </tr>
      <tr>
        <th>Current Period</th>
        <td>{{ lightPeriodSeconds }} s</td>
      </tr>
    </table>
    <br>
    <form action="/set_light_schedule" method="get" style="display:inline">
      <table>
        <tr>
          <th style="width:30%">New Start Hour</th>
          <td>
            <input type="number" name="lightBeginHour"
                  min="0" max="23"
                  value="{{ lightBeginHour }}">
          </td>
        </tr>
        <tr>
          <th>New Start Minute</th>
          <td>
            <input type="number" name="lightBeginMinute"
                  min="0" max="59"
                  value="{{ lightBeginMinute }}">
          </td>
        </tr>
        <tr>
          <th>New Period (seconds)</th>
          <td>
            <input type="number" name="lightPeriodSeconds"
                  min="1" max="86399"
                  value="{{ lightPeriodSeconds }}">
          </td>
        </tr>
        <tr>
          <th></th>
          <td>
            <button class="btn-auto" type="submit">Apply Schedule</button>
          </td>
        </tr>
      </table>
    </form>

    <h3>Actuators</h3>
    <table>
      <tr>
        <th style="width:20%">Name</th>
        <th style="width:15%">State</th>
        <th style="width:15%">Mode</th>
        <th>Actions</th>
      </tr>
      {% for k in devices %}
      <tr>
        <td>{{ k }}</td>

        <td id="relay-{{ k }}">{{ state[k] }}</td>

        {% if state[k+'Force'] is none %}
          <td class="green">AUTO</td>
        {% else %}
          <td class="red">FORCED</td>
        {% endif %}

        <td class="actions">
          <form action="/force/{{k}}/auto" method="get" style="display:inline">
            <button class="btn-auto {% if state[k+'Force'] is none %}active{% endif %}">
              AUTO
            </button>
          </form>

          <form action="/force/{{k}}/on" method="get" style="display:inline">
            <button class="btn-force {% if state[k+'Force'] is true %}active{% endif %}">
              ON
            </button>
          </form>

          <form action="/force/{{k}}/off" method="get" style="display:inline">
            <button class="btn-force {% if state[k+'Force'] is false %}active{% endif %}">
              OFF
            </button>
          </form>
        </td>
      </tr>
      {% endfor %}
    </table>
      <script>
        async function refreshState()
        {
          try
          {
            const r = await fetch("/api/state", { cache: "no-store" });
            const s = await r.json();

            document.getElementById("serverTime").textContent = s.serverTime;

            const lr = document.getElementById("lastReceive");
            lr.textContent = s.lastReceive + " (" + s.lastAge + "s)";
            lr.className = s.stale ? "red" : "green";

            // ---- TEMP
            const tVal = (s.temperature === null || s.temperature === undefined) ? "None" : String(s.temperature);
            document.getElementById("temperature").textContent = tVal;

            const tCell = document.getElementById("temperatureCell");
            const tBad =
              (s.temperature === null || s.temperature === undefined) ||
              (s.temperature < s.tmin) ||
              (s.temperature > s.tmax);
            tCell.className = tBad ? "red" : "green";

            // ---- HUM (DAY/NIGHT AWARE)
            const hVal = (s.humidity === null || s.humidity === undefined) ? "None" : String(s.humidity);
            document.getElementById("humidity").textContent = hVal;

            const hCell = document.getElementById("humidityCell");
            const hBad =
              (s.humidity === null || s.humidity === undefined) ||
              (s.humidity < s.hmin) ||
              (s.humidity > s.hmax);
            hCell.className = hBad ? "red" : "green";

            // ---- ACTUATORS
            for (const k in s.relays)
            {
              const el = document.getElementById("relay-" + k);
              if (el)
              {
                el.textContent = s.relays[k] ? "True" : "False";
              }
            }
          }
          catch (e)
          {
            console.error("AJAX refresh failed", e);
          }
        }

        refreshState();
        setInterval(refreshState, 5000);
      </script>
  </body>
</html>
"""

def is_daytime(now_dt):
    start = now_dt.replace(
        hour=state["lightBeginHour"],
        minute=state["lightBeginMinute"],
        second=0,
        microsecond=0
    )
    end = start + timedelta(seconds=state["lightPeriodSeconds"])

    if end.day != start.day:
        return now_dt >= start or now_dt < end
    return start <= now_dt < end

def force_camera_controls():
    subprocess.run([
        "v4l2-ctl", "-d", "/dev/video0",
        "--set-ctrl=auto_exposure=1",
        "--set-ctrl=exposure_time_absolute=20",
        "--set-ctrl=gain=0",
        "--set-ctrl=brightness=75",
        "--set-ctrl=contrast=145",
        "--set-ctrl=saturation=125",
        "--set-ctrl=sharpness=110",
        "--set-ctrl=backlight_compensation=0",
        "--set-ctrl=white_balance_automatic=0",
        "--set-ctrl=white_balance_temperature=4800",
        "--set-ctrl=power_line_frequency=1",
        "--set-ctrl=focus_automatic_continuous=0"
    ], check=False)

    time.sleep(0.4)

def open_camera_with_warmup():
    force_camera_controls()
    cap = cv2.VideoCapture(CAMERA_INDEX, cv2.CAP_V4L2)
    if not cap.isOpened():
        return None

    cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*"MJPG"))
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)
    cap.set(cv2.CAP_PROP_FPS, 30)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

    for i in range(CAMERA_WARMUP_FRAMES):
        ret, _ = cap.read()
        if not ret:
            break
        time.sleep(0.02)

    return cap

def is_light_on_by_schedule(now_dt, begin_h, begin_m, period_sec):
    day_min = 24 * 60
    start = (int(begin_h) * 60 + int(begin_m)) % day_min
    nowm = (now_dt.hour * 60 + now_dt.minute) % day_min
    period_min = max(1, int(period_sec) // 60)

    delta = (nowm - start) % day_min
    return delta < period_min

def capture_snapshot(save):
    cap = open_camera_with_warmup()
    if cap is None:
        return None

    ret, frame = cap.read()
    cap.release()

    if not ret:
        return None

    if save:
        ts = now()
        day_dir = ts.strftime("%d_%m_%y")
        time_name = ts.strftime("%H_%M") + ".jpg"

        base = os.path.join(LOG_DIR, "photo", day_dir)
        os.makedirs(base, exist_ok=True)

        cv2.imwrite(os.path.join(base, time_name), frame)

    ok, jpg = cv2.imencode(
        ".jpg",
        frame,
        [cv2.IMWRITE_JPEG_QUALITY, JPEG_QUALITY_SNAPSHOT]
    )
    if not ok:
        return None

    return jpg.tobytes()

def live_stream_generator():
    cap = open_camera_with_warmup()
    if cap is None:
        return

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                break

            ok, jpg = cv2.imencode(
                ".jpg",
                frame,
                [cv2.IMWRITE_JPEG_QUALITY, JPEG_QUALITY_STREAM]
            )
            if not ok:
                continue

            yield (
                b"--frame\r\n"
                b"Content-Type: image/jpeg\r\n\r\n" +
                jpg.tobytes() +
                b"\r\n"
            )

            time.sleep(0.05)
    finally:
        cap.release()

@app.route("/live")
@login_required
def live():
    return Response(
        live_stream_generator(),
        mimetype="multipart/x-mixed-replace; boundary=frame"
    )

@app.route("/snip")
@login_required
def snip():
    img = capture_snapshot(save=False)
    if img is None:
        return "Camera error", 500
    return Response(img, mimetype="image/jpeg")

@app.route("/")
@login_required
def index():
    with STATE_LOCK:
        last = state["lastReceive"]
        last_age = age_seconds(last)
        stale = (last is None) or (now() - last > timedelta(seconds=10))

        is_day = is_light_on_by_schedule(
            now(),
            state["lightBeginHour"],
            state["lightBeginMinute"],
            state["lightPeriodSeconds"]
        )

        tmin, tmax = (TEMP_DAY_MIN, TEMP_DAY_MAX) if is_day else (TEMP_NIGHT_MIN, TEMP_NIGHT_MAX)

        temp = state["temperature"]
        hum = state["humidity"]

        temp_bad = (temp is None) or (temp < tmin) or (temp > tmax)

        hmin, hmax = (HUM_DAY_MIN, HUM_DAY_MAX) if is_day else (HUM_NIGHT_MIN, HUM_NIGHT_MAX)
        hum_bad = (hum is None) or (hum < hmin) or (hum > hmax)

        return render_template_string(
            HTML,
            refresh=UI_REFRESH_SEC,
            server_time=now().strftime("%H:%M:%S"),
            last_receive=str(last) if last else "None",
            last_age=last_age,
            stale=stale,

            temperature=temp,
            humidity=hum,
            ppm=state["ppm"],

            is_day=is_day,
            tmin=tmin,
            tmax=tmax,
            hmin=hmin,
            hmax=hmax,
            temp_bad=temp_bad,
            hum_bad=hum_bad,

            lightBeginHour=state["lightBeginHour"],
            lightBeginMinute=state["lightBeginMinute"],
            lightPeriodSeconds=state["lightPeriodSeconds"],
            state=dict(state),
            devices=DEVICES
        )

@app.route("/set_light_schedule", methods=["GET"])
def set_light_schedule():
    log("ARGS:", dict(request.args))   # 👈 BUNU EKLE

    with STATE_LOCK:
        state["lightBeginHour"] = int(request.args.get("lightBeginHour", state["lightBeginHour"]))
        state["lightBeginMinute"] = int(request.args.get("lightBeginMinute", state["lightBeginMinute"]))
        state["lightPeriodSeconds"] = int(request.args.get("lightPeriodSeconds", state["lightPeriodSeconds"]))
        state["lightScheduleUpdate"] = True
    return redirect(url_for("index"))

@app.route("/force/<dev>/<mode>", methods=["GET"])
def force(dev, mode):
    # IMPORTANT: dev comes from our HTML (Light/Humidifier/Exhaust/Heater)
    dev = dev.strip()

    with STATE_LOCK:
        key = dev + "Force"
        if key not in state:
            log("FORCE INVALID DEVICE:", dev)
            return "INVALID DEVICE", 400

        if mode == "auto":
            state[key] = None
        elif mode == "on":
            state[key] = True
        elif mode == "off":
            state[key] = False
        else:
            log("FORCE INVALID MODE:", mode)
            return "INVALID MODE", 400

        log("FORCE SET:", dev, "->", state[key])

    return redirect(url_for("index"))

def log_filename(ts):
    return ts.strftime("%d-%m-%y") + ".log"

def logger_loop():
    os.makedirs(LOG_DIR, exist_ok=True)

    last_log_ts = 0.0

    while True:
        now_ts = now()
        now_epoch = time.time()

        if (now_epoch - last_log_ts) >= LOG_INTERVAL_SEC:
            with STATE_LOCK:
                line = {
                    "time": now_ts.isoformat(),
                    "temperature": state["temperature"],
                    "humidity": state["humidity"],
                    "ppm": state["ppm"],
                    "Light": state["Light"],
                    "Humidifier": state["Humidifier"],
                    "Exhaust": state["Exhaust"],
                    "Heater": state["Heater"]
                }

            path = os.path.join(LOG_DIR, log_filename(now_ts))
            with open(path, "a") as f:
                f.write(json.dumps(line) + "\n")

            last_log_ts = now_epoch

        time.sleep(0.5)

LOGS_HTML = """
<!doctype html>
<html>
  <head>
    <title>Climate Monitor - Logs</title>
    <style>
      body {
        font-family: monospace;
        background:#111;
        color:#eee;
        margin: 20px;
      }

      h2, h3 {
        margin-top: 30px;
        margin-bottom: 10px;
      }

      table {
        border-collapse: collapse;
        width: 100%;
        max-width: 900px;
      }

      th, td {
        padding: 8px 10px;
        border-bottom: 1px solid #333;
        text-align: left;
        vertical-align: middle;
      }

      th {
        color: #aaa;
        font-weight: normal;
      }

      .green { color:#0f0; }
      .red   { color:#f33; }
      .muted { color:#888; }

      .actions {
        white-space: nowrap;
      }

      button {
        padding: 4px 10px;
        margin-right: 4px;
        background: #222;
        color: #eee;
        border: 1px solid #444;
        cursor: pointer;
      }

      button:hover {
        background: #333;
      }

      .btn-auto.active { border-color:#0f0; color:#0f0; }
      .btn-force.active { border-color:#f33; color:#f33; }
    </style>
  </head>
  <body>
    <p>Logged in as: {{session['user']}}</p>
    <h2><center><a href="/">Index</a> - <a href="/logs">Logs</a> - <a href="/live">Live</a> - <a href="/snip">Snip</a> - <a href="/logout">Logout</a></center></h2>

    <br>
    <h2>Log Files</h2>
    <a href="/">← Back</a>
    <ul>
    {% for f in files %}
      <li><a href="/logs/{{ f }}">{{ f }}</a></li>
    {% else %}
      <li>No logs found</li>
    {% endfor %}
    </ul>
  </body>
</html>
"""

LOG_VIEW_HTML = """
<!doctype html>
<html>
  <head>
    <title>Climate Monitor - {{ fname }}</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
      body {
        font-family: monospace;
        background:#111;
        color:#eee;
        margin: 20px;
      }

      h2, h3 {
        margin-top: 30px;
        margin-bottom: 10px;
      }

      table {
        border-collapse: collapse;
        width: 100%;
        max-width: 900px;
      }

      th, td {
        padding: 8px 10px;
        border-bottom: 1px solid #333;
        text-align: left;
        vertical-align: middle;
      }

      th {
        color: #aaa;
        font-weight: normal;
      }

      .green { color:#0f0; }
      .red   { color:#f33; }
      .muted { color:#888; }

      .actions {
        white-space: nowrap;
      }

      button {
        padding: 4px 10px;
        margin-right: 4px;
        background: #222;
        color: #eee;
        border: 1px solid #444;
        cursor: pointer;
      }

      button:hover {
        background: #333;
      }

      .btn-auto.active { border-color:#0f0; color:#0f0; }
      .btn-force.active { border-color:#f33; color:#f33; }
    </style>
  </head>
  <body>
    <p>Logged in as: {{session['user']}}</p>
    <h2><center><a href="/">Index</a> - <a href="/logs">Logs</a> - <a href="/live">Live</a> - <a href="/snip">Snip</a> - <a href="/logout">Logout</a></center></h2>

    <br>
    <h2>{{ fname }}</h2>
    <a href="/logs">← Back to logs</a>
    <button onclick="refreshLog()" style="margin-bottom:10px;">
      Refresh graph
    </button>
    <div style="max-width: 1920px; height: 700px;">
        <canvas id="climateChart"></canvas>
    </div>
    <script>
      const RELAY_BASE_TEMP = -0.02;
      const RELAY_STEP = 0.05;
      const ROW_LIGHT      = 0;
      const ROW_HUMIDIFIER = 1;
      const ROW_EXHAUST    = 2;
      const ROW_HEATER     = 3;

      function relayRow(states, rowIndex)
      {
        const y = RELAY_BASE_TEMP - rowIndex * RELAY_STEP;
        const out = [];
        let prev = null;

        for (const v of states)
        {
          if (v)
          {
            out.push(y);
            prev = y;
          }
          else
          {
            out.push(prev !== null ? y : null);
            prev = null;
          }
        }

        return out;
      }
      
      function isDayTime(label)
      {
        const [h, m] = label.split(":").map(Number);
        const mins = h * 60 + m;

        const dayStart = 6 * 60 + 30;   // 06:30
        const dayEnd   = 24 * 60 + 30;  // 00:30 (next day)

        if (mins >= dayStart) return true;
        if (mins < 30) return true;     // 00:00–00:30
        return false;
      }

      function drawTimeBand(ctx, chart, yScale, yMin, yMax, color, predicate)
      {
        const area = chart.chartArea;
        const xScale = chart.scales.x;
        if (!area) return;

        const labels = chart.data.labels;

        let startIdx = null;

        for (let i = 0; i <= labels.length; i++)
        {
          const match = (i < labels.length) && predicate(labels[i]);

          if (match && startIdx === null)
          {
            startIdx = i;
          }
          else if (!match && startIdx !== null)
          {
            const x1 = xScale.getPixelForValue(startIdx);
            const x2 = xScale.getPixelForValue(i);

            const y1 = yScale.getPixelForValue(yMax);
            const y2 = yScale.getPixelForValue(yMin);

            ctx.fillStyle = color;
            //ctx.fillRect(x1, y1, x2 - x1, y2 - y1);

            startIdx = null;
          }
        }
      }

      const times = {{ times | safe }};
      const temps = {{ temps | safe }};
      const hums  = {{ hums  | safe }};

      const lights = {{ lights | safe }};
      const humidifiers = {{ humidifiers | safe }};
      const exhausts = {{ exhausts | safe }};
      const heaters = {{ heaters | safe }};

      const lightRow   = relayRow(lights, ROW_LIGHT);
      const humidRow   = relayRow(humidifiers, ROW_HUMIDIFIER);
      const exhaustRow = relayRow(exhausts, ROW_EXHAUST);
      const heaterRow  = relayRow(heaters, ROW_HEATER);

      const bgBandsPlugin = {
        id: "bgBands",
        beforeDatasetsDraw(chart)
        {
          const ctx = chart.ctx;
          const yTemp = chart.scales.yTempL;
          const yHum  = chart.scales.yHumL;

          // ---- DAY
          drawTimeBand(
            ctx, chart, yTemp,
            25.0, 28.0,
            "rgba(0,255,0,0.10)",
            t => isDayTime(t)
          );

          drawTimeBand(
            ctx, chart, yHum,
            45, 60,
            "rgba(0,255,0,0.10)",
            t => isDayTime(t)
          );

          // ---- NIGHT
          drawTimeBand(
            ctx, chart, yTemp,
            18.25, 21.25,
            "rgba(0,255,0,0.10)",
            t => !isDayTime(t)
          );

          drawTimeBand(
            ctx, chart, yHum,
            45, 60,
            "rgba(0,255,0,0.10)",
            t => !isDayTime(t)
          );
        }
      };

      const chart = new Chart(document.getElementById("climateChart"), {
        type: "line",
        data: {
          labels: times,
          datasets: [
            {
              label: "Temperature (°C)",
              data: temps,
              borderColor: "#ff9f1a",
              borderWidth: 3,
              tension: 0.15,
              pointRadius: 0,
              yAxisID: "yTempL",
              spanGaps: true
            },
            {
              label: "Humidity (%)",
              data: hums,
              borderColor: "#7ecbff",
              borderWidth: 3,
              tension: 0.15,
              pointRadius: 0,
              yAxisID: "yHumL",
              spanGaps: true
            },
            {
              label: "Light",
              data: lightRow,
              borderColor: "#fff59d",
              stepped: true,
              borderWidth: 4.5,
              pointRadius: 0,
              yAxisID: "yRelay"
            },
            {
              label: "Humidifier",
              data: humidRow,
              borderColor: "#1e3a8a",
              stepped: true,
              borderWidth: 4.5,
              pointRadius: 0,
              yAxisID: "yRelay"
            },
            {
              label: "Exhaust",
              data: exhaustRow,
              borderColor: "#ffffff",
              stepped: true,
              borderWidth: 4.5,
              pointRadius: 0,
              yAxisID: "yRelay"
            },
            {
              label: "Heater",
              data: heaterRow,
              borderColor: "#ff3b3b",
              stepped: true,
              borderWidth: 4.5,
              pointRadius: 0,
              yAxisID: "yRelay"
            }
          ]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          interaction: { mode: "index", intersect: false },
          scales: {
            yTempL: {
              position: "left",
              min: 15,
              max: 30,
              ticks: { stepSize: 0.5, color: "orange" },
              grid: {
                color: "rgba(255,255,255,0.10)",
                borderDash: ctx => (ctx.index % 2 === 0 ? [] : [4, 4])
              }
            },
            yTempR: {
              position: "right",
              min: 15,
              max: 30,
              ticks: { stepSize: 1, color: "orange" },
              grid: {
                color: "rgba(255,255,255,0.10)",
                borderDash: ctx => (ctx.index % 2 === 0 ? [] : [4, 4])
              }
            },
            yHumL: {
              position: "left",
              min: 35,
              max: 85,
              ticks: { stepSize: 3.3333333333333333333333333333334 / 2, color: "cyan" },
              grid: {
                color: "rgba(255,255,255,0.10)",
                borderDash: ctx => (ctx.index % 2 === 0 ? [] : [4, 4])
              }
            },
            yHumR: {
              position: "right",
              min: 35,
              max: 85,
              ticks: { stepSize: 3.3333333333333333333333333333334 / 2, color: "cyan" },
              grid: {
                color: "rgba(255,255,255,0.10)",
                borderDash: ctx => (ctx.index % 2 === 0 ? [] : [4, 4])
              }
            },
            yRelay: {
              min: -0.5,
              max: 3.5,
              grid: {
                borderDash: ctx => (ctx.index % 2 === 0 ? [] : [4, 4])
              }
            },
            x: {
              ticks: { color: "#aaa" },
              grid: {
                color: "rgba(255,255,255,0.07)",
                borderDash: ctx => (ctx.index % 2 === 0 ? [] : [6, 6])
              }
            }
          }
        },
        plugins: [bgBandsPlugin]
      });
    </script>
    <script>
      const canvas = document.getElementById("climateChart");

      let isDragging = false;
      let lastX = 0;

      function getXValueAtPixel(chart, pixelX)
      {
        const xScale = chart.scales.x;
        return xScale.getValueForPixel(pixelX);
      }

      // ---- Zoom X only (mouse wheel)
      canvas.addEventListener("wheel", e =>
      {
        e.preventDefault();

        const chartArea = chart.chartArea;
        if (!chartArea) return;

        const mouseX = e.offsetX;
        if (mouseX < chartArea.left || mouseX > chartArea.right) return;

        const xScale = chart.scales.x;

        const zoomFactor = e.deltaY > 0 ? 1.15 : 0.85;

        const min = xScale.min;
        const max = xScale.max;
        const center = getXValueAtPixel(chart, mouseX);

        const newMin = center - (center - min) * zoomFactor;
        const newMax = center + (max - center) * zoomFactor;

        xScale.options.min = newMin;
        xScale.options.max = newMax;

        chart.update("none");
      }, { passive: false });

      // ---- Pan X only (drag)
      canvas.addEventListener("mousedown", e =>
      {
        isDragging = true;
        lastX = e.clientX;
      });

      window.addEventListener("mouseup", () =>
      {
        isDragging = false;
      });

      window.addEventListener("mousemove", e =>
      {
        if (!isDragging) return;

        const dx = e.clientX - lastX;
        lastX = e.clientX;

        const xScale = chart.scales.x;
        const range = xScale.max - xScale.min;

        const shift = dx / canvas.clientWidth * range;

        xScale.options.min -= shift;
        xScale.options.max -= shift;

        chart.update("none");
      });
    </script>
    <script>
      async function refreshLog()
      {
        try
        {
          const r = await fetch("/api/logs/{{ fname }}", { cache: "no-store" });
          const d = await r.json();

          chart.data.labels = d.times;
          chart.data.datasets[0].data = d.temps;
          chart.data.datasets[1].data = d.hums;
          chart.data.datasets[2].data = relayRow(d.lights, ROW_LIGHT);
          chart.data.datasets[3].data = relayRow(d.humidifiers, ROW_HUMIDIFIER);
          chart.data.datasets[4].data = relayRow(d.exhausts, ROW_EXHAUST);
          chart.data.datasets[5].data = relayRow(d.heaters, ROW_HEATER);

          chart.update("none");   // 🔴 animasyon YOK
        }
        catch (e)
        {
          console.error("log refresh failed", e);
        }
      }

      setInterval(refreshLog, 10000); 
    </script>
  </body>
</html>
"""

@app.route("/logs")
@login_required
def logs_index():
    if not os.path.isdir(LOG_DIR):
        files = []
    else:
        files = sorted(
            [f for f in os.listdir(LOG_DIR) if f.endswith(".log")],
            reverse=True
        )

    return render_template_string(LOGS_HTML, files=files)

def stepped_with_break(values):
    out = []
    prev = None
    for v in values:
        if prev is not None and v != prev:
            out.append(None)  # break vertical edge
        out.append(v)
        prev = v
    return out

@app.route("/api/state")
@login_required
def api_state():
    with STATE_LOCK:
        last = state["lastReceive"]
        last_age = age_seconds(last)
        stale = (last is None) or (now() - last > timedelta(seconds=SERIAL_STALE_SEC))

        is_day = is_light_on_by_schedule(
            now(),
            state["lightBeginHour"],
            state["lightBeginMinute"],
            state["lightPeriodSeconds"]
        )

        tmin, tmax = (TEMP_DAY_MIN, TEMP_DAY_MAX) if is_day else (TEMP_NIGHT_MIN, TEMP_NIGHT_MAX)
        hmin, hmax = (HUM_DAY_MIN, HUM_DAY_MAX) if is_day else (HUM_NIGHT_MIN, HUM_NIGHT_MAX)

        temp = state["temperature"]
        hum = state["humidity"]

        return jsonify({
            "serverTime": now().strftime("%H:%M:%S"),
            "lastReceive": str(last) if last else "None",
            "lastAge": last_age,
            "stale": stale,

            "isDay": is_day,
            "tmin": tmin,
            "tmax": tmax,

            "temperature": temp,
            "humidity": hum,
            "ppm": state["ppm"],
            "hmin": hmin,
            "hmax": hmax,

            "relays": {k: state[k] for k in DEVICES}
        })

@app.route("/api/logs/<fname>")
@login_required
def api_log(fname):
    path = os.path.join(LOG_DIR, fname)
    if not os.path.isfile(path):
        return jsonify({"error": "not found"}), 404

    times = []
    temps = []
    hums = []
    lights = []
    humidifiers = []
    exhausts = []
    heaters = []

    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or not line.startswith("{") or not line.endswith("}"):
                continue

            try:
                d = json.loads(line)
            except json.JSONDecodeError:
                continue

            if "time" not in d:
                continue

            t = datetime.fromisoformat(d["time"]).strftime("%H:%M:%S")

            times.append(t)
            temps.append(d["temperature"])
            hums.append(d["humidity"])
            lights.append(bool(d["Light"]))
            humidifiers.append(bool(d["Humidifier"]))
            exhausts.append(bool(d["Exhaust"]))
            heaters.append(bool(d["Heater"]))

    return jsonify({
        "times": times,
        "temps": temps,
        "hums": hums,
        "lights": lights,
        "humidifiers": humidifiers,
        "exhausts": exhausts,
        "heaters": heaters
    })

@app.route("/logs/<fname>")
@login_required
def view_log(fname):
    path = os.path.join(LOG_DIR, fname)
    if not os.path.isfile(path):
        return "Log not found", 404

    times = []
    temps = []
    hums = []

    lights = []
    humidifiers = []
    exhausts = []
    heaters = []

    last_time = None

    with open(path) as f:
        for line in f:
            d = json.loads(line)

            # minute resolution, UNIQUE
            t = datetime.fromisoformat(d["time"]).strftime("%H:%M")

            # skip duplicate minutes completely
            if t == last_time:
                continue
            last_time = t

            times.append(t)
            temps.append(d["temperature"])
            hums.append(d["humidity"])

            lights.append(bool(d["Light"]))
            humidifiers.append(bool(d["Humidifier"]))
            exhausts.append(bool(d["Exhaust"]))
            heaters.append(bool(d["Heater"]))

    return render_template_string(
        LOG_VIEW_HTML,
        fname=fname,
        times=json.dumps(times),
        temps=json.dumps(temps),
        hums=json.dumps(hums),
        lights=json.dumps(lights),
        humidifiers=json.dumps(humidifiers),
        exhausts=json.dumps(exhausts),
        heaters=json.dumps(heaters)
    )

def photo_hourly_loop():
    last_hour = None

    while True:
        ts = now()
        if last_hour != ts.hour:
            last_hour = ts.hour
            capture_snapshot(save=True)
            log("PHOTO: hourly snapshot saved")

        time.sleep(30)

def serial_watchdog_loop():
    global serial_reset_requested

    while True:
        time.sleep(1)

        with STATE_LOCK:
            last = state["lastReceive"]
            if last is None:
                continue

            age = (now() - last).total_seconds()

            if age > SERIAL_STALE_SEC:
                if not serial_reset_requested:
                    log("WATCHDOG: Serial stale for", int(age), "s -> requesting reset")
                    serial_reset_requested = True

if __name__ == "__main__":
    threading.Thread(target=serial_loop, daemon=True).start()
    threading.Thread(target=logger_loop, daemon=True).start()
    threading.Thread(target=serial_watchdog_loop, daemon=True).start()
    threading.Thread(target=photo_hourly_loop, daemon=True).start()
    app.run(host="127.0.0.1", port=5000, threaded=True, debug=False, use_reloader=False)
