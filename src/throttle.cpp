#include <Arduino.h>
#include "throttle.h"

using namespace throttle;

/**
 * Sends main command to rear motor
 *
 * @param throttle +ive is forwards.
 *        between -255, 255 # TODO: Change the argument type
 */
void throttle::main(int throttle) {
    throttle = -throttle; // Motor is inverted

    throttle = max(-255, min(throttle, 255));
    if (abs(throttle) < 40) throttle = 0;

    analogWrite(throttle_pin_a, max(0, throttle));
    analogWrite(throttle_pin_b, max(0, -throttle));
}

/**
 * Sets power of steering motor
 *
 * @param throttle
 *        between -255, 255 # TODO: Change the argument type
 */
void throttle::steer(int throttle) {
    throttle = -throttle;
    throttle = max(-255, min(throttle, 255));
    if (abs(throttle) < 40) throttle = 0;

    analogWrite(steer_pin_a, max(0, throttle));
    analogWrite(steer_pin_b, max(0, -throttle));

    /*if (abs(throttle) < 40) {
        analogWrite(steer_pin_a, 255);
        analogWrite(steer_pin_b, 255);
    } else {
        analogWrite(steer_pin_a, max(0, throttle));
        analogWrite(steer_pin_b, max(0, -throttle));
    }*/
}