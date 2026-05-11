#ifndef LED_H_
#define LED_H_

#include <avr/io.h>

// ATmega128 PORTB 사용 기준 (0:Red, 1:Green, 2:Blue)
#define LED_DDR   DDRB
#define LED_PORT  PORTB
#define PIN_RED   PB0
#define PIN_GREEN PB1
#define PIN_BLUE  PB2

// 시스템 상태 정의 (main.c와 일치됨)
typedef enum {
    STATUS_GOOD,    // 초록색
    STATUS_CHECK,   // 노란색
    STATUS_WARNING  // 빨간색
} SystemStatus;

void LED_Init(void);
void LED_SetStatus(SystemStatus status);
void LED_AllOff(void);

#endif