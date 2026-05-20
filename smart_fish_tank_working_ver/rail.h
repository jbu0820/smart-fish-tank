#ifndef RAIL_H
#define RAIL_H

#include <stdint.h>

#define FEED_DELAY_MS 3000

void relay_init(void);

void motor_forward(void);
void motor_reverse(void);
void motor_stop(void);

void rail_motion(void);
void feed_delay(void);

void rail_run(uint8_t cleanRequest, uint8_t feedRequest, uint8_t *done_sig);

#endif