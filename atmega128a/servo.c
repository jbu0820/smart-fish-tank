#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "servo.h"

void servo_init(void)
{
    DDRB |= (1 << PB5);

    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);

    ICR1 = 39999;
    OCR1A = SERVO_MIN;
}

void servo_set(uint16_t value)
{
    OCR1A = value;
}

void feed_fish(void)
{
    for (uint8_t i = 0; i < 2; i++)
    {
        servo_set(SERVO_MAX);
        _delay_ms(3000);

        servo_set(SERVO_MIN);
        _delay_ms(1000);
    }
}
