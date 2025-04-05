#include <WiFiNINA.h>
#include <PubSubClient.h>  // MQTT library
#include <BH1750.h>
#include <Wire.h>


// WiFi Credentials
#define WIFI_SSID "NOKIA-F776"
#define WIFI_PASSWORD "72abf27252"

// MQTT Broker Credentials
#define MQTT_SERVER "737ff9e089a54d5b8e75bcf49a0804c8.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_TOPIC "sensor/light"
#define MQTT_ALERT_TOPIC "sensor/email_alert"
#define MQTT_USER "hivemq.webclient.1743081729184"
#define MQTT_PASSWORD ":tyT.9&;zK8xVHYhO6l5"


// Initialize BH1750 Light Sensor
BH1750 lightMeter;

// Use SSL Client instead of regular WiFiClient
WiFiSSLClient wifiSSLClient;
PubSubClient mqttClient(wifiSSLClient);

// Function to Connect to MQTT Broker
void connectMQTT() {
   while (!mqttClient.connected()) {
       Serial.println("Connecting to MQTT...");
       if (mqttClient.connect("ArduinoNanoIoT", MQTT_USER, MQTT_PASSWORD)) {
           Serial.println("Connected to MQTT Broker!");
       } else {
           Serial.print("Failed, rc=");
           Serial.print(mqttClient.state());
           Serial.println(" Retrying in 5 seconds...");
           delay(5000);
       }
   }
}

void setup() {
   Serial.begin(115200);
   Wire.begin();
   lightMeter.begin();

   // Connect to WiFi
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
   while (WiFi.status() != WL_CONNECTED) {
       delay(1000);
       Serial.println("Connecting to WiFi...");
   }
   Serial.println("Connected to WiFi");

   // Set MQTT Server
   mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  
   // Connect to MQTT
   connectMQTT();
}

void loop() {
   if (!mqttClient.connected()) {
       connectMQTT();
   }
   mqttClient.loop(); 

   float lux = lightMeter.readLightLevel();

   Serial.print("Light Intensity: ");
   Serial.print(lux);
   Serial.println(" lx");

   String luxStr = String(lux);
   mqttClient.publish(MQTT_TOPIC, luxStr.c_str());



   delay(5000); 
}