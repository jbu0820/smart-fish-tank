#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "common.h"
#include "system.h"
#include "feeder.h"
#include "rail.h"
#include "turbidity.h"
#include "i2c_lcd.h"
#include "rgb_led.h"

static void lcd_show_water_state(WaterState state)
{
    LCD_Clear();

    if (state == WATER_STATE_GOOD)
    {
        LCD_SetCursor(0, 0);
        LCD_WriteString("Water: CLEAN");
        LCD_SetCursor(1, 0);
        LCD_WriteString("Status: GOOD");
    }
    else if (state == WATER_STATE_NORMAL)
    {
        LCD_SetCursor(0, 0);
        LCD_WriteString("Water: NORMAL");
        LCD_SetCursor(1, 0);
        LCD_WriteString("Status: CHECK");
    }
    else
    {
        LCD_SetCursor(0, 0);
        LCD_WriteString("Water: BAD");
        LCD_SetCursor(1, 0);
        LCD_WriteString("Rail Cleaning...");
    }
}

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

int main(void)
{
    TurbidityData turbidityData;

    uint8_t cleanRequest = 0;
    uint8_t feedRequest  = 0;
    uint8_t done_sig     = 0;

    WaterState lastWaterState = 255;

    system_init();   // RTC + servo + feeder init
    I2C_Init();
    LCD_Init();
    LED_Init();
    Turbidity_Init();
    relay_init();

    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_WriteString("Smart Aquarium");
    LCD_SetCursor(1, 0);
    LCD_WriteString("System Start");
    _delay_ms(1000);

    while (1)
    {
        // 1. RTC 기반 먹이 스케줄 확인
        feedRequest = feeder_update();

        // 2. 탁도 FSM 업데이트
        Turbidity_Update();
        Turbidity_GetData(&turbidityData);

        cleanRequest = (uint8_t)turbidityData.cleanRequest;

        // 3. 수질 상태 표시 (상태가 바뀔 때만 갱신)
        if (turbidityData.waterState != lastWaterState)
        {
            lcd_show_water_state(turbidityData.waterState);
            led_show_water_state(turbidityData.waterState);
            lastWaterState = turbidityData.waterState;
        }

        // 4. 레일 동작
        rail_run(cleanRequest, feedRequest, &done_sig);

        // 5. 완료 후 요청 정리
        if (done_sig == 1)
        {
            feedRequest = 0;
        }

        // Turbidity_Update() 주기 고려
        _delay_ms(1000);
    }
}