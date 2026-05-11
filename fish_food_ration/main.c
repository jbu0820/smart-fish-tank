#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include "ds1302.h"
#include "servo.h"

RTC_Time now;

// RTC 연결 진단 모드입니다.
// 1이면 부팅 시 DS1302에 시간을 써보고 다시 읽어서 정상 연결이면 서보가 1회 동작합니다.
// 진단이 끝나면 반드시 0으로 되돌리세요.
#define ENABLE_RTC_CONNECTION_TEST 0

// 1로 바꾸면 부팅 시 DS1302 시간을 아래 SET_RTC_* 값으로 1회 설정합니다.
// 시간 설정 후에는 반드시 0으로 되돌려 다시 업로드하세요.
#define ENABLE_RTC_TIME_SET 0
#define SET_RTC_YEAR       26
#define SET_RTC_MONTH      5
#define SET_RTC_DATE       11
#define SET_RTC_HOUR       16
#define SET_RTC_MIN        11
#define SET_RTC_SEC        0

// DS1302 실제 시간이 아래 시간과 같으면 서보모터가 1회 동작합니다.
// 테스트 시간: 16:10
#define FEED_HOUR 16
#define FEED_MIN  14


// 시(hour), 분(min)을 함께 저장해서 같은 분 안에서 반복 동작하지 않도록 함
uint8_t last_fed_hour = 255;
uint8_t last_fed_min  = 255;

static void disable_jtag(void)
{
#if defined(JTD) && defined(MCUCSR)
    MCUCSR |= (1 << JTD);
    MCUCSR |= (1 << JTD);
#endif
}

int main(void)
{
    disable_jtag();

    ds1302_init();
    servo_init();
    servo_set(SERVO_MIN);

#if ENABLE_RTC_CONNECTION_TEST
    RTC_set_time(26, 5, 11, 13, 0, 0);
    _delay_ms(1200);
    RTC_read_time(&now);

    if (now.hour == 13 && now.min == 0 && now.sec >= 1 && now.sec <= 5)
    {
        feed_fish();
    }

    while (1)
    {
        _delay_ms(1000);
    }
#endif

#if ENABLE_RTC_TIME_SET
    RTC_set_time(SET_RTC_YEAR, SET_RTC_MONTH, SET_RTC_DATE,
                 SET_RTC_HOUR, SET_RTC_MIN, SET_RTC_SEC);
#endif

    _delay_ms(1000);

    while (1)
    {
        RTC_read_time(&now);

        // DS1302에서 읽은 실제 시간이 지정한 동작 시간인지 확인
        uint8_t is_feed_time =
            (now.hour == FEED_HOUR
             && now.min == FEED_MIN);

        if (is_feed_time)
        {
            // 같은 분 안에서 중복 실행 방지
            if (last_fed_hour != now.hour || last_fed_min != now.min)
            {
                feed_fish();                  // 먹이 배급 (약 8초 소요)
                last_fed_hour = now.hour;     // 중복 실행 방지용 시 저장
                last_fed_min  = now.min;      // 중복 실행 방지용 분 저장
            }
        }

        _delay_ms(500); // 0.5초 단위로 시간 확인
    }
}
