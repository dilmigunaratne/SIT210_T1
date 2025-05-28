from flask import Flask, render_template, request, jsonify
from threading import Timer, Lock, Thread
from datetime import datetime
import time
import smbus
import RPi.GPIO as GPIO


app = Flask(__name__)


BUZZER_PIN = 22
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
GPIO.setup(BUZZER_PIN, GPIO.OUT)


# I2C setup
I2C_ADDRESS = 0x08  # Replace with your Arduino's I2C address
bus = smbus.SMBus(1)  # 1 indicates /dev/i2c-1

# Globals
medicine_times = {
    'morning': '08:00',
    'afternoon': '13:00',
    'night': '20:00'
}
buzzing = False
buzzing_lock = Lock()
current_alert_slot = None
countdown_timer = None
taken_log = []

# Buzzer control 
def buzzer_on():
    try:
        GPIO.output(BUZZER_PIN, GPIO.HIGH)
        print("[BUZZER] ON")
    except Exception as e:
        print(f"[ERROR] Buzzer ON failed: {e}")

def buzzer_off():
    try:
        GPIO.output(BUZZER_PIN, GPIO.LOW)
        print("[BUZZER] OFF")
    except Exception as e:
        print(f"[ERROR] Buzzer OFF failed: {e}")

def buzzer_loop():
    global buzzing
    while buzzing:
        buzzer_on()
        time.sleep(0.5)
        buzzer_off()
        time.sleep(0.5)

def log_dose(slot, status):
    taken_log.append({
        'slot': slot,
        'time': datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        'status': status
    })

def trigger_reminder(slot):
    global current_alert_slot, countdown_timer, buzzing
    print(f"[INFO] Triggering reminder for {slot}")
    with buzzing_lock:
        current_alert_slot = slot
        buzzing = True
        Thread(target=buzzer_loop, daemon=True).start()
        app.config['CURRENT_ALERT'] = slot

    # Schedule to stop beeping and start countdown
    def start_countdown():
        global buzzing
        with buzzing_lock:
            buzzing = False  # This will stop the buzzer loop
        start_5_minute_window(slot)

    Timer(10, start_countdown).start()


def start_5_minute_window(slot):
    global countdown_timer
    countdown_timer = Timer(300, mark_missed_dose)
    countdown_timer.start()

def mark_missed_dose():
    global current_alert_slot, countdown_timer
    print(f"[INFO] Dose missed for {current_alert_slot}")
    log_dose(current_alert_slot, 'missed')
    with buzzing_lock:
        buzzer_off()
        current_alert_slot = None
        countdown_timer = None
        app.config['CURRENT_ALERT'] = None

# Flask Routes
@app.route('/')
def home():
    return render_template('home.html', medicine_times=medicine_times)

@app.route('/get_alert')
def get_alert():
    return jsonify({'alert': app.config.get('CURRENT_ALERT')})

@app.route('/set_time', methods=['POST'])
def set_time():
    medicine_times['morning'] = request.form['morning']
    medicine_times['afternoon'] = request.form['afternoon']
    medicine_times['night'] = request.form['night']
    return render_template('home.html', medicine_times=medicine_times)

@app.route('/summary')
def summary():
    return render_template("summary.html", log=taken_log)

# Check medicine schedule every 30s
def check_medicine_schedule():
    while True:
        now = datetime.now().strftime("%H:%M")
        for slot, time_str in medicine_times.items():
            if now == time_str and app.config.get('CURRENT_ALERT') is None:
                trigger_reminder(slot)
        time.sleep(50)

# Arduino I2C listener
def listen_for_button_press():
    global countdown_timer, current_alert_slot
    while True:
        try:
            # Read a byte from Arduino
            button_status = bus.read_byte(I2C_ADDRESS)
            if button_status == 2:  # Assume Arduino sends 2 when button pressed
                print("[I2C] Button Pressed received")
                if current_alert_slot and countdown_timer:
                    countdown_timer.cancel()
                    log_dose(current_alert_slot, 'taken')
                    with buzzing_lock:
                        buzzer_off()
                        current_alert_slot = None
                        countdown_timer = None
                        app.config['CURRENT_ALERT'] = None
            time.sleep(1)
        except Exception as e:
            print(f"[I2C ERROR] {e}")
            time.sleep(1)

if __name__ == '__main__':
    Thread(target=listen_for_button_press, daemon=True).start()
    Thread(target=check_medicine_schedule, daemon=True).start()
    app.config['CURRENT_ALERT'] = None
    app.run(host='0.0.0.0', port=5000)
