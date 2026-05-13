#ifndef LED_H_
#define LED_H_

#include <avr/io.h>

// ATmega128 PORTD 사용 기준 (0:Red, 1:Green, 2:Blue)
#define LED_DDR   DDRD
#define LED_PORT  PORTD
#define PIN_RED   PD2
#define PIN_GREEN PD3
#define PIN_BLUE  PD4

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