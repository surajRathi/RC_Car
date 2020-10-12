#include <Arduino.h>

#include "stop.h"
#include "throttle.h"

const double rate = 20/*hz*/;
const uint16_t interval = static_cast<uint16_t>(1000 / rate);

const int steer_pin_pot = A0;
uint16_t POT_MIN = 1023, POT_MAX = 0, target;

int16_t steer_p(uint16_t pot, uint16_t target) {
    // const auto err = target - pot;
    // return ((err > 0) - (err < 0)) * sqrt(abs(err)) * 5;
    return (target - pot) * 5;
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.begin(9600);
    while (!Serial);

    Serial.println("\n\nStarting...");

    // Init steering pot
    throttle::steer(200);
    delay(1000);
    POT_MAX = analogRead(steer_pin_pot);
    Serial.println(POT_MAX);
    throttle::steer(-200);
    delay(1000);
    POT_MIN = analogRead(steer_pin_pot);
    Serial.println(POT_MIN);
    throttle::steer(0);
    target = (POT_MIN + POT_MAX) / 2;

    if (POT_MIN >= POT_MAX) {
        Serial.write("Swap POT +5 and GND");
        Serial.flush();
        // delay(100); // Let Serial messages finish.
        stop();
    }

    Serial.print("Main loop aiming for ");
    Serial.print(interval);
    Serial.println("ms per loop.");
}

void loop() {
    const auto timestamp = millis();

    if (Serial.available()) {
        auto val = Serial.parseInt();
        if (val == -42) {
            Serial.println("Stopping...");
            delay(100);
            Serial.end();
            stop();
        }
        target = map(val, 0, 100, POT_MIN, POT_MAX);
    }

    auto pot = analogRead(steer_pin_pot);
    auto steer_throttle_val = steer_p(pot, target);
    throttle::steer(steer_throttle_val);
    Serial.print('\r');
    Serial.print(target);
    Serial.print(" ");
    Serial.print(pot);
    Serial.print(" ");
    Serial.print(steer_throttle_val);
    Serial.print("                    ");
    // Serial.println(" ");

    auto elapsed = millis() - timestamp;
    if (elapsed > interval) {
        Serial.print("Warning overshot loop time (ms): ");
        Serial.print(elapsed);
        Serial.print(" instead of ");
        Serial.println(interval);
    } else {
        delay(interval - elapsed);
    }

    // if (elapsed < interval)
    //    delay(interval - elapsed);
}
