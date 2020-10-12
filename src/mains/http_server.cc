#include <Arduino.h>
// #include <cstring>
extern "C" {
#include <string.h>
}

#include "stop.h"

// #define gzip
#ifdef gzip

#include "../../html/http_iframe_gz.h"

auto &http_iframe_html = http_iframe_html_gz;
auto &http_iframe_html_len = http_iframe_html_gz_len;
#else

#include "../../html/http_iframe_html.h"

#endif
#define EOL "\r\n"
auto &ESP = Serial1;
int a = 0;

bool wait_for_str(const char *const str, unsigned long timeout, bool echo = false) {
    size_t index = 0;
    const size_t len = strlen(str);

    auto end = millis() + timeout;
    int ch;
    while (millis() < end) {
        if ((ch = ESP.read()) != -1) {
            if (echo) Serial.write(ch);
            if (ch == str[index]) {
                ++index;
                if (index == len)
                    return true;
            } else {
                if (ch != str[index]) index = 0;
                else index = 1;
            }
        } else {
            delay(10);
        }
    }
    return false;
}

bool wait_ok(unsigned long timeout, bool echo = false) {
    return wait_for_str("OK" EOL, timeout, echo);
    /*
    const static char *str = ;
    size_t index = 0;

    auto end = millis() + timeout;
    int ch;
    while (millis() < end) {
        if ((ch = ESP.read()) != -1) {
            if (echo) Serial.write(ch);
            if (ch == str[index]) {
                ++index;
                if (index == strlen(str))
                    return true;
            } else {
                *//*index = 0;
                if (ch == str[index]) index += 1;*//*
                if (ch != str[index]) index = 0;
                else index = 1;
            }
        } else {
            delay(10);
        }
    }
    return false;*/
}


template<size_t n, bool echo = false>
int wait_for_strs(const char *(&strs)[n], unsigned long timeout) {
// int wait_for_strs(const char *const strs[n], unsigned long timeout) {
    int indices[n] = {0};

    const auto end = millis() + timeout;
    int ch;
    while (millis() < end) {
        if ((ch = ESP.read()) != -1) {
            if (echo) Serial.write(ch);
            for (size_t i = 0; i < n; i++) {
                if (ch == strs[i][indices[i]]) {
                    ++indices[i];
                    if (strs[i][indices[i]] == '\0')
                        return i;
                } else {
                    if (ch != strs[i][indices[i]]) indices[i] = 0;
                    else indices[i] = 1;
                }
            }
        } else {
            delay(10);
        }
    }
    return -1;
}

template<typename T>
// T is an unsigned int
size_t num_digits(T num) { // TODO : check for small numbers first.
    return (num >= 1000000000) ? 9 :
           (num >= 100000000) ? 8 :
           (num >= 10000000) ? 7 :
           (num >= 1000000) ? 6 :
           (num >= 100000) ? 5 :
           (num >= 10000) ? 4 :
           (num >= 1000) ? 3 :
           (num >= 100) ? 2 :
           (num >= 10) ? 1 : 0;

}

void setup() {
    Serial.begin(9600);
    Serial.println("Insert coin to start...");
    while (Serial.read() == -1);

    ESP.begin(115200);
    delay(500);
    while (ESP.read() != -1); // Clear old data

    Serial.println("Resetting...");
    ESP.print("AT+RST" EOL);
    if (!wait_for_str("ready" EOL, 2000)) {
        Serial.println("Device not ready...");
        stop();
    }

    // Check for wifi state. TODO: use a command
    int ret;
    const char *wifi_strs[] = {"WIFI CONNECTED" EOL, "WIFI DISCONNECT" EOL};
    if ((ret = wait_for_strs(wifi_strs, 5000)) >= 0) {
        switch (ret) {
            case 0: // "WIFI CONNECTED"
                Serial.println("Connected!");
                if (!wait_for_str("WIFI GOT IP", 5000)) {
                    Serial.println("Didnt get ip");
                    // stop();
                }
                Serial.println("Have IP!");
                ESP.println("AT+CIFSR" EOL);
                wait_ok(1000, true);
                break;

            case 1:
                Serial.println("Not connected!");

                // Connect to wifi
                ESP.print("AT+CWMODE=1" EOL);
                if (!wait_ok(500)) {
                    Serial.println("Could not change mode");
                    stop();
                }

                ESP.print("AT+CWJAP=\"blah\",\"test1234\"" EOL);
                if (!wait_for_str("WIFI CONNECTED" EOL, 50000)) {
                    Serial.println("Cant connect to wifi.");
                    stop();
                }

                if (!wait_for_str("WIFI GOT IP" EOL, 10000)) {
                    Serial.println("getting IP timed out.");
                    stop();
                }

                if (!wait_ok(5000)) {
                    Serial.println("Didnt get ok from conn");
                    stop();
                }

                ESP.println("AT+CIFSR" EOL);
                wait_ok(1000, true);
                break;

            default:
                Serial.print("Invalid wifi response answer");
                stop();
        }
    } else {
        Serial.print("Could not get wifi state.");
        stop();
    }

    // Set Multiplexing
    ESP.print("AT+CIPMUX=1" EOL);
    if (!wait_ok(500)) {
        Serial.println("Cant multiplex conn");
        stop();
    }

    // Set Server
    ESP.print("AT+CIPSERVER=1,80" EOL);
    if (!wait_ok(500)) {
        Serial.println("Cant start server");
        stop();
    }

    Serial.println("Listening at port 80");

    ESP.println("AT" EOL);
    if (!wait_ok(500)) Serial.println("Not OK");
    delay(100);

    Serial.println("\nReady...\n\n");
    Serial.flush();
}

void write_cipsend(char conn_num, size_t header_len) {
    ESP.print("AT+CIPSEND=");
    ESP.print(conn_num);
    ESP.print(",");
    ESP.print(header_len);
    ESP.print(EOL);

    char ch;
    Serial.println("wait for >");
    while ((ch = ESP.read()) != '>'); // Serial.write(ch);
    Serial.println("wait for space");
    while ((ch = ESP.read()) != ' '); // Serial.write(ch);
}

void send_main_html(char conn_num) {
    static const char *resp_header[] = {
            "HTTP/1.1 200 OK" EOL
            "Content-Length: ", /*content_len*/
            EOL "Content-Type: text/html; charset=utf-8" EOL EOL EOL
    };

    size_t header_len = strlen(resp_header[0]) + strlen(resp_header[1]) + num_digits(http_iframe_html_len);

    write_cipsend(conn_num, header_len);

    ESP.write(resp_header[0], strlen(resp_header[0]));
    ESP.print(http_iframe_html_len);
    ESP.write(resp_header[1], strlen(resp_header[1]));
    ESP.write(EOL EOL);

    Serial.print("Wrote header of ");
    Serial.print(header_len);
    Serial.println(" bytes.");

    // HTTP Data
    write_cipsend(conn_num, http_iframe_html_len);
    Serial.println("Wrote cipsend");
    ESP.write(http_iframe_html, http_iframe_html_len);

    Serial.print("Wrote body of ");
    Serial.print(header_len);
    Serial.println(" bytes.");
}

void send_state(char conn_num) {
    static const char *body[] = {"Value: " /*val*/ , EOL};
    size_t content_len = strlen(body[0]) + num_digits(a) + strlen(body[1]);
    static const char *resp_header[] = {
            "HTTP/1.1 200 OK" EOL
            "Content-Length: ", /*content_len*/
            EOL "Content-Type: text/plain; charset=utf-8" EOL
            #ifdef gzip
            "Content-Encoding: gzip " EOL
            #endif
            EOL
    };

    size_t header_len = strlen(resp_header[0]) + strlen(resp_header[1]) + num_digits(content_len);
    // size_t header_len = strlen(resp_header[0]) + strlen(resp_header[1]) + num_digits(http_iframe_html_len);

    write_cipsend(conn_num, header_len);

    ESP.write(resp_header[0], strlen(resp_header[0]));
    ESP.print(content_len);
    ESP.write(resp_header[1], strlen(resp_header[1]));

    ESP.write(EOL EOL);

    Serial.print("Wrote header of ");
    Serial.print(header_len);
    Serial.println(" bytes.");

    // HTTP Data
    write_cipsend(conn_num, content_len);
    ESP.write(body[0], strlen(body[0]));
    ESP.print(a);
    ESP.write(body[1], strlen(body[1]));

    Serial.println("Sent the state");
}

void send_404(char conn_num) {
    static const char *resp_header[] = {
            "HTTP/1.1 404 Not Found" EOL
            "Connection: close" EOL EOL
    };

    write_cipsend(conn_num, strlen(resp_header[0]));
    ESP.write(resp_header[0], strlen(resp_header[0]));

    Serial.println("Wrote 404");
}


int to_int(const char *buffer, size_t len) {
    // Serial.println("to_int");
    // Serial.println(len);
    // Serial.write(buffer, len);
    int ret = 0;
    for (int i = 0; i < len; i++) {
        ret *= 10;
        ret += buffer[i] - '0';
    }
    return ret;
}


void loop() {
    if (wait_for_str("+IPD,", 100000, false)) {
        Serial.print("Received data at connection ");

        char conn_num;
        while (true) { // TODO : Can directly get
            char ch;
            while ((ch = ESP.read()) == -1);
            if (ch == ',') break;
            conn_num = ch;
        }

        Serial.print(conn_num);
        Serial.print(" of ");

        // Parse get data length
        size_t len = 0;
        while (true) {
            char ch;
            while ((ch = ESP.read()) == -1);
            if (ch == ':') break;
            len *= 10;
            len += ch - '0';
        }

        Serial.print(len);
        Serial.println(" chars.");

        char buffer[20];
        buffer[0] = '/';
        size_t b_len = 1;

        bool slash = false, sp = false;
        for (int i = 0; i < len; i++) {
            char ch;
            while ((ch = ESP.read()) == -1);
            // TODO: Parse...
            // Need function with length and timeout
            if (sp) continue; // Already found the loc
            if (!slash) { // Need to find start of path
                if (ch == '/') slash = true;
            } else {
                if (ch == ' ') sp = true; // End of path.
                else if (b_len < sizeof(buffer)) {
                    buffer[b_len] = ch;
                    ++b_len;
                }
            }
            // Serial.print(ch);
        }
        Serial.println("Read Request");
        Serial.print("Requested ");
        Serial.write(buffer, b_len);
        Serial.println(" .");

        if (b_len == 1 /*&& buffer[0] == '/'*/)
            send_main_html(conn_num);
        else if (b_len == strlen("/state") && memcmp(buffer, "/state", strlen("/state")) == 0) {
            send_state(conn_num);
        } else if (b_len > strlen("/send?") && memcmp(buffer, "/send?", strlen("/send?")) == 0) {
            auto i = to_int(buffer + strlen("/send?"), b_len - strlen("/send?"));
            a += i;
            Serial.println(a);
            send_state(conn_num);
        } else
            send_404(conn_num);

        ESP.flush();
        Serial.println();
    }
}