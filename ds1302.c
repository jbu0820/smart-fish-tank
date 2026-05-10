#define F_CPU 16000000UL

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
            DS1302_PORT |= (1 << DS1302_IO);
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

    DS1302_DDR &= ~(1 << DS1302_IO);

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
    DS1302_PORT |= (1 << DS1302_RST);

    ds1302_write_byte(addr);
    ds1302_write_byte(data);

    DS1302_PORT &= ~(1 << DS1302_RST);
}

static uint8_t ds1302_read(uint8_t addr)
{
    uint8_t data;

    DS1302_PORT &= ~(1 << DS1302_CLK);
    DS1302_PORT |= (1 << DS1302_RST);

    ds1302_write_byte(addr | 0x01);
    data = ds1302_read_byte();

    DS1302_PORT &= ~(1 << DS1302_RST);

    return data;
}

void RTC_set_test_time(void)
{
    ds1302_write(0x8E, 0x00);

    ds1302_write(0x80, dec_to_bcd(50)); // 초
    ds1302_write(0x82, dec_to_bcd(24)); // 분
    ds1302_write(0x84, dec_to_bcd(17)); // 시

    ds1302_write(0x86, dec_to_bcd(9));  // 일
    ds1302_write(0x88, dec_to_bcd(5));  // 월
    ds1302_write(0x8A, dec_to_bcd(6));  // 요일
    ds1302_write(0x8C, dec_to_bcd(26)); // 년

    ds1302_write(0x8E, 0x80);
}

void RTC_read_time(RTC_Time *t)
{
    t->sec   = bcd_to_dec(ds1302_read(0x80) & 0x7F);
    t->min   = bcd_to_dec(ds1302_read(0x82));
    t->hour  = bcd_to_dec(ds1302_read(0x84) & 0x3F);
    t->date  = bcd_to_dec(ds1302_read(0x86));
    t->month = bcd_to_dec(ds1302_read(0x88));
    t->year  = bcd_to_dec(ds1302_read(0x8C));
}