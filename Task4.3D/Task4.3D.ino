#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>

// Pin configuration
#define BUTTON_PIN 2
#define LED1_PIN 3
#define LED2_PIN 4
#define LED3_PIN 5
#define DHT_PIN 6

// Sensor setup
BH1750 lightSensor;
DHT dhtSensor(DHT_PIN, DHT11);

// Flags for events
volatile bool isButtonPressed = false;

// Timer variables
unsigned long lastMillis = 0;
const long timerInterval = 1000; // 1 second

// Threshold constants
const float LIGHT_THRESHOLD = 300.0;  // in Lux
const float TEMP_THRESHOLD = 30.0;    // in Celsius

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for serial connection

  // Configure pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

  // Attach external interrupt for button
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, FALLING);

  // Initialize sensors
  Wire.begin();
  lightSensor.begin();
  dhtSensor.begin();
  delay(2000); // Let DHT settle

  Serial.println("System initialized");
}

void loop() {
  unsigned long currentMillis = millis();

  // Timer-based sensor reading
  if (currentMillis - lastMillis >= timerInterval) {
    lastMillis = currentMillis;

    // Read sensors
    float lightLevel = lightSensor.readLightLevel();
    float temperature = dhtSensor.readTemperature();

    // Check light threshold and control LED2
    if (lightLevel > LIGHT_THRESHOLD) {
      digitalWrite(LED2_PIN, HIGH);
      Serial.println("Light threshold exceeded - LED2 ON");
    } else {
      digitalWrite(LED2_PIN, LOW);
      Serial.println("Light below threshold - LED2 OFF");
    }

    // Check temperature threshold
    if (!isnan(temperature) && temperature > TEMP_THRESHOLD) {
      Serial.println("Temperature threshold exceeded");
    } else {
      Serial.println("Temperature normal");
    }

    // Toggle LED3 every second
    digitalWrite(LED3_PIN, !digitalRead(LED3_PIN));

    // Display sensor values
    Serial.print("Light: ");
    Serial.print(lightLevel);
    Serial.print(" lux | Temp: ");
    Serial.print(temperature);
    Serial.println(" °C");
  }

  // Handle button press
  if (isButtonPressed) {
    isButtonPressed = false;
    digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
    Serial.println("Button pressed - LED1 toggled");
  }
}

// Button ISR
void onButtonPress() {
  static unsigned long lastInterrupt = 0;
  unsigned long now = millis();

  if (now - lastInterrupt > 200) {
    isButtonPressed = true;
  }
  lastInterrupt = now;
}
