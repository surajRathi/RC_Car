#include <Arduino.h>

void setup() {
    Serial.begin(9600);
    Serial.println("Press any key to start...");
    while (!Serial.available());

    Serial1.begin(115200);

    delay(100);

    while (Serial.available()) Serial.read();
    while (Serial1.available()) Serial1.read();

    Serial.println("Started");
}

void loop() {
    while (Serial1.available())
        Serial.write(Serial1.read());

    while (Serial.available())
        Serial1.write(Serial.read());
}