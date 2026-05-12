#include <stdint.h>

#include "ds1302.h"
#include "servo.h"
#include "feeder.h"
#include "system.h"

// 테스트용 먹이 배급 시간
#define FEED_HOUR_MORNING    9
#define FEED_HOUR_AFTERNOON  14
#define FEED_MIN             0

static RTC_Time now;

static uint8_t last_fed_hour = 255;
static uint8_t last_fed_min  = 255;

void feeder_init(void)
{
    last_fed_hour = 255;
    last_fed_min  = 255;
}
uint8_t feeder_update(void)
{
    RTC_read_time(&now);

    uint8_t is_feed_time =
        ((now.hour == FEED_HOUR_MORNING || now.hour == FEED_HOUR_AFTERNOON)
         && now.min == FEED_MIN);

    if (is_feed_time)
    {
        if (last_fed_hour != now.hour || last_fed_min != now.min)
        {
            feed_fish();

            last_fed_hour = now.hour;
            last_fed_min  = now.min;

            return 1;   // 먹이 줬음 -> feedRequest 역할
        }
    }

    return 0;           // 먹이 안 줌
}