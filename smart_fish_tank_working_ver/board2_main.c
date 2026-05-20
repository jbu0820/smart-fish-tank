// for board 2
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "servo.h"

/*
 * ============================================================
 * 보드2 (서보모터 전용)
 * ============================================================
 *
 * 핀 배치:
 *   PB6 : 서보모터 PWM (OC1B)
 *   PC2 : feed_cmd  입력 ← 보드1
 *   PC3 : feed_done 출력 → 보드1
 *
 * 전원:
 *   서보모터 전용 전원 사용 (보드1과 전원 분리)
 *   GND는 반드시 보드1과 공통 연결
 * ============================================================
 */

/* ============================================================
 * GPIO 통신 핀
 * PC2 : feed_cmd  입력 ← 보드1
 * PC3 : feed_done 출력 → 보드1
 * ============================================================ */
#define FEED_CMD_DDR    DDRC
#define FEED_CMD_PINR   PINC
#define FEED_CMD_PIN    PC2

#define FEED_DONE_DDR   DDRC
#define FEED_DONE_PORT  PORTC
#define FEED_DONE_PIN   PC3

/* ============================================================
 * GPIO 초기화 / 함수
 * ============================================================ */
static void gpio_feed_slave_init(void)
{
    /* PC2 : feed_cmd 입력 */
    FEED_CMD_DDR  &= ~(1 << FEED_CMD_PIN);

    /* PC3 : feed_done 출력, 초기 LOW */
    FEED_DONE_DDR  |=  (1 << FEED_DONE_PIN);
    FEED_DONE_PORT &= ~(1 << FEED_DONE_PIN);
}

static uint8_t feed_command_received(void)
{
    return (FEED_CMD_PINR & (1 << FEED_CMD_PIN)) ? 1 : 0;
}

static void feed_done_set(void)
{
    FEED_DONE_PORT |= (1 << FEED_DONE_PIN);    /* HIGH → 보드1 */
}

static void feed_done_clear(void)
{
    FEED_DONE_PORT &= ~(1 << FEED_DONE_PIN);   /* LOW */
}

/* ============================================================
 * 메인
 * ============================================================ */
int main(void)
{
    servo_init();
    gpio_feed_slave_init();

    servo_set(SERVO_MIN);      /* 초기 위치 */
    _delay_ms(1000);

    while (1)
    {
        /* 보드1 feed_cmd(PC2) HIGH 대기 */
        if (feed_command_received())
        {
            /* 먹이 배급 동작 */
            feed_fish();

            /* 완료 신호 → 보드1 PC3 HIGH */
            feed_done_set();

            /* 보드1이 feed_cmd 내릴 때까지 대기 (중복 동작 방지) */
            while (feed_command_received())
            {
                _delay_ms(10);
            }

            /* 명령 해제되면 done 신호도 내림 */
            feed_done_clear();

            servo_set(SERVO_MIN);  /* 초기 위치 복귀 */
            _delay_ms(300);
        }

        _delay_ms(10);
    }
}
