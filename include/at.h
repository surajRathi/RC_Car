#ifndef CAR_AT_H
#define CAR_AT_H

#include "Arduino.h"

#define EOL "\r\n"

namespace at {

    class AT {
        uint8_t _rst;
    public:
        bool connected{false}, have_ip{false};
        bool connections[4] = {false};
        char ip_addr[16] = "", mac_addr[18] = "";


        HardwareSerial &serial;

        // TODO: Is 0 a valid pin on arduino
        explicit AT(HardwareSerial &dev, uint8_t reset_pin) : _rst(reset_pin),
                                                              serial{dev} {}

        bool initialize();

        bool reset();

        bool tick();

        bool wait_for_str(const char *str, unsigned long timeout, bool echo = false);

        template<size_t n>
        // int wait_for_strs(const char *const strs[n], unsigned long timeout, bool echo = false) {
        int wait_for_strs(const char *(&strs)[n], unsigned long timeout, bool echo = false) {
            int indices[n] = {0};

            const auto end = millis() + timeout;
            int ch;
            while (millis() < end) {
                if ((ch = serial.read()) != -1) {
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

        bool last_status, pending;

        int _timedRead(unsigned long timeout);

        bool cmd_ok(const char *cmd, unsigned long timeout, bool echo);

        bool start_server(/*byte port = 80*/);
    };
}
#endif //CAR_AT_H
