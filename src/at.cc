#include <at.h>

using namespace at;

bool AT::initialize() {
    serial.begin(115200);
    serial.setTimeout(200);

    digitalWrite(_rst, LOW);

    return reset();
}

const char *ready = "ready" EOL;

bool AT::reset() {
    pinMode(_rst, OUTPUT);
    delay(500);
    pinMode(_rst, INPUT);

    // Wait for ready
    return wait_for_str(ready, 5000, false)
           && cmd_ok("ATE0" EOL, 100, false)
           && cmd_ok("AT+CWMODE=1" EOL, 100, false);
}

bool AT::wait_for_str(const char *const str, unsigned long timeout, bool echo) {
    size_t index = 0;
    const size_t len = strlen(str);

    auto end = millis() + timeout;
    int ch;
    while (millis() < end) {
        if ((ch = serial.read()) != -1) {
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


enum Cmd {
    wifi_conn = 0, wifi_disconn = 1, got_ip = 2, ok = 3, error = 4,
    ip = 5, mac = 6, recv_data = 7,
    con_0 = 8, con_1 = 9, con_2 = 10, con_3 = 11,
    clo_0 = 12, clo_1 = 13, clo_2 = 14, clo_3 = 15,
    none = 16
};

const char *cmds[] = {
        "WIFI CONNECTED" EOL,
        "WIFI DISCONNECT" EOL,
        "WIFI GOT IP" EOL,
        "OK" EOL,
        "ERROR" EOL,

        "+CIFSR:STAIP,",
        "+CIFSR:STAMAC,",
        "+IPD,",

        "0,CONNECT" EOL,
        "1,CONNECT" EOL,
        "2,CONNECT" EOL,
        "3,CONNECT" EOL,

        "0,CLOSED" EOL,
        "1,CLOSED" EOL,
        "2,CLOSED" EOL,
        "3,CLOSED" EOL,

        EOL // If no other command matches
};
// Each CmdName should correspond to a cmd.

bool AT::tick() {
    if (flags & flag::data) return false;

    if (!serial.available()) return false;
    // Read until cmd finish, EOL, or timeout

    switch ((Cmd) wait_for_strs(cmds, 100, false)) {
        case wifi_conn:
            connected = true;
            have_ip = false;
            break;

        case wifi_disconn:
            connected = false;
            have_ip = false;
            Serial.println("Disconnected.");
            break;

        case got_ip:
            have_ip = true;
            break;

        case ok:
            pending = false;
            Serial.println("OK" EOL);
            break;
        case error:
            pending = false;
            Serial.println("ERROR" EOL);
            break;

        case none:
            // Serial.println("No cmd.");
            break;

        case ip:
            // Read IP (till EOL) to internal buffer
            if (_timedRead(100) != '"') break;
            ip_addr[serial.readBytesUntil('"', ip_addr, sizeof(ip_addr) - 1)] = '\0';
            break;

        case mac:
            // Read MAC (till EOL) to internal buffer
            if (_timedRead(100) != '"') break;
            mac_addr[serial.readBytesUntil('"', mac_addr, sizeof(mac_addr) - 1)] = '\0';
            break;

        case recv_data:
            Serial.println("Receiving data");
            avail_data.conn_num = _timedRead(100) - '0';
            if (avail_data.conn_num < 0 || avail_data.conn_num >= 4) {
                while (serial.read() != -1);
                break;
            }

            if (_timedRead(100) != ',') {
                while (serial.read() != -1);
                break;
            }

            int ch;
            avail_data.length = 0;
            while ((ch = _timedRead(100)) != -1) {
                if (ch == ':') break;
                avail_data.length *= 10;
                avail_data.length += ch - '0'; // TODO check ch range.
            }

            if (avail_data.length > SERIAL_RX_BUFFER_SIZE - 20) {
                Serial.println("RX Buffer Overflow");
                while (serial.read() != -1);
                break;
            }

            flags |= flag::data;

            // TODO: Somehow allow only reading avail_data.length bytes.
            break;


        case con_0:
            connections[0] = true;
            flags |= flag::con0;
            break;
        case con_1:
            connections[1] = true;
            flags |= flag::con1;
            break;
        case con_2:
            connections[2] = true;
            flags |= flag::con2;
            break;
        case con_3:
            connections[3] = true;
            flags |= flag::con3;
            break;

        case clo_0:
            connections[0] = false;
            flags &= ~flag::con0;
            break;
        case clo_1:
            connections[1] = false;
            flags &= ~flag::con1;
            break;
        case clo_2:
            connections[2] = false;
            flags &= ~flag::con2;
            break;
        case clo_3:
            connections[3] = false;
            flags &= ~flag::con3;
            break;
    }

    return true;
}

int AT::_timedRead(unsigned long timeout) {
    const auto end = millis() + timeout;
    int c;

    while (millis() < end) {
        if ((c = serial.read()) != -1)
            return c;
    }
    return -1;
}

bool AT::cmd_ok(const char *cmd, unsigned long timeout, bool echo) {
    serial.write(cmd);
    return wait_for_str("OK" EOL, timeout, echo);
}

bool AT::start_server(/*uint8_t port*/) {
    return connected &&
           cmd_ok("AT+CIPMUX=1" EOL, 100, false) &&
           cmd_ok("AT+CIPSERVER=1,80" EOL, 100, false);
}

bool AT::close_conn(uint8_t num) {
    if (!connections[num]) return true;
    static char cmd[] = "AT+CIPCLOSE=" "_" EOL;
    cmd[12] = '0' + num;
    connections[num] = cmd_ok(cmd, 100, false);
    return connections[num];
}

bool AT::init_send_data(uint8_t conn_num, size_t length) {
    if (!connections[conn_num]) return false;
    if (length >= 100000) return false;
    static char cmd[] = "AT+CIPSEND=" "_" "," "00000" EOL;

    cmd[11] = '0' + conn_num;
    char *end = cmd + 11 + 1 + 5;
    while (length > 0) {
        *end = length % 10 + '0';
        --end;
        length /= 10;
    }
    serial.write(cmd);


    return wait_for_str(">", 500, false); //|| (Serial.println("no >..."), false);
}

AT::wait_for_str_ret AT::wait_for_str(const char *str, unsigned long timeout, size_t max_chars, bool echo) {
    size_t index = 0;
    const size_t len = strlen(str);

    auto end = millis() + timeout;
    size_t read = 0;

    int ch;
    while (millis() < end && read < (max_chars - 1)) {
        if ((ch = serial.read()) != -1) {
            ++read;
            if (echo) Serial.write(ch);
            if (ch == str[index]) {
                ++index;
                if (index == len)
                    return {true, read};
            } else {
                if (ch != str[index]) index = 0;
                else index = 1;
            }
        } else {
            delay(10);
        }
    }
    return {false, read};
}
