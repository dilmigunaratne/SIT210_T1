#include <Wire.h>
#include <BH1750.h>

const int buttonPin = 2;     // External interrupt pin
const int ledPin1 = 4;       // LED toggled by button
const int ledPin2 = 5;       // LED toggled by BH1750

volatile bool led1State = false;
bool led2State = false;
bool lastLed2State = false;  // Track previous state for logging

BH1750 lightMeter;

unsigned long previousMillis = 0;
const long interval = 1000; // Check light every 1000ms
int lightThreshold = 15;    // lux threshold (adjust as needed)

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT_PULLUP); // Button with internal pull-up
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);

  Wire.begin();
  lightMeter.begin();

  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonInterrupt, FALLING);
  Serial.println("Setup complete. Waiting for events...");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float lux = lightMeter.readLightLevel();

    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");

    // Decide whether LED2 should be ON or OFF
    if (lux < lightThreshold) {
      led2State = true;
    } else {
      led2State = false;
    }

    // Only print when the state actually changes
    if (led2State != lastLed2State) {
      digitalWrite(ledPin2, led2State);
      Serial.println("Sensor Interrupt: LED2 toggled");
      lastLed2State = led2State; // update the last known state
    } else {
      digitalWrite(ledPin2, led2State); // still ensure LED state is correct
    }
  }
}

// Debounce logic for the button interrupt
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

void handleButtonInterrupt() {
  unsigned long currentTime = millis();
  if (currentTime - lastDebounceTime > debounceDelay) {
    led1State = !led1State;
    digitalWrite(ledPin1, led1State);
    Serial.println("Button Interrupt: LED1 toggled");
    lastDebounceTime = currentTime;
  }
}
