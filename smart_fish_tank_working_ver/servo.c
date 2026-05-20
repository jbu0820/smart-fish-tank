#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "servo.h"

void servo_init(void)
{
    // PB6 = OC1B
    DDRB |= (1 << PB6);

    // Timer1 Fast PWM, TOP = ICR1
    // OC1B non-inverting mode
    TCCR1A = (1 << COM1B1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);

    // 16MHz / 8 = 2MHz
    // 20ms = 40000 counts
    ICR1 = 39999;

    OCR1B = SERVO_MID;
}

void servo_set(uint16_t value)
{
    OCR1B = value;
}

void feed_fish(void)
{
    /* 0도 → 180도 → 0도 → 180도 → 0도 */

    servo_set(SERVO_MIN);   /* 0도 */
    _delay_ms(1000);

    servo_set(SERVO_MAX);   /* 180도 */
    _delay_ms(1000);

    servo_set(SERVO_MIN);   /* 0도 */
    _delay_ms(1000);

    servo_set(SERVO_MAX);   /* 180도 */
    _delay_ms(1000);

    servo_set(SERVO_MIN);   /* 0도 (마지막 복귀) */
    _delay_ms(1000);
}