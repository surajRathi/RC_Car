#ifndef CAR_THROTTLE_H
#define CAR_THROTTLE_H

namespace throttle {
    const int steer_pin_a = 8;
    const int steer_pin_b = 9;
    const int throttle_pin_a = 10;
    const int throttle_pin_b = 11;

    void main(int throttle);

    void steer(int throttle);
}
#endif //CAR_THROTTLE_H
