#include <Arduino.h>
#include <stop.h>
#include <at.h>
#include "../../html/http_sse_html.h"

#define EOL "\r\n"
at::AT esp(Serial1, 22);
int state = 0;

template<typename T>
// T is an unsigned int
size_t num_digits(T num) { // TODO : check for small numbers first.
    return (num >= 1000000000) ? 10 :
           (num >= 100000000) ? 9 :
           (num >= 10000000) ? 8 :
           (num >= 1000000) ? 7 :
           (num >= 100000) ? 6 :
           (num >= 10000) ? 5 :
           (num >= 1000) ? 4 :
           (num >= 100) ? 3 :
           (num >= 10) ? 2 : 1;
}

void setup() {
    Serial.begin(9600);
    Serial.println("Insert a coin to start...");
    int c;
    while ((c = Serial.read()) == -1);
    if (c == 'p') {
        // Serial passthrough mode:
        Serial1.begin(115200);
        delay(100);
        Serial.println("Started Passthrough");

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
        while (true) {
            while (Serial1.available())
                Serial.write(Serial1.read());

            while (Serial.available())
                Serial1.write(Serial.read());
        }
#pragma clang diagnostic pop
        stop();
    }
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

bool serve_request(const char *path, const char *accept_methods, const char *http_version);

void loop() {
    while (esp.tick());
    sse &= ~esp.flags; // If connection state changed, reset the sse state
    esp.flags &= ~(0b1111u); // Clear new connection flags

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
            Serial.println("SSE RECV DATA");

            while (remaining > 1 /*0*/) {
                if ((ch = esp._timedRead(100)) == -1) {
                    Serial.println("read data timed out");
                    break;
                }
                remaining--;
                Serial.write(ch);
            }

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

            // TODO: Find bytes leak, dont timeout.
            while (remaining > 0) { // TODO Support http body
                // Serial.println(remaining);
                static const char *header_label[] = {"Host: ", "Accept: ", EOL};
                auto[vers_index, vers_bytes_read] = esp.wait_for_strs(header_label, 100, remaining, false);
                remaining -= vers_bytes_read;
                if (!vers_bytes_read) {
                    Serial.println("Bytes leak.");
                    break;
                }
                // Serial.println(vers_bytes_read);
                if (vers_index == 0) {
                    read = esp.serial.readBytesUntil(EOL[1], host, min(sizeof(host) - 1, remaining));
                    remaining -= read;
                    // TODO: use length of EOL properly
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

            esp.flags &= ~at::flag::data;

            Serial.println();

            // Request /
            // Request /send?___
            // Request /stream
            serve_request(path, accept, http_version);

            Serial.println();
        }
    }
    int ch;
    // while ((ch = esp.serial.read()) != -1) Serial.write(ch);
    while ((ch = Serial.read()) != -1) Serial.write(ch), esp.serial.write(ch);
}

bool serve_request(const char *path, const char *accept_methods, const char *http_version) {
    if (strcmp(path, "/") == 0) {
        static char *resp_header[] = {
                "HTTP/___ 200 OK" EOL
                "Content-Length: ", /*content_len*/
                EOL "Content-Type: text/html; charset=utf-8" EOL EOL EOL
        };
        memcpy(resp_header[0] + 5, http_version, 3);

        size_t header_len = strlen(resp_header[0]) + strlen(resp_header[1]) + num_digits(http_sse_html_len);
        if (!esp.init_send_data(esp.avail_data.conn_num, header_len)) {
            Serial.println("Cant send data :(");
            esp.close_conn(esp.avail_data.conn_num);
            return false;
        }
        esp.serial.write(resp_header[0], strlen(resp_header[0]));
        esp.serial.print(http_sse_html_len);
        esp.serial.write(resp_header[1], strlen(resp_header[1]));
        esp.serial.write(EOL EOL);
        if (!esp.wait_for_str("SEND OK" EOL, 500, false)) {
            Serial.println("No send OK");
            while (!esp.serial.available()) esp.serial.write(EOL);
            esp.close_conn(esp.avail_data.conn_num);
            return false;
        }
        Serial.print("Wrote header of ");
        Serial.print(header_len);
        Serial.println(" bytes.");


        if (!esp.init_send_data(esp.avail_data.conn_num, http_sse_html_len)) {
            Serial.println("Cant send data :(");
            esp.close_conn(esp.avail_data.conn_num);
            return false;
        }
        esp.serial.write(http_sse_html, http_sse_html_len);
        if (!esp.wait_for_str("SEND OK" EOL, 500, false)) {
            Serial.println("No send OK");
            while (!esp.serial.available()) esp.serial.write(EOL);
            esp.close_conn(esp.avail_data.conn_num);
            return false;
        }
        Serial.print("Wrote body of ");
        Serial.print(header_len);
        Serial.println(" bytes.");
        esp.close_conn(esp.avail_data.conn_num);

    } else if (memcmp(path, "/send?", strlen("/send?")) == 0) {
        Serial.println("State change");
        const auto end = path + strlen(path);
        auto start = path + strlen("/send?");
        int change = 0;
        bool neg = false;
        if (*start == '-') {
            start++;
            neg = true;
        }
        while (start < end) {
            if ('0' <= *start and *start <= '9') {
                change *= 10;
                change += *start - '0';
            } else {
                Serial.println("Invalid state.");
            }
            start++;
        }
        if (neg) change *= -1;
        Serial.println(change);
        state += change;

        static char resp_header[] = {
                "HTTP/___ 204 No Content" EOL EOL EOL
        };
        size_t header_len = strlen(resp_header);
        if (!esp.init_send_data(esp.avail_data.conn_num, header_len)) {
            Serial.println("Cant send data :(");
            esp.close_conn(esp.avail_data.conn_num);
            return false;
        }
        esp.serial.write(resp_header);

        if (!esp.wait_for_str("SEND OK" EOL, 500, false)) {
            Serial.println("No send OK");
            while (!esp.serial.available()) esp.serial.write(EOL);
            esp.close_conn(esp.avail_data.conn_num);
            return false;
        }
        esp.close_conn(esp.avail_data.conn_num);

        // Update all:
        for (int i = 0; i < 4; i++) {
            if (sse & at::flag::from_conn_num(i)) {
                Serial.print("Updateing: ");
                Serial.println(i);
                const char *sse_data[] = {"data: ", "\n" EOL};
                if (!esp.init_send_data(i, num_digits(state) + strlen(sse_data[0]) + strlen(sse_data[1]))) {
                    Serial.println("Cant send data :(");
                    esp.close_conn(esp.avail_data.conn_num);
                } else {
                    esp.serial.write(sse_data[0]);
                    esp.serial.print(state);
                    esp.serial.write(sse_data[1]);
                    if (!esp.wait_for_str("SEND OK" EOL, 200, false)) {
                        Serial.println("No send OK");
                    }
                }
            }
        }


    } else if (strcmp(path, "/stream") == 0 && strcmp(accept_methods, "text/event-stream") == 0) {
        Serial.println("Received new event stream request.");
        static char *resp_header[] = {
                "HTTP/___ 200 OK" EOL
                "Connection: keep-alive" EOL
                "Content-type: text/event-stream" EOL
                "Cache-Control: no-cache" EOL EOL EOL
                // "Transfer-Encoding: chunked" EOL EOL EOL
        };
        memcpy(resp_header[0] + 5, http_version, 3);

        size_t header_len = strlen(resp_header[0]);
        if (!esp.init_send_data(esp.avail_data.conn_num, header_len)) {
            Serial.println("Cant send data :(");
            // esp.close_conn(esp.avail_data.conn_num);
            return false;
        }
        esp.serial.write(resp_header[0], strlen(resp_header[0]));
        if (!esp.wait_for_str("SEND OK" EOL, 500, false)) {
            Serial.println("No send OK");
            while (!esp.serial.available()) esp.serial.write(EOL);
            // esp.close_conn(esp.avail_data.conn_num);
            return false;
        }
        Serial.print("Responded to SSE request ");
        Serial.print(header_len);
        Serial.println(" bytes.");

        sse |= at::flag::from_conn_num(esp.avail_data.conn_num);
    } else {
        Serial.println("Sending 404");
        static char resp_header[] = {
                "HTTP/___ 404 Not Found" EOL
                "Connection: close" EOL EOL
        };
        memcpy(resp_header + 5, http_version, 3);

        if (!esp.init_send_data(esp.avail_data.conn_num, strlen(resp_header))) {
            Serial.println("Cant send data :(");
            esp.close_conn(esp.avail_data.conn_num);
            return false;
        }
        esp.serial.write(resp_header, strlen(resp_header));
        if (!esp.wait_for_str("SEND OK" EOL, 200, false)) {
            Serial.println("No send OK");
            while (!esp.serial.available()) esp.serial.write(EOL);
            return false;
        }

        esp.close_conn(esp.avail_data.conn_num);
    }

    return true;
}
