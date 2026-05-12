#ifndef DS1302_H_
#define DS1302_H_

#include <stdint.h>

typedef struct
{
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t date;
    uint8_t month;
    uint8_t year;
} RTC_Time;

void ds1302_init(void);
void RTC_set_test_time(void);
void RTC_set_time(uint8_t year, uint8_t month, uint8_t date,
                  uint8_t hour, uint8_t min, uint8_t sec);
void RTC_read_time(RTC_Time *t);

#endif
