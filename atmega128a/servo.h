#ifndef SERVO_H_
#define SERVO_H_

#include <stdint.h>

#define SERVO_MIN 2000
#define SERVO_MAX 5000

void servo_init(void);
void servo_set(uint16_t value);
void feed_fish(void);

#endif
