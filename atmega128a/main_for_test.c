#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "system.h"
#include "rail.h"
#include "i2c_lcd.h"
#include "rgb_led.h"
#include "servo.h"

static void lcd_show(const char *line1, const char *line2)
{
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_WriteString((char *)line1);
    LCD_SetCursor(1, 0);
    LCD_WriteString((char *)line2);
}

int main(void)
{
    uint8_t done_sig = 0;

    // 기본 초기화
    system_init();   // RTC + servo + feeder init
    I2C_Init();
    LCD_Init();
    LED_Init();
    relay_init();

    while (1)
    {
        /*
         * ----------------------------------------------------
         * 1. 시작 화면
         * ----------------------------------------------------
         */
        lcd_show("Smart Aquarium", "Demo Start");
        LED_AllOff();
        _delay_ms(2000);

        /*
         * ----------------------------------------------------
         * 2. 수질 GOOD 상태 시연
         * ----------------------------------------------------
         */
        lcd_show("Water: CLEAN", "Status: GOOD");
        LED_SetStatus(STATUS_GOOD);
        _delay_ms(3000);

        /*
         * ----------------------------------------------------
         * 3. 수질 NORMAL 상태 시연
         * ----------------------------------------------------
         */
        lcd_show("Water: NORMAL", "Status: CHECK");
        LED_SetStatus(STATUS_CHECK);
        _delay_ms(3000);

        /*
         * ----------------------------------------------------
         * 4. 수질 BAD 상태 시연
         * ----------------------------------------------------
         */
        lcd_show("Water: BAD", "Need Cleaning");
        LED_SetStatus(STATUS_WARNING);
        _delay_ms(3000);

        /*
         * ----------------------------------------------------
         * 5. 청소 요청 -> 레일 동작
         * cleanRequest = 1
         * ----------------------------------------------------
         */
        lcd_show("Rail Demo", "Cleaning Run");
        done_sig = 0;
        rail_run(1, 0, &done_sig);

        if (done_sig == 1)
        {
            lcd_show("Rail Demo", "Clean Done");
            _delay_ms(2000);
        }

        /*
         * ----------------------------------------------------
         * 6. 먹이 배급 시연
         * ----------------------------------------------------
         */
        lcd_show("Feeding Demo", "Feed Start");
        LED_SetStatus(STATUS_GOOD);
        _delay_ms(1000);

        feed_fish();

        lcd_show("Feeding Demo", "Feed Done");
        _delay_ms(2000);

        /*
         * ----------------------------------------------------
         * 7. 먹이 후 레일 동작 시연
         * feedRequest = 1
         * rail_run 내부에서 delay 후 동작
         * ----------------------------------------------------
         */
        lcd_show("After Feeding", "Rail Waiting");
        done_sig = 0;
        rail_run(0, 1, &done_sig);

        if (done_sig == 1)
        {
            lcd_show("After Feeding", "Rail Done");
            _delay_ms(2000);
        }

        /*
         * ----------------------------------------------------
         * 8. 데모 종료 후 잠시 대기
         * ----------------------------------------------------
         */
        lcd_show("Demo Sequence", "Restart Soon");
        LED_AllOff();
        _delay_ms(3000);
    }
}