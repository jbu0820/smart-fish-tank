// for board 1 ( main controller )
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "ds1302.h"
#include "rail.h"
#include "turbidity.h"
#include "i2c_lcd.h"
#include "rgb_led.h"

/* =========================
   GPIO 통신 (Board1 <-> Board2)
   Board1:
   PA0 -> feed_cmd 출력
   PA1 <- feed_done 입력
   ========================= */
#define FEED_CMD_DDR   DDRA
#define FEED_CMD_PORT  PORTA
#define FEED_CMD_PIN   PA0

#define FEED_DONE_DDR  DDRA
#define FEED_DONE_PINR PINA
#define FEED_DONE_PIN  PA1

/* =========================
   Feed 시간 설정
   ========================= */
#define FEED_HOUR_MORNING    9
#define FEED_HOUR_AFTERNOON  14
#define FEED_MIN             0

/* 시연용 강제 feed 모드
   1이면 RTC 무시하고 강제 feed 시연
   0이면 RTC 기반 실제 시간 확인 */
#define FORCE_FEED_DEMO      1

/* feed 후 rail 대기 시간(ms)
   플로우차트상 3~5초, 필요하면 5000으로 변경 가능 */
#define FEED_TO_RAIL_WAIT_MS 3000

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

static SystemState sysState = SYS_STATE_MONITOR;

static uint8_t cleanRequest = 0;
static uint8_t feedRequest  = 0;
static uint8_t done_sig     = 0;

static uint8_t last_fed_hour = 255;
static uint8_t last_fed_min  = 255;

/* =========================
   GPIO 통신 함수
   ========================= */
static void gpio_feed_master_init(void)
{
    /* PA0 : feed_cmd 출력 */
    FEED_CMD_DDR |= (1 << FEED_CMD_PIN);
    FEED_CMD_PORT &= ~(1 << FEED_CMD_PIN);

    /* PA1 : feed_done 입력 */
    FEED_DONE_DDR &= ~(1 << FEED_DONE_PIN);
}

static void feed_start_command(void)
{
    FEED_CMD_PORT |= (1 << FEED_CMD_PIN);
}

static void feed_clear_command(void)
{
    FEED_CMD_PORT &= ~(1 << FEED_CMD_PIN);
}

static uint8_t feed_done_received(void)
{
    return (FEED_DONE_PINR & (1 << FEED_DONE_PIN)) ? 1 : 0;
}

/* =========================
   LCD 출력 함수
   ========================= */
static void lcd_print_2line(const char *line1, const char *line2)
{
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_WriteString((char *)line1);
    LCD_SetCursor(1, 0);
    LCD_WriteString((char *)line2);
}

static void lcd_show_startup(void)
{
    lcd_print_2line("SMART AQUARIUM", "SYSTEM START");
}

static void lcd_show_monitor(WaterState state, uint16_t diff)
{
    char line2[17];

    snprintf(line2, sizeof(line2), "DIFF:%3u", diff);

    if (state == WATER_STATE_GOOD)
    {
        lcd_print_2line("Water: CLEAN", line2);
    }
    else if (state == WATER_STATE_NORMAL)
    {
        lcd_print_2line("Water: CLOUDY", line2);
    }
    else
    {
        lcd_print_2line("!! WARNING !!", line2);
    }
}

static void lcd_show_clean_cause(void)
{
    lcd_print_2line("Rail Cause:", "Water Cleaning");
}

static void lcd_show_clean_done(void)
{
    lcd_print_2line("Rail Done", "Cause: Clean");
}

static void lcd_show_feed_start(void)
{
    lcd_print_2line("Feed Start", "Command Sent");
}

static void lcd_show_feed_running(void)
{
    lcd_print_2line("Feed Running", "System Operate");
}

static void lcd_show_feed_wait(void)
{
    lcd_print_2line("Feed Request ON", "Rail Waiting...");
}

static void lcd_show_feed_cause(void)
{
    lcd_print_2line("Rail Cause:", "After Feed");
}

static void lcd_show_feed_done(void)
{
    lcd_print_2line("Rail Done", "Cause: Feed");
}

/* =========================
   LED 상태 표시
   ========================= */
static void led_show_water_state(WaterState state)
{
    if (state == WATER_STATE_GOOD)
    {
        LED_SetStatus(STATUS_GOOD);
    }
    else if (state == WATER_STATE_NORMAL)
    {
        LED_SetStatus(STATUS_CHECK);
    }
    else
    {
        LED_SetStatus(STATUS_WARNING);
    }
}

/* =========================
   RTC 시간 기반 feed 요청
   Board1에서는 서보를 직접 돌리지 않고
   feedRequest만 생성
   ========================= */
static uint8_t check_feed_time_request(void)
{
    RTC_Time now;

    RTC_read_time(&now);

    if (((now.hour == FEED_HOUR_MORNING) || (now.hour == FEED_HOUR_AFTERNOON)) &&
        (now.min == FEED_MIN))
    {
        if ((last_fed_hour != now.hour) || (last_fed_min != now.min))
        {
            last_fed_hour = now.hour;
            last_fed_min  = now.min;
            return 1;
        }
    }

    return 0;
}

/* =========================
   메인
   ========================= */
int main(void)
{
    TurbidityData turbidityData;
    WaterState lastState = (WaterState)255;
    uint16_t lastDiff    = 65535;

    ds1302_init();
    gpio_feed_master_init();
    I2C_Init();
    LCD_Init();
    LED_Init();
    Turbidity_Init();
    relay_init();

    lcd_show_startup();
    _delay_ms(2000);

    while (1)
    {
        switch (sysState)
        {
            case SYS_STATE_MONITOR:
            {
                /* 1. 조도센서 기반 수질 측정 */
                Turbidity_Update();
                Turbidity_GetData(&turbidityData);
                cleanRequest = (uint8_t)turbidityData.cleanRequest;

                /* 2. 평상시 상태 표시 */
                if ((turbidityData.waterState != lastState) ||
                    (turbidityData.turbidityValue != lastDiff))
                {
                    lcd_show_monitor(turbidityData.waterState,
                                     turbidityData.turbidityValue);
                    led_show_water_state(turbidityData.waterState);

                    lastState = turbidityData.waterState;
                    lastDiff  = turbidityData.turbidityValue;
                }

                /* 3. feed 요청 확인 */
#if FORCE_FEED_DEMO
                /* 시연용: monitor 상태에서 잠시 후 강제 feed */
                _delay_ms(1000);
                feedRequest = 1;
#else
                feedRequest = check_feed_time_request();
#endif

                /* 4. 상태 전이 */
                if (feedRequest == 1)
                {
                    sysState = SYS_STATE_FEED_START;
                }
                else if (cleanRequest == 1)
                {
                    sysState = SYS_STATE_CLEAN_CAUSE;
                }
                else
                {
                    _delay_ms(500);
                }

                break;
            }

            case SYS_STATE_CLEAN_CAUSE:
            {
                lcd_show_clean_cause();
                _delay_ms(1500);
                sysState = SYS_STATE_CLEAN_RAIL;
                break;
            }

            case SYS_STATE_CLEAN_RAIL:
            {
                done_sig = 0;
                rail_run(1, 0, &done_sig);

                if (done_sig == 1)
                {
                    sysState = SYS_STATE_CLEAN_DONE;
                }
                break;
            }

            case SYS_STATE_CLEAN_DONE:
            {
                lcd_show_clean_done();
                _delay_ms(1500);

                done_sig = 0;
                sysState = SYS_STATE_MONITOR;
                break;
            }

            case SYS_STATE_FEED_START:
            {
                lcd_show_feed_start();
                _delay_ms(1000);

                feed_start_command();
                sysState = SYS_STATE_FEED_RUNNING;
                break;
            }

            case SYS_STATE_FEED_RUNNING:
            {
                lcd_show_feed_running();

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

            case SYS_STATE_FEED_WAIT:
            {
                lcd_show_feed_wait();
                _delay_ms(FEED_TO_RAIL_WAIT_MS);

                sysState = SYS_STATE_FEED_CAUSE;
                break;
            }

            case SYS_STATE_FEED_CAUSE:
            {
                lcd_show_feed_cause();
                _delay_ms(1500);

                sysState = SYS_STATE_FEED_RAIL;
                break;
            }

            case SYS_STATE_FEED_RAIL:
            {
                done_sig = 0;
                rail_run(0, 1, &done_sig);

                if (done_sig == 1)
                {
                    sysState = SYS_STATE_FEED_DONE;
                }
                break;
            }

            case SYS_STATE_FEED_DONE:
            {
                lcd_show_feed_done();
                _delay_ms(1500);

                feedRequest = 0;
                done_sig = 0;
                sysState = SYS_STATE_MONITOR;
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

// for board 2 ( final )
// #ifndef F_CPU
// #define F_CPU 16000000UL
// #endif

// #include <avr/io.h>
// #include <util/delay.h>
// #include <stdint.h>
// #include "servo.h"

// /* =========================
//    ATmega128A pin map 기준
//    PA0 = 51번 핀  -> feed_cmd 입력
//    PA1 = 50번 핀  -> feed_done 출력
//    PB5 = 15번 핀  -> servo PWM (OC1A)
//    ========================= */

// #define FEED_CMD_DDR   DDRA
// #define FEED_CMD_PORT  PORTA
// #define FEED_CMD_PINR  PINA
// #define FEED_CMD_PIN   PA0

// #define FEED_DONE_DDR  DDRA
// #define FEED_DONE_PORT PORTA
// #define FEED_DONE_PIN  PA1

// static void gpio_feed_slave_init(void)
// {
//     /* PA0 : feed_cmd 입력 */
//     FEED_CMD_DDR &= ~(1 << FEED_CMD_PIN);

//     /* PA1 : feed_done 출력 */
//     FEED_DONE_DDR |= (1 << FEED_DONE_PIN);
//     FEED_DONE_PORT &= ~(1 << FEED_DONE_PIN);
// }

// static uint8_t feed_command_received(void)
// {
//     return (FEED_CMD_PINR & (1 << FEED_CMD_PIN)) ? 1 : 0;
// }

// static void feed_done_set(void)
// {
//     FEED_DONE_PORT |= (1 << FEED_DONE_PIN);
// }

// static void feed_done_clear(void)
// {
//     FEED_DONE_PORT &= ~(1 << FEED_DONE_PIN);
// }

// /* 시연용 feed 동작
//    - 1회 배급 동작
//    - 너무 큰 범위 대신 비교적 안전한 범위 사용 */
// static void demo_feed_motion(void)
// {
//     servo_set(2800);
//     _delay_ms(1500);

//     servo_set(3400);
//     _delay_ms(1500);

//     servo_set(3000);
//     _delay_ms(800);
// }

// int main(void)
// {
//     servo_init();
//     gpio_feed_slave_init();

//     /* 초기 중립 위치 */
//     servo_set(3000);
//     _delay_ms(1000);

//     while (1)
//     {
//         if (feed_command_received())
//         {
//             /* 1회 먹이 배급 */
//             demo_feed_motion();

//             /* 완료 신호 전송 */
//             feed_done_set();

//             /* 보드1이 feed_cmd를 내릴 때까지 대기
//                -> HIGH 유지 중 반복 배급 방지 */
//             while (feed_command_received())
//             {
//                 _delay_ms(10);
//             }

//             /* 명령 해제되면 done도 내림 */
//             feed_done_clear();

//             /* 다시 중립 위치 */
//             servo_set(3000);
//             _delay_ms(300);
//         }
//     }
// }

// //for board 2 ( feed )
// #ifndef F_CPU
// #define F_CPU 16000000UL
// #endif

// #include <avr/io.h>
// #include <util/delay.h>
// #include <stdint.h>
// #include "servo.h"

// /* ATmega128A pin map 기준
//    PA0 = 51번 핀
//    PA1 = 50번 핀
//    PB5 = 15번 핀 (OC1A) */

// #define FEED_CMD_DDR   DDRA
// #define FEED_CMD_PORT  PORTA
// #define FEED_CMD_PINR  PINA
// #define FEED_CMD_PIN   PA0   // 51번 핀

// #define FEED_DONE_DDR  DDRA
// #define FEED_DONE_PORT PORTA
// #define FEED_DONE_PIN  PA1   // 50번 핀

// static void gpio_feed_slave_init(void)
// {
//     /* PA0: feed_cmd 입력 */
//     FEED_CMD_DDR &= ~(1 << FEED_CMD_PIN);

//     /* PA1: feed_done 출력 */
//     FEED_DONE_DDR |= (1 << FEED_DONE_PIN);
//     FEED_DONE_PORT &= ~(1 << FEED_DONE_PIN);
// }

// static uint8_t feed_command_received(void)
// {
//     return (FEED_CMD_PINR & (1 << FEED_CMD_PIN)) ? 1 : 0;
// }

// static void feed_done_set(void)
// {
//     FEED_DONE_PORT |= (1 << FEED_DONE_PIN);
// }

// static void feed_done_clear(void)
// {
//     FEED_DONE_PORT &= ~(1 << FEED_DONE_PIN);
// }

// static void demo_feed_motion(void)
// {
//     servo_set(2950);
//     _delay_ms(1200);

//     servo_set(3050);
//     _delay_ms(1200);

//     servo_set(3000);
//     _delay_ms(800);
// }

// int main(void)
// {
//     servo_init();            // PB5(15번, OC1A)
//     gpio_feed_slave_init();  // PA0 입력, PA1 출력

//     servo_set(3000);
//     _delay_ms(1000);

//     while (1)
//     {
//         if (feed_command_received())
//         {
//             demo_feed_motion();
//             feed_done_set();

//             while (feed_command_received())
//             {
//                 _delay_ms(10);
//             }

//             feed_done_clear();
//             servo_set(3000);
//             _delay_ms(300);
//         }
        
//     }
// }