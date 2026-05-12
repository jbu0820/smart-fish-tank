#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "rail.h"

int main(void) {
    uint8_t cleanRequest = 0;
    uint8_t feedRequest  = 0;
    uint8_t done_sig     = 0;

    relay_init();

    while (1) {
        // 예시 1: 먹이 주기 요청
        feedRequest = 1;

        rail_run(cleanRequest, feedRequest, &done_sig);

        if (done_sig == 1) {
            cleanRequest = 0;
            feedRequest  = 0;
        }

        _delay_ms(100);
    }
}
