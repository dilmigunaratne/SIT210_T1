#include <WiFiNINA.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "NOKIA-F776"; 
const char* password = "72abf27252"; 

//MQTT broker settings
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;
const char* topic = "SIT210/wave -  Dilmi";

// MQTT client
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

//Hardware pins
const int trigPin = 2;
const int echoPin = 3;
const int ledPin = 5;

unsigned long lastWaveTime = 0;  // Timestamp of last wave detection
unsigned long lastPatTime = 0;
const int debounceTime = 2000;   // Minimum time between wave detections (2 sec)
const int waveThreshold = 10;    // Distance threshold (in cm)
const int patThreshold =5;

// Function to connect to WiFi
void connectWiFi() {
    Serial.print("Connecting to WiFi...");
    while (WiFi.begin(ssid, password) != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi!");
}

// Function to connect to MQTT broker
void connectMQTT() {
    Serial.print("Connecting to MQTT broker...");
    while (!mqttClient.connected()) {
        if (mqttClient.connect("ArduinoClient")) {
            Serial.println("Connected to MQTT broker!");
            mqttClient.subscribe(topic);
        } else {
            Serial.print(".");
            delay(2000);
        }
    }
}

// Callback function for MQTT messages
void callback(char* receivedTopic, byte* payload, unsigned int length) {
    Serial.print("Message received: ");
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);

    if (message == "Dilmi") {
        // Blink LED 3 times for "Dilmi"
        for (int i = 0; i < 3; i++) {
            digitalWrite(ledPin, HIGH);
            delay(500);
            digitalWrite(ledPin, LOW);
            delay(500);
        }
    }else if (message == "pat") {
        // Flash LED fast 5 times
        for (int i = 0; i < 5; i++) {
            digitalWrite(ledPin, HIGH);
            delay(200);
            digitalWrite(ledPin, LOW);
            delay(200);
        }
    }
}

// Function to measure distance
int getDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 30000); // Timeout after 30ms
    int distance = (duration * 0.0343) / 2;

    // Ensure valid readings
    if (distance <= 0 || distance > 400) {
        return -1; // Invalid reading
    }
    return distance;
}

// Function to detect a "Dilmi"
// void detectWave() {
//     int stableCount = 0;
//     const int requiredStableReadings = 3; // Require 3 consecutive valid readings
//     int distance;

//     for (int i = 0; i < requiredStableReadings; i++) {
//         distance = getDistance();
//         Serial.print("Measured Distance: ");
//         Serial.print(distance);
//         Serial.println(" cm");

//         if (distance > 0 && distance < waveThreshold) {
//             stableCount++;
//         } else {
//             stableCount = 0;  // Reset count if an invalid reading occurs
//         }
//         delay(100);
//     }

//     if (stableCount >= requiredStableReadings) {
//         unsigned long currentTime = millis();
//         if (currentTime - lastWaveTime >= debounceTime) {
//             Serial.println("Wave detected! Publishing message...");
//             mqttClient.publish(topic, "Dilmi");
//             lastWaveTime = currentTime;
//         }
//     }
// }

// Detect pat gesture
void detectPat() {
    int count = 0;
    for (int i = 0; i < 2; i++) {
        int distance = getDistance();
        Serial.print("Pat Check Distance: ");
        Serial.println(distance);
        if (distance > 0 && distance < patThreshold) {
            count++;
        }
        delay(100);
    }

    if (count >= 2 && millis() - lastPatTime > debounceTime) {
        mqttClient.publish(topic, "pat");
        Serial.println("Pat detected! Published: pat");
        lastPatTime = millis();
    }
}

// Setup function
void setup() {
    Serial.begin(115200);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(ledPin, OUTPUT);

    connectWiFi();
    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(callback);
    connectMQTT();
}

// Loop function
void loop() {
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();
    //detectWave();
    detectPat();
    delay(500);
}






