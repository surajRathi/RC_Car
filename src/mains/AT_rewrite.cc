#include <Arduino.h>
#include <stop.h>
#include <at.h>

#define EOL "\r\n"
at::AT esp(Serial1, 22);

void setup() {
    Serial.begin(9600);
    Serial.println("Insert a coin to start...");
    while (Serial.read() == -1);
    delay(10);
    while (Serial.read() != -1);

    Serial.println("Resetting...");
    if (!esp.initialize()) {
        Serial.println("Reset Failed...");
        stop();
    }
    Serial.println("Ready");

    delay(500);
    while (esp.tick());

    if (!esp.connected) {
        Serial.println("Trying to connect to wifi.");
        delay(5000);
        while (esp.tick());
        if (!esp.connected) {
            Serial.println("Cant connect");
            stop();
        }
    }
    Serial.println("Connected");

    if (!esp.have_ip) {
        Serial.println("Trying to get IP");

        delay(2000);
        while (esp.tick());
        if (!esp.have_ip) {
            Serial.println("Can't get IP");
            stop();
        }
    }
    Serial.println("Have an IP");

    if (!esp.start_server()) {
        Serial.println("Couldn't start server...");
        stop();
    }

    esp.serial.write("AT+CIFSR" EOL);
    delay(200);
    while (esp.tick());

    Serial.print("Listening on ");
    Serial.print(esp.ip_addr);
    Serial.println(":80.");
}

void loop() {
    // while (esp.tick());

    int ch;
    while ((ch = esp.serial.read()) != -1) Serial.write(ch);
    while ((ch = Serial.read()) != -1) Serial.write(ch), esp.serial.write(ch);
}