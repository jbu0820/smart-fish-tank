#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include "ds1302.h"
#include "servo.h"

RTC_Time now;

uint8_t last_minute = 255;

int main(void)
{
    ds1302_init();

    servo_init();

    servo_set(SERVO_MIN);

    _delay_ms(1000);

    while (1)
    {
        RTC_read_time(&now);

        // 분 변경 감지
        if(now.min != last_minute)
        {
            last_minute = now.min;

            // 1분마다 실행
            feed_fish();
        }

        _delay_ms(500);
    }
}

