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

    if (!esp.connected) {
        auto end = millis() + 5000;
        while ((!esp.connected || !esp.have_ip) && millis() < end)
            while (esp.tick());

        if (!esp.connected) { // TODO: AT+CWJAP
            Serial.println("Cant connect");
            stop();
        } else if (!esp.have_ip) {
            Serial.println("Can't get IP");
            stop();
        }
    }

    Serial.println("Wifi Connected");
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
    while (esp.tick());
    esp.flags &= ~0b1111; // Clear new connection flags

    if (esp.flags & esp.flag::data) {
        int ch;
        for (size_t i = 0; i < esp.avail_data.length; ++i) {
            while ((ch = esp.serial.read()) == -1);
            Serial.write(ch);
        }

        esp.flags &= ~esp.flag::data;

        static const char *data = "hello world";
        if (esp.init_send_data(esp.avail_data.conn_num, strlen(data))) {
            esp.serial.write(data);
            if (!esp.wait_for_str("SEND OK" EOL, 100, false))
                Serial.println("Some write error...");
        } else
            Serial.println("Cant send data :(");
    }

    int ch;
    // while ((ch = esp.serial.read()) != -1) Serial.write(ch);
    while ((ch = Serial.read()) != -1) Serial.write(ch), esp.serial.write(ch);
}