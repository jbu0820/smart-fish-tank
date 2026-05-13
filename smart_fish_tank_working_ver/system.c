#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>

#include "ds1302.h"
#include "servo.h"
#include "feeder.h"
#include "system.h"

// RTC 시간 설정 모드
// 1로 바꾸면 아래 시간으로 RTC를 1회 설정함
// 설정 후에는 반드시 0으로 되돌리고 다시 업로드
#define ENABLE_RTC_TIME_SET 0

#define SET_RTC_YEAR       26
#define SET_RTC_MONTH      5
#define SET_RTC_DATE       13
#define SET_RTC_HOUR       1
#define SET_RTC_MIN        07
#define SET_RTC_SEC        0

static void disable_jtag(void)
{
#if defined(JTD) && defined(MCUCSR)
    MCUCSR |= (1 << JTD);
    MCUCSR |= (1 << JTD);
#endif
}

void system_init(void)
{
    disable_jtag();

    ds1302_init();

    servo_init();
    servo_set(SERVO_MIN);

    feeder_init();

#if ENABLE_RTC_TIME_SET
    RTC_set_time(SET_RTC_YEAR, SET_RTC_MONTH, SET_RTC_DATE,
                 SET_RTC_HOUR, SET_RTC_MIN, SET_RTC_SEC);
#endif

    _delay_ms(1000);
}
