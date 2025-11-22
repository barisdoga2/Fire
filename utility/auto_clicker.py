from pynput.mouse import Button, Controller
from pynput.keyboard import Listener, Key
import threading
import time

mouse = Controller()
clicking = False   # is clicking on/off


def click_loop():
    global clicking
    while True:
        if clicking:
            mouse.click(Button.left)
            time.sleep(1/20)  # 3 clicks per second
        else:
            time.sleep(0.01)


def on_press(key):
    global clicking
    # Detect F4 (this works on ALL keyboards)
    if key == Key.f4:
        clicking = not clicking


# Start clicking thread
threading.Thread(target=click_loop, daemon=True).start()

print("[INFO] Auto-clicker running.")
print("[INFO] Press F4 to start/stop clicking.")

# Start key listener
with Listener(on_press=on_press) as listener:
    listener.join()
