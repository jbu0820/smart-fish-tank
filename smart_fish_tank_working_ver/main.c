// for board 1
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "rail.h"
#include "turbidity.h"
#include "i2c_lcd.h"
#include "rgb_led.h"

/*
 * ============================================================
 * 보드1 (메인 컨트롤러)
 * ============================================================
 *
 * 핀 배치:
 *   PC0       : Relay1
 *   PC1       : Relay2
 *   PC2       : feed_cmd  출력 → 보드2
 *   PC3       : feed_done 입력 ← 보드2
 *   PD0/PD1   : I2C LCD SCL/SDA
 *   PD2       : RGB LED R
 *   PD3       : RGB LED G
 *   PD4       : RGB LED B
 *   PF0       : 조도센서 ADC0 (turbidity.c 내부)
 *   PF1~PF3   : 탁도 측정 LED (turbidity.c 내부)
 * ============================================================
 */

/* ============================================================
 * GPIO 통신 핀
 * PC2 : feed_cmd  출력 → 보드2
 * PC3 : feed_done 입력 ← 보드2
 * ============================================================ */
#define FEED_CMD_DDR    DDRC
#define FEED_CMD_PORT   PORTC
#define FEED_CMD_PIN    PC2

#define FEED_DONE_DDR   DDRC
#define FEED_DONE_PINR  PINC
#define FEED_DONE_PIN   PC3

/* ============================================================
 * 시연 설정
 * FORCE_FEED_DEMO  = 1 : RTC 무시, cleanRequest 처리 후 바로 feed 강제
 * FEED_TO_RAIL_WAIT_MS : feed 완료 후 레일 대기 시간 (LCD 표시용)
 * ============================================================ */
#define FORCE_FEED_DEMO       1
#define FEED_TO_RAIL_WAIT_MS  2000

/* ============================================================
 * 시스템 FSM 상태
 * ============================================================ */
typedef enum
{
    SYS_STATE_MONITOR = 0,
    SYS_STATE_CLEAN_CAUSE,
    SYS_STATE_CLEAN_RAIL,
    SYS_STATE_CLEAN_DONE,
    SYS_STATE_FEED_START,
    SYS_STATE_FEED_RUNNING,
    SYS_STATE_FEED_WAIT,
    SYS_STATE_FEED_CAUSE,
    SYS_STATE_FEED_RAIL,
    SYS_STATE_FEED_DONE
} SystemState;

static SystemState sysState     = SYS_STATE_MONITOR;
static uint8_t     cleanRequest = 0;
static uint8_t     feedRequest  = 0;
static uint8_t     done_sig     = 0;

/* ============================================================
 * GPIO 통신 함수
 * ============================================================ */
static void gpio_feed_master_init(void)
{
    /* PC2 : feed_cmd 출력, 초기 LOW */
    FEED_CMD_DDR  |=  (1 << FEED_CMD_PIN);
    FEED_CMD_PORT &= ~(1 << FEED_CMD_PIN);

    /* PC3 : feed_done 입력 */
    FEED_DONE_DDR &= ~(1 << FEED_DONE_PIN);
}

static void feed_start_command(void)
{
    FEED_CMD_PORT |= (1 << FEED_CMD_PIN);    /* HIGH → 보드2 feed 시작 */
}

static void feed_clear_command(void)
{
    FEED_CMD_PORT &= ~(1 << FEED_CMD_PIN);   /* LOW */
}

static uint8_t feed_done_received(void)
{
    return (FEED_DONE_PINR & (1 << FEED_DONE_PIN)) ? 1 : 0;
}

/* ============================================================
 * 탁도 FSM 완료까지 대기 후 데이터 읽기
 * Turbidity_Update()는 1회 호출로 상태 하나씩만 전이되므로
 * IDLE 상태로 복귀할 때까지 반복 호출해야 함
 * ============================================================ */
static void turbidity_measure_and_get(TurbidityData *data)
{
    /* 측정 요청 */
    Turbidity_RequestMeasure();

    /* IDLE로 복귀할 때까지 FSM 반복 구동 */
    while (Turbidity_GetFsmState() != TURBIDITY_STATE_IDLE)
    {
        Turbidity_Update();
    }

    /* 마지막 IDLE 상태에서 1회 더 호출하여 측정 시작 */
    Turbidity_Update();

    /* 다시 IDLE로 복귀할 때까지 대기 */
    while (Turbidity_GetFsmState() != TURBIDITY_STATE_IDLE)
    {
        Turbidity_Update();
    }

    Turbidity_GetData(data);
}

/* ============================================================
 * LCD 출력
 * ============================================================ */
static void lcd_print_2line(const char *line1, const char *line2)
{
    LCD_Clear();
    _delay_ms(5);
    LCD_SetCursor(0, 0);
    LCD_WriteString((char *)line1);
    LCD_SetCursor(1, 0);
    LCD_WriteString((char *)line2);
}

static void lcd_show_monitor(WaterState state, uint16_t diff)
{
    char line2[17];
    snprintf(line2, sizeof(line2), "DIFF:%3u", diff);

    if (state == WATER_STATE_GOOD)
        lcd_print_2line("Water: CLEAN", line2);
    else if (state == WATER_STATE_NORMAL)
        lcd_print_2line("Water: CLOUDY", line2);
    else
        lcd_print_2line("!! WARNING !!", line2);
}

/* ============================================================
 * LED 상태 표시
 * ============================================================ */
static void led_show_water_state(WaterState state)
{
    if (state == WATER_STATE_GOOD)
        LED_SetStatus(STATUS_GOOD);      /* 초록 */
    else if (state == WATER_STATE_NORMAL)
        LED_SetStatus(STATUS_CHECK);     /* 노란 */
    else
        LED_SetStatus(STATUS_WARNING);   /* 빨간 */
}

/* ============================================================
 * 메인
 * ============================================================ */
int main(void)
{
    TurbidityData turbidityData;
    WaterState    lastState = (WaterState)255;
    uint16_t      lastDiff  = 65535;

    _delay_ms(500);

    /*
     * relay_init 먼저 호출하여 PC0/PC1 설정 후
     * gpio_feed_master_init 으로 PC2/PC3 설정
     * 순서가 바뀌면 relay_init 이 PC2/PC3 을 덮어씌울 수 있음
     */
    relay_init();
    gpio_feed_master_init();

    I2C_Init();
    _delay_ms(100);
    LCD_Init();

    LED_Init();
    Turbidity_Init();

    lcd_print_2line("SMART AQUARIUM", "SYSTEM START");
    _delay_ms(1500);

    while (1)
    {
        switch (sysState)
        {
            /* ------------------------------------------------
             * 평상시 모니터링
             * 실제 센서로 탁도 측정
             * 시연: 손으로 센서 가려서 CLOUDY → BAD 연출
             * ------------------------------------------------ */
            case SYS_STATE_MONITOR:
            {
                turbidity_measure_and_get(&turbidityData);
                cleanRequest = (uint8_t)turbidityData.cleanRequest;

                if ((turbidityData.waterState != lastState) ||
                    (turbidityData.turbidityValue != lastDiff))
                {
                    lcd_show_monitor(turbidityData.waterState,
                                     turbidityData.turbidityValue);
                    led_show_water_state(turbidityData.waterState);
                    lastState = turbidityData.waterState;
                    lastDiff  = turbidityData.turbidityValue;
                }

                if (cleanRequest == 1)
                {
                    sysState = SYS_STATE_CLEAN_CAUSE;
                }
#if FORCE_FEED_DEMO
                else
                {
                    _delay_ms(500);
                    feedRequest = 1;
                    sysState    = SYS_STATE_FEED_START;
                }
#endif
                break;
            }

            /* ------------------------------------------------
             * cleanRequest → 레일 원인 표시
             * ------------------------------------------------ */
            case SYS_STATE_CLEAN_CAUSE:
            {
                lcd_print_2line("Rail Cause:", "Water Cleaning");
                _delay_ms(1500);
                sysState = SYS_STATE_CLEAN_RAIL;
                break;
            }

            /* ------------------------------------------------
             * 레일 구동 (cleanRequest)
             * ------------------------------------------------ */
            case SYS_STATE_CLEAN_RAIL:
            {
                done_sig = 0;
                rail_run(1, 0, &done_sig);
                if (done_sig == 1)
                    sysState = SYS_STATE_CLEAN_DONE;
                break;
            }

            /* ------------------------------------------------
             * 레일 완료 → 모니터링 복귀
             * ------------------------------------------------ */
            case SYS_STATE_CLEAN_DONE:
            {
                lcd_print_2line("Rail Done", "Cause: Clean");
                LED_SetStatus(STATUS_GOOD);
                _delay_ms(1500);

                cleanRequest = 0;
                done_sig     = 0;
                lastState    = (WaterState)255;  /* 복귀 후 LCD 강제 갱신 */
                sysState     = SYS_STATE_MONITOR;
                break;
            }

            /* ------------------------------------------------
             * 먹이 배급 시작 → 보드2 PC2 HIGH
             * ------------------------------------------------ */
            case SYS_STATE_FEED_START:
            {
                lcd_print_2line("Feed Start", "Command Sent");
                LED_SetStatus(STATUS_CHECK);
                _delay_ms(1000);

                feed_start_command();
                sysState = SYS_STATE_FEED_RUNNING;
                break;
            }

            /* ------------------------------------------------
             * 보드2 완료 신호(PC3 HIGH) 대기
             * ------------------------------------------------ */
            case SYS_STATE_FEED_RUNNING:
            {
                lcd_print_2line("Feed Running", "System Operate");

                if (feed_done_received())
                {
                    feed_clear_command();
                    sysState = SYS_STATE_FEED_WAIT;
                }
                else
                {
                    _delay_ms(100);
                }
                break;
            }

            /* ------------------------------------------------
             * feed 완료 후 대기
             * ------------------------------------------------ */
            case SYS_STATE_FEED_WAIT:
            {
                lcd_print_2line("Feed Request ON", "Rail Waiting...");
                _delay_ms(FEED_TO_RAIL_WAIT_MS);
                sysState = SYS_STATE_FEED_CAUSE;
                break;
            }

            /* ------------------------------------------------
             * feedRequest → 레일 원인 표시
             * ------------------------------------------------ */
            case SYS_STATE_FEED_CAUSE:
            {
                lcd_print_2line("Rail Cause:", "After Feed");
                _delay_ms(1500);
                sysState = SYS_STATE_FEED_RAIL;
                break;
            }

            /* ------------------------------------------------
             * 레일 구동 (feedRequest)
             * rail_run 내부에서 FEED_DELAY_MS 대기 후 동작
             * ------------------------------------------------ */
            case SYS_STATE_FEED_RAIL:
            {
                done_sig = 0;
                rail_run(0, 1, &done_sig);
                if (done_sig == 1)
                    sysState = SYS_STATE_FEED_DONE;
                break;
            }

            /* ------------------------------------------------
             * 먹이 후 레일 완료 → 모니터링 복귀
             * ------------------------------------------------ */
            case SYS_STATE_FEED_DONE:
            {
                lcd_print_2line("Rail Done", "Cause: Feed");
                LED_SetStatus(STATUS_GOOD);
                _delay_ms(1500);

                feedRequest = 0;
                done_sig    = 0;
                lastState   = (WaterState)255;  /* 복귀 후 LCD 강제 갱신 */
                sysState    = SYS_STATE_MONITOR;
                break;
            }

            default:
            {
                sysState = SYS_STATE_MONITOR;
                break;
            }
        }
    }
}
