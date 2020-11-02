#ifndef CAR_AT_H
#define CAR_AT_H

#include "Arduino.h"

#define EOL "\r\n"

namespace at {

    namespace flag {
        enum flag : unsigned {
            // These flags represent a change in conn state
            // conn -> close or close -> conn
            con0 = 1u << 0u,
            con1 = 1u << 1u,
            con2 = 1u << 2u,
            con3 = 1u << 3u,

            // If any data is available (from any connection), details stored in avail_data
            // if set, must read avail_data.length bytes from device
            data = 1u << 4u,
            none = 0
        };

        static flag from_conn_num(byte num) {
            switch (num) {
                case 0:
                    return flag::con0;
                case 1:
                    return flag::con1;
                case 2:
                    return flag::con2;
                case 3:
                    return flag::con3;
                default:
                    return flag::none;
            }
        }

    }
    /**
     * Interfaces with an AT based ESP8266 device over HardwareSerial
     * First initialize the device
     * Then call tick to parse the next line of data from the device
     * The device sets flags when it receives/closes connections or data is received
     *
     */
    class AT {
        uint8_t _rst;
    public:

        // Wifi Status
        bool connected{false}, have_ip{false};

        // Incoming connection status
        bool connections[4] = {false};

        // Command running status. TODO
        bool last_status{true}, pending{false};

        uint8_t flags = 0x0;

        // To be used if and only if byte 4 of flag is set
        // Means we can now read length bytes from device
        struct {
            uint8_t conn_num;
            size_t length;
        } avail_data{0, 0};

        // Stored from output of CIFSR
        char ip_addr[16] = "", mac_addr[18] = "";

        HardwareSerial &serial;


        explicit AT(HardwareSerial &dev, uint8_t reset_pin) : _rst(reset_pin),
                                                              serial{dev} {}

        // Start Serial connetion and reset device
        bool initialize();

        // Hard Reset the device and change to station mode
        bool reset();

        // Check for and parse the next statement in serial
        bool tick();

        // Blocking read from serial until specific string is read
        bool wait_for_str(const char *str, unsigned long timeout, bool echo = false);

        // Blocking read from serial until specific string is read
        struct wait_for_str_ret {
            bool success;
            size_t bytes_read;
        };

        wait_for_str_ret wait_for_str(const char *str, unsigned long timeout, size_t max_chars, bool echo = false);

        // Read single byte with timeout
        int _timedRead(unsigned long timeout);

        // Send the cmd and wait for OK
        bool cmd_ok(const char *cmd, unsigned long timeout, bool echo = false);

        // Enabe multiplexing and open servere on :80
        bool start_server(/*uint8_t port = 80*/);

        // Sends a CIPSEND to the device
        // @return if true, must write length bytes to the device
        bool init_send_data(uint8_t conn_num, size_t length);

        // Send CIPCLOSE
        bool close_conn(uint8_t num);

        // Returns when any string from strs is encountered
        // @return index of that string in strs
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

        struct wait_for_strs_ret {
            size_t index;
            size_t bytes_read;
        };

        // Returns when any string from strs is encountered
        // @return index and number of characters read. If not found, returns n
        // std::pair<int, size_t> wait_for_strs(const char *(&strs)[n], unsigned long timeout, size_t max_chars, bool echo = false) {
        // TODO: why template
        template<size_t n>
        wait_for_strs_ret
        wait_for_strs(const char *(&strs)[n], unsigned long timeout, size_t max_chars, bool echo = false) {
            int indices[n] = {0};

            const auto end = millis() + timeout;
            size_t read = 0;

            int ch;
            while (millis() < end && read < (max_chars - 1)) {
                if ((ch = serial.read()) != -1) {
                    ++read;
                    if (echo) Serial.write(ch);
                    for (size_t i = 0; i < n; i++) {
                        if (ch == strs[i][indices[i]]) {
                            ++indices[i];
                            if (strs[i][indices[i]] == '\0')
                                return {i, read};
                        } else {
                            if (ch != strs[i][indices[i]]) indices[i] = 0;
                            else indices[i] = 1;
                        }
                    }
                } else {
                    delay(10);
                }
            }
            return {n, read};
        }

    };
}
#endif //CAR_AT_H
