#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

#include "common.h"
#include "turbidity.h"

/*
 * ============================================================
 * main.c
 * ============================================================
 *
 * 최종 프로젝트용 기본 main입니다.
 *
 * 이 파일에서는 탁도 모듈을 초기화하고,
 * while(1) 안에서 Turbidity_Update()를 반복 호출합니다.
 *
 * 다른 팀원 모듈과 통합할 때는 이 main.c에
 * LCD, LED, 먹이 배급, 레일 모듈 초기화 및 업데이트 함수를
 * 추가하면 됩니다.
 */

int main(void)
{
    TurbidityData turbidityData;

    /*
     * ============================================================
     * 모듈 초기화
     * ============================================================
     */
    Turbidity_Init();

    /*
     * 추후 통합 시 예시:
     *
     * LCD_Init();
     * StatusLed_Init();
     * Feeding_Init();
     * Rail_Init();
     */

    while (1)
    {
        /*
         * ========================================================
         * 탁도 FSM 업데이트
         * ========================================================
         */
        Turbidity_Update();

        /*
         * ========================================================
         * 현재 탁도 데이터 가져오기
         * ========================================================
         */
        Turbidity_GetData(&turbidityData);

        /*
         * ========================================================
         * 다른 모듈과 연결할 때 사용하는 부분
         * ========================================================
         *
         * 1. 수질 상태 표시
         *
         * if (turbidityData.waterState == WATER_STATE_GOOD)
         * {
         *     LCD_ShowWaterGood();
         *     StatusLed_Green();
         * }
         * else if (turbidityData.waterState == WATER_STATE_NORMAL)
         * {
         *     LCD_ShowWaterNormal();
         *     StatusLed_Yellow();
         * }
         * else
         * {
         *     LCD_ShowWarning();
         *     StatusLed_Red();
         * }
         *
         * 2. 레일 청소 요청
         *
         * if (turbidityData.cleanRequest == CLEAN_REQUEST_ON)
         * {
         *     Rail_RequestClean();
         * }
         *
         * 3. 먹이 배급 후 측정 요청
         *
         * 먹이 담당 모듈에서 배급 후 일정 시간이 지나면:
         *
         * Turbidity_RequestAfterFeedingCheck();
         */

        /*
         * 현재 Turbidity_Update() 내부의 자동 측정 주기는
         * 이 delay가 약 1초라는 가정으로 동작합니다.
         */
        _delay_ms(1000);
    }
}
