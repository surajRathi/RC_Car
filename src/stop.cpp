#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <Arduino.h>
#include "stop.h"

[[noreturn]] void stop() {
    Serial.end();
    noInterrupts();
    wdt_disable();
/*
    DDRA = 0;
    DDRB = 0;
    DDRC = 0;
    DDRD = 0;
    DDRE = 0;
    DDRF = 0;
*/
    /* Set analog pins LOW */ {
        pinMode(A0, OUTPUT);
        pinMode(A1, OUTPUT);
        pinMode(A2, OUTPUT);
        pinMode(A3, OUTPUT);
        pinMode(A4, OUTPUT);
        pinMode(A5, OUTPUT);
        pinMode(A6, OUTPUT);
        pinMode(A7, OUTPUT);
        pinMode(A8, OUTPUT);
        pinMode(A9, OUTPUT);
        pinMode(A10, OUTPUT);
        pinMode(A11, OUTPUT);
        pinMode(A12, OUTPUT);
        pinMode(A13, OUTPUT);
        pinMode(A14, OUTPUT);
        pinMode(A15, OUTPUT);

        digitalWrite(A0, LOW);
        digitalWrite(A1, LOW);
        digitalWrite(A2, LOW);
        digitalWrite(A3, LOW);
        digitalWrite(A4, LOW);
        digitalWrite(A5, LOW);
        digitalWrite(A6, LOW);
        digitalWrite(A7, LOW);
        digitalWrite(A8, LOW);
        digitalWrite(A9, LOW);
        digitalWrite(A10, LOW);
        digitalWrite(A11, LOW);
        digitalWrite(A12, LOW);
        digitalWrite(A13, LOW);
        digitalWrite(A14, LOW);
        digitalWrite(A15, LOW);
    }

    // Set digital pins LOW
    for (int i = 0; i <= 53; i++) {
        pinMode(i, INPUT);
        // pinMode(i, OUTPUT);
        digitalWrite(i, LOW);
        // digitalWrite(i, LOW);
    }

    while (true) {
        noInterrupts();
        wdt_disable();
        power_all_disable();
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    }
}