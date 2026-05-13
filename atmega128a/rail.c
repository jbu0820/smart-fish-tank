#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include "rail.h"

void relay_init(void) {
    RELAY_DDR |= (1 << RELAY1) | (1 << RELAY2);

    // 기본 OFF
    RELAY_PORT |= (1 << RELAY1);
    RELAY_PORT |= (1 << RELAY2);
}

void motor_stop(void) {
    RELAY_PORT &= ~(1 << RELAY1);
    RELAY_PORT &= ~(1 << RELAY2);
}

void motor_forward(void) {
    RELAY_PORT |= (1 << RELAY1);
    RELAY_PORT &= ~(1 << RELAY2);
}

void motor_reverse(void) {
    RELAY_PORT &= ~(1 << RELAY1);
    RELAY_PORT |= (1 << RELAY2);
}

// ---------------------------------
// 먹이 후 대기 함수
// ---------------------------------
void feed_delay(void) {
    _delay_ms(FEED_DELAY_MS);
}

// ---------------------------------
// 실제 레일 왕복 동작 함수
// ---------------------------------
void rail_motion(void) {
    motor_forward();
    _delay_ms(500);

    motor_stop();
    _delay_ms(500);

    motor_reverse();
    _delay_ms(1000);

    motor_stop();
    _delay_ms(500);
}

// ---------------------------------
// 상위 요청 처리 함수
// cleanRequest == 1 -> 바로 rail_motion()
// feedRequest  == 1 -> feed_delay() 후 rail_motion()
// done_sig 반환
// ---------------------------------
void rail_run(uint8_t cleanRequest, uint8_t feedRequest, uint8_t *done_sig) {
    if (done_sig == 0) {
        return;
    }

    *done_sig = 0;

    if ((cleanRequest == 0) && (feedRequest == 0)) {
        return;
    }

    if (feedRequest == 1) {
        feed_delay();
        rail_motion();
    }
    else if (cleanRequest == 1) {
        rail_motion();
    }

    *done_sig = 1;
}