#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include "ds1302.h"

#define DS1302_PORT PORTC
#define DS1302_DDR  DDRC
#define DS1302_PIN  PINC

#define DS1302_CLK  PC0
#define DS1302_IO   PC1
#define DS1302_RST  PC2

static uint8_t dec_to_bcd(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

static uint8_t bcd_to_dec(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static uint8_t decode_hour(uint8_t hour_reg)
{
    if (hour_reg & 0x80)
    {
        uint8_t hour = bcd_to_dec(hour_reg & 0x1F);
        uint8_t is_pm = hour_reg & 0x20;

        if (hour == 12)
            return is_pm ? 12 : 0;

        return is_pm ? hour + 12 : hour;
    }

    return bcd_to_dec(hour_reg & 0x3F);
}

void ds1302_init(void)
{
    DS1302_DDR |= (1 << DS1302_CLK) | (1 << DS1302_RST);

    DS1302_PORT &= ~(1 << DS1302_CLK);
    DS1302_PORT &= ~(1 << DS1302_RST);
}

static void ds1302_write_byte(uint8_t data)
{
    DS1302_DDR |= (1 << DS1302_IO);

    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x01)
            DS1302_PORT |=  (1 << DS1302_IO);
        else
            DS1302_PORT &= ~(1 << DS1302_IO);

        DS1302_PORT |= (1 << DS1302_CLK);
        _delay_us(2);

        DS1302_PORT &= ~(1 << DS1302_CLK);
        _delay_us(2);

        data >>= 1;
    }
}

static uint8_t ds1302_read_byte(void)
{
    uint8_t data = 0;

    // 버그3 수정: 내부 풀업 먼저 해제 후 입력 전환
    DS1302_PORT &= ~(1 << DS1302_IO);
    DS1302_DDR  &= ~(1 << DS1302_IO);

    // 버그4 수정: DS1302 첫 비트 출력 안정화 대기
    _delay_us(2);

    for (uint8_t i = 0; i < 8; i++)
    {
        if (DS1302_PIN & (1 << DS1302_IO))
            data |= (1 << i);

        DS1302_PORT |= (1 << DS1302_CLK);
        _delay_us(2);

        DS1302_PORT &= ~(1 << DS1302_CLK);
        _delay_us(2);
    }

    return data;
}

static void ds1302_write(uint8_t addr, uint8_t data)
{
    DS1302_PORT &= ~(1 << DS1302_CLK);
    DS1302_PORT |=  (1 << DS1302_RST);

    ds1302_write_byte(addr);
    ds1302_write_byte(data);

    DS1302_PORT &= ~(1 << DS1302_RST);
}

static uint8_t ds1302_read(uint8_t addr)
{
    uint8_t data;

    DS1302_PORT &= ~(1 << DS1302_CLK);
    DS1302_PORT |=  (1 << DS1302_RST);

    ds1302_write_byte(addr | 0x01);
    data = ds1302_read_byte();

    DS1302_PORT &= ~(1 << DS1302_RST);

    return data;
}

void RTC_set_test_time(void)
{
    RTC_set_time(26, 5, 11, 11, 59, 50);
}

void RTC_set_time(uint8_t year, uint8_t month, uint8_t date,
                  uint8_t hour, uint8_t min, uint8_t sec)
{
    ds1302_write(0x8E, 0x00);

    ds1302_write(0x80, dec_to_bcd(sec) & 0x7F); // 초, CH 비트 0으로 클럭 동작
    ds1302_write(0x82, dec_to_bcd(min));        // 분
    ds1302_write(0x84, dec_to_bcd(hour));       // 시, 24시간제

    ds1302_write(0x86, dec_to_bcd(date));  // 일
    ds1302_write(0x88, dec_to_bcd(month)); // 월
    ds1302_write(0x8A, dec_to_bcd(1));  // 요일
    ds1302_write(0x8C, dec_to_bcd(year)); // 년

    ds1302_write(0x8E, 0x80);
}

void RTC_read_time(RTC_Time *t)
{
    t->sec   = bcd_to_dec(ds1302_read(0x80) & 0x7F);
    t->min   = bcd_to_dec(ds1302_read(0x82));
    t->hour  = decode_hour(ds1302_read(0x84));
    t->date  = bcd_to_dec(ds1302_read(0x86));
    t->month = bcd_to_dec(ds1302_read(0x88));
    t->year  = bcd_to_dec(ds1302_read(0x8C));
}
