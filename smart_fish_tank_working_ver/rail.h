#ifndef RAIL_H
#define RAIL_H

#include <stdint.h>

// 릴레이 연결 핀
#define RELAY_DDR DDRC
#define RELAY_PORT PORTC

#define RELAY1 PC0
#define RELAY2 PC1

// 먹이 후 대기 시간
#define FEED_DELAY_MS 5000   // 5초, 필요하면 10000으로 변경 가능
// #define FEED_DELAY_MS 3000 // 시연용 

void relay_init(void);
void motor_stop(void);
void motor_forward(void);
void motor_reverse(void);

void feed_delay(void);
void rail_motion(void);
void rail_run(uint8_t cleanRequest, uint8_t feedRequest, uint8_t *done_sig);

#endif