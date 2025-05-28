#include <Wire.h>

#define BUTTON_PIN 2
bool buttonPressed = false;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Wire.begin(0x08); // Arduino I2C slave address
  Wire.onRequest(sendData);
  Wire.onReceive(receiveData);
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      buttonPressed = true;
    }
  }
}

void sendData() {
  if (buttonPressed) {
    Wire.write(2);  // Notify Pi that button was pressed
    buttonPressed = false;
  } else {
    Wire.write(0);  // No button press
  }
}

void receiveData(int bytes) {
  int command = Wire.read();
  if (command == 1) {
    // Turn buzzer ON (implement if needed)
  } else if (command == 0) {
    // Turn buzzer OFF
  }
}
