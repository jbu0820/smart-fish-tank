#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "rail.h"

/*
 * Relay pin
 * Relay1 IN -> PC0
 * Relay2 IN -> PC1
 */
#define RELAY1_PIN PC0
#define RELAY2_PIN PC1

#define RELAY_DDR  DDRC
#define RELAY_PORT PORTC

/*
 * Active-Low Relay
 * LOW  = ON
 * HIGH = OFF
 */
static void relay1_on(void)
{
    RELAY_PORT &= ~(1 << RELAY1_PIN);
}

static void relay1_off(void)
{
    RELAY_PORT |= (1 << RELAY1_PIN);
}

static void relay2_on(void)
{
    RELAY_PORT &= ~(1 << RELAY2_PIN);
}

static void relay2_off(void)
{
    RELAY_PORT |= (1 << RELAY2_PIN);
}

void relay_init(void)
{
    /*
     * active-low 릴레이는 LOW가 ON이므로
     * 출력 설정 전에 먼저 HIGH로 만들어 OFF 상태를 준비한다.
     */
    RELAY_PORT |= (1 << RELAY1_PIN) | (1 << RELAY2_PIN);

    RELAY_DDR |= (1 << RELAY1_PIN) | (1 << RELAY2_PIN);

    motor_stop();
}

void motor_forward(void)
{
    /*
     * Relay1 ON + Relay2 ON = 정방향
     */
    relay1_on();
    relay2_on();
}

void motor_reverse(void)
{
    /*
     * Relay1 OFF + Relay2 OFF = 역방향
     */
    relay1_off();
    relay2_off();
}

void motor_stop(void)
{
    /*
     * 한쪽 ON, 한쪽 OFF = 정지
     * 여기서는 Relay1 ON, Relay2 OFF 사용
     */
    relay1_on();
    relay2_off();
}

void rail_motion(void)
{
    motor_forward();
    _delay_ms(3000);

    motor_stop();
    _delay_ms(1000);

    motor_reverse();
    _delay_ms(3000);

    motor_stop();
    _delay_ms(1000);
}

void feed_delay(void)
{
    _delay_ms(FEED_DELAY_MS);
}

void rail_run(uint8_t cleanRequest, uint8_t feedRequest, uint8_t *done_sig)
{
    if (done_sig == 0)
    {
        return;
    }

    *done_sig = 0;

    if ((cleanRequest == 0) && (feedRequest == 0))
    {
        motor_stop();
        return;
    }

    if (feedRequest == 1)
    {
        feed_delay();
        rail_motion();
    }
    else if (cleanRequest == 1)
    {
        rail_motion();
    }

    motor_stop();
    *done_sig = 1;
}