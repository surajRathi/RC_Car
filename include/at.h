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
        bool last_status{true}, pending{false};

        enum flag {
            con0 = 1u << 0u,
            con1 = 1u << 1u,
            con2 = 1u << 2u,
            con3 = 1u << 3u,
            data = 1u << 4u
        };
        // Bytes 0-3 Connection state change ; Byte 4 incoming data
        uint8_t flags = 0x0;

        // To be used if and only if byte 4 of flag is set
        struct avail_data {
            uint8_t conn_num;
            size_t length;
        } avail_data{0, 0};

        char ip_addr[16] = "", mac_addr[18] = "";

        HardwareSerial &serial;

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

        int _timedRead(unsigned long timeout);

        bool cmd_ok(const char *cmd, unsigned long timeout, bool echo);

        bool start_server(/*uint8_t port = 80*/);

        bool init_send_data(uint8_t conn_num, size_t length);

        bool close_conn(uint8_t num);
    };
}
#endif //CAR_AT_H
