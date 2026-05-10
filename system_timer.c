#include <avr/io.h>
#include <avr/interrupt.h>

#include "system_timer.h"
#include "turbidity.h"

void SystemTimer_Init(void)
{
    /*
     * Timer0 CTC 모드
     */
    TCCR0 = (1 << WGM01);

    /*
     * 분주비 64
     * 16MHz / 64 = 250kHz
     */
    TCCR0 |= (1 << CS01) | (1 << CS00);

    /*
     * 1ms 주기
     * 250 tick * 4us = 1ms
     */
    OCR0 = 249;

    /*
     * Timer0 Compare Match Interrupt Enable
     */
    TIMSK |= (1 << OCIE0);
}

ISR(TIMER0_COMP_vect)
{
    Turbidity_TimerTick1ms();
}