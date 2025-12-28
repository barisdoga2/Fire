from flask import Flask, render_template_string, redirect, url_for, request, jsonify
from datetime import datetime, timedelta
import json
import serial
import threading
import time

# ---------------- CONFIG ----------------
SERIAL_PORT = "/dev/ttyUSB0"
BAUD = 9600
UI_REFRESH_SEC = 60

# ---------------- STATE ----------------
STATE_LOCK = threading.Lock()

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
    "lightScheduleUpdate": False
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

@app.before_request
def debug_http():
    log("HTTP", request.method, request.path)

HTML = """
<!doctype html>
<html>
<head>
  <title>Climate Monitor</title>
  <meta http-equiv="refresh" content="{{ refresh }}">
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

<h2>Climate Control</h2>
<div class="muted">Auto-refresh: every {{ refresh }}s</div>

<table>
  <tr><th>Server Time</th><td>{{ server_time }}</td></tr>
  <tr>
    <th>Last Receive</th>
    <td class="{{ 'red' if stale else 'green' }}">
      {{ last_receive }}
      {% if last_age is not none %} ({{ last_age }}s ago){% endif %}
    </td>
  </tr>
  <tr><th>Temperature</th><td>{{ temperature }}</td></tr>
  <tr><th>Humidity</th><td>{{ humidity }}</td></tr>
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
               min="1" max=86399"
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

  <td>{{ state[k] }}</td>

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

</body>
</html>
"""

@app.route("/")
def index():
    with STATE_LOCK:
        last = state["lastReceive"]
        last_age = age_seconds(last)
        stale = (last is None) or (now() - last > timedelta(seconds=10))

        # Render friendly values
        temp = state["temperature"]
        hum = state["humidity"]
        ppm = state["ppm"]
        
        lightBeginHour = state["lightBeginHour"]
        lightBeginMinute = state["lightBeginMinute"]
        lightPeriodSeconds = state["lightPeriodSeconds"]

        return render_template_string(
            HTML,
            refresh=UI_REFRESH_SEC,
            server_time=now().strftime("%H:%M:%S"),
            last_receive=str(last) if last else "None",
            last_age=last_age,
            stale=stale,
            temperature=temp,
            humidity=hum,
            ppm=ppm,
            lightBeginHour=lightBeginHour,
            lightBeginMinute=lightBeginMinute,
            lightPeriodSeconds=lightPeriodSeconds,
            state=dict(state),   # snapshot
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

if __name__ == "__main__":
    threading.Thread(target=serial_loop, daemon=True).start()
    app.run(host="0.0.0.0", port=8080, debug=False, use_reloader=False)
