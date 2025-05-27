from flask import Flask, render_template, request, jsonify
from datetime import datetime
import threading
import time
import RPi.GPIO as GPIO
import atexit
import paho.mqtt.client as mqtt

app = Flask(__name__)

BUZZER_PIN = 22
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
GPIO.setup(BUZZER_PIN, GPIO.OUT)

# Store medicine times
medicine_times = {
    "morning": "08:00",
    "afternoon": "13:00",
    "night": "20:00"
}

# Log of taken medicines
taken_log = []

# Control buzzer state
buzzing = False
current_alert_slot = None
buzzing_lock = threading.Lock()

# Ensure buzzer is off and GPIO is cleaned up
def shutdown():
    global buzzing
    with buzzing_lock:
        buzzing = False
        GPIO.output(BUZZER_PIN, GPIO.LOW)
    GPIO.cleanup()
    print("[INFO] GPIO cleaned up.")

atexit.register(shutdown)

def buzzer_on():
    GPIO.output(BUZZER_PIN, GPIO.HIGH)

def buzzer_off():
    GPIO.output(BUZZER_PIN, GPIO.LOW)

def buzzer_loop():
    global buzzing
    while buzzing:
        buzzer_on()
        time.sleep(0.5)
        buzzer_off()
        time.sleep(0.5)

# Check medicine time every 30 seconds
def check_medicine_schedule():
    global buzzing, current_alert_slot
    while True:
        now = datetime.now().strftime('%H:%M')
        with buzzing_lock:
            if not buzzing:
                for slot, slot_time in medicine_times.items():
                    if now == slot_time:
                        buzzing = True
                        current_alert_slot = slot
                        threading.Thread(target=buzzer_loop, daemon=True).start()
                        break
        time.sleep(30)

@app.route('/')
def home():
    return render_template('home.html', medicine_times=medicine_times)

@app.route('/set_time', methods=['POST'])
def set_time():
    medicine_times['morning'] = request.form.get('morning')
    medicine_times['afternoon'] = request.form.get('afternoon')
    medicine_times['night'] = request.form.get('night')
    return "Times updated", 302, {'Location': '/'}

@app.route('/get_alert')
def get_alert():
    global buzzing, current_alert_slot
    return jsonify({"alert": current_alert_slot if buzzing else None})

@app.route('/medicine_taken', methods=['POST'])
def medicine_taken():
    global buzzing, current_alert_slot
    data = request.get_json()
    slot = data.get('compartment')
    if slot and buzzing and slot == current_alert_slot:
        with buzzing_lock:
            buzzing = False
            buzzer_off()
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")          
            taken_log.append({'slot': slot, 'time': timestamp})
            current_alert_slot = None
        return jsonify({"status": "success"})
    return jsonify({"status": "fail"}), 400

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT with result code " + str(rc))
    client.subscribe("medicine/status")

def on_message(client, userdata, msg):
    message = msg.payload.decode()
    print("[MQTT] Received:", message)
    if ":" in message:
        compartment, status = message.split(":")
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        taken_log.append({
            "slot": compartment.strip(),
            "status": status.strip(),
            "time": timestamp
        })

mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect("localhost", 1883, 60)
threading.Thread(target=mqtt_client.loop_forever, daemon=True).start()


@app.route('/summary')
def summary():
    return render_template('summary.html', log=taken_log)

if __name__ == '__main__':
    threading.Thread(target=check_medicine_schedule, daemon=True).start()
    app.run(host='0.0.0.0', port=5000)

