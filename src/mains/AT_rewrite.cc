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

uint8_t sse = 0x0;

void loop() {
    while (esp.tick());
    sse &= ~esp.flags; // If connection state changed, reset the sse state
    esp.flags &= ~0b1111; // Clear new connection flags

    if (esp.flags & at::flag::data) {
        int ch;

        char path[20];
        char host[20];
        char accept[40];
        const char *method = nullptr;
        const char *http_version = nullptr;

        Serial.print("Received ");
        Serial.print(esp.avail_data.length);
        Serial.print(" bytes on ");
        Serial.println(esp.avail_data.conn_num);

        size_t remaining = esp.avail_data.length; // TODO: Enforce
        if (sse & at::flag::from_conn_num(esp.avail_data.conn_num)) {
            // TODO
        } else {
            // Simple http. Assume new connection
            static const char *req_methods[] = {"GET ", "POST ", " "};
            auto[meth, read] = esp.wait_for_strs(req_methods, 1000, remaining, false);
            remaining -= read;
            if (meth != 0) {
                Serial.println("Invalid method.");
                // Mop up extra data and send req error code.
            }
            method = req_methods[meth];
            if (remaining == 0) {/*TODO*/}

            read = esp.serial.readBytesUntil(' ', path, min(sizeof(path) - 1, remaining));
            remaining -= read;
            path[read] = '\0';

            static const char *http[] = {"HTTP/", "/"};
            auto[index, bytes_read] = esp.wait_for_strs(http, 100, min(remaining, strlen("HTTP/") + 1), false);
            remaining -= bytes_read;
            if (index != 0) {
                Serial.println("NO HTTP.");
                // Mop up extra data and send req error code.
            }
            if (remaining == 0) {/*TODO*/}

            static const char *httpvers[] = {"1.0" EOL, "1.1" EOL, EOL};
            auto[vers_index, vers_bytes_read] = esp.wait_for_strs(httpvers, 100, remaining);
            remaining -= vers_bytes_read;
            if (vers_index >= 2) {
                Serial.println("Invalid http version.");
                // Mop up extra data and send req error code.
            }
            http_version = httpvers[vers_index];
            if (remaining == 0) {/*TODO*/}

            while (remaining > 2) {
                static const char *header_label[] = {"Host: ", "Accept: ", EOL};
                auto[vers_index, vers_bytes_read] = esp.wait_for_strs(header_label, 100, remaining, false);
                remaining -= vers_bytes_read;
                if (vers_index == 0) {
                    read = esp.serial.readBytesUntil(EOL[1], host, min(sizeof(host) - 1, remaining));
                    remaining -= read;
                    // TODO: use lenght of eol properly
                    path[(read > 1 ? read - 1 : 0)] = '\0'; // Note strip the first char of EOL.
                } else if (vers_index == 1) {
                    read = esp.serial.readBytesUntil(EOL[1], accept, min(sizeof(accept) - 1, remaining));
                    remaining -= read;
                    accept[(read > 1 ? read - 1 : 0)] = '\0'; // Note strip the first char of EOL.
                }
            }

            Serial.println("Request: ");
            Serial.print(req_methods[meth]);
            Serial.print(' ');
            Serial.print(path);
            Serial.print(" HTTP/");
            Serial.print(httpvers[vers_index]);
            if (host[0] != '\0') {
                Serial.print("Host: ");
                Serial.println(host);
            }
            if (accept[0] != '\0') {
                Serial.print("Accept: ");
                Serial.println(accept);
            }
        }

        // Mop up extra data
        // TODO: Timeout cause reset
        while (remaining > 1 /*0*/) {
            if ((ch = esp._timedRead(100)) == -1) {
                Serial.println("read data timed out");
                break;
            }
            remaining--;
            Serial.write(ch);
        }

        esp.flags &= ~at::flag::data;

        Serial.println("Parsed incoming data.");

        /*// Request /
        if (strcmp(path, "/") == 0) {

        } else if (strncmp(path, "/stream?", strlen("/stream?")) == 0) {

        } else if (strcmp(path, "/stream") == 0 && strcmp(accept, "text/event-stream")) {
            Serial.println("Received new event stream request.");

        }
        // Request /send?___
        // Request /stream*/

        static const char *data = "hello world";
        if (esp.init_send_data(esp.avail_data.conn_num, strlen(data))) {
            esp.serial.write(data);
            if (!esp.wait_for_str("SEND OK" EOL, 100, false))
                Serial.println("Some write error...");
            else
                Serial.println("Wrote Data");
        } else
            Serial.println("Cant send data :(");

        esp.close_conn(esp.avail_data.conn_num);
        Serial.println();
        Serial.println();

    }

    int ch;
    // while ((ch = esp.serial.read()) != -1) Serial.write(ch);
    while ((ch = Serial.read()) != -1) Serial.write(ch), esp.serial.write(ch);
}