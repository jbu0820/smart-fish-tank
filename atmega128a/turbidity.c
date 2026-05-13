#include "turbidity.h"

#include <avr/io.h>
#include <util/delay.h>

/*
 * ============================================================
 * 수질 판단 기준값
 * ============================================================
 *
 * 실제 측정값 기준:
 *
 * 맑은 물:
 *   DIFF 약 90~100
 *
 * 탁한 물:
 *   DIFF 약 30~40
 *
 * 최종 판단 기준:
 *
 * DIFF >= 80:
 *   WATER_STATE_GOOD
 *
 * DIFF >= 50:
 *   WATER_STATE_NORMAL
 *
 * DIFF < 50:
 *   WATER_STATE_BAD
 */
#define TURBIDITY_GOOD_THRESHOLD       80 // for test//80 orginal
#define TURBIDITY_NORMAL_THRESHOLD     50 // for test//50 orginal

/*
 * ============================================================
 * 측정 시간 설정
 * ============================================================
 *
 * 플로우차트 기준:
 *
 * 측정 LED OFF
 * → 주변광 기준값 측정
 * → 측정 LED ON
 * → 안정화 대기
 * → LED ON 상태 조도값 측정
 * → 측정 LED OFF
 * → 빛 통과량 계산
 */
#define TURBIDITY_LED_OFF_WAIT_MS      100 // for test //1000 orginal
#define TURBIDITY_LED_ON_WAIT_MS       200// for test//2000 orginal

/*
 * ============================================================
 * ADC 평균 설정
 * ============================================================
 *
 * 조도센서 값은 순간적으로 흔들릴 수 있으므로
 * 여러 번 측정한 뒤 평균값을 사용합니다.
 */
#define TURBIDITY_ADC_SAMPLE_COUNT     10//for test//30 orginal
#define TURBIDITY_ADC_SAMPLE_DELAY_MS  2//for test // 5 orginal

/*
 * ============================================================
 * 자동 측정 주기
 * ============================================================
 *
 * main.c에서 Turbidity_Update()를 약 1초마다 호출한다고 가정하면,
 * 약 5초마다 한 번 자동 측정합니다.
 */
#define TURBIDITY_MEASURE_INTERVAL     1 // fortest //5 orginal

/*
 * ============================================================
 * 핀 설정
 * ============================================================
 *
 * RGB LED 모듈:
 *
 * RGB LED 모듈 -  → GND
 * RGB LED 모듈 R  → PB1
 * RGB LED 모듈 G  → PB2
 * RGB LED 모듈 B  → PB0
 *
 * 조도센서:
 *
 * 조도센서 아날로그 출력 → PF0 / ADC0
 */
#define TURBIDITY_LED_DDR      DDRF
#define TURBIDITY_LED_PORT     PORTF

#define TURBIDITY_LED_PIN_R    PF1
#define TURBIDITY_LED_PIN_G    PF2
#define TURBIDITY_LED_PIN_B    PF3

#define TURBIDITY_LED_MASK     ((1 << TURBIDITY_LED_PIN_R) | \
                                (1 << TURBIDITY_LED_PIN_G) | \
                                (1 << TURBIDITY_LED_PIN_B))

#define TURBIDITY_ADC_CHANNEL  0

/*
 * ============================================================
 * 내부 상태 변수
 * ============================================================
 */
static TurbidityFsmState turbidityFsmState = TURBIDITY_STATE_IDLE;

static WaterState waterState = WATER_STATE_NORMAL;
static CleanRequestState cleanRequest = CLEAN_REQUEST_OFF;

static uint16_t ledOffValue = 0;
static uint16_t ledOnValue = 0;
static uint16_t turbidityValue = 0;

static uint8_t measureRequest = FALSE;
static uint8_t afterFeedingCheckRequest = FALSE;

static uint16_t updateCounter = 0;

/*
 * ============================================================
 * 내부 함수 선언
 * ============================================================
 */
static void TurbidityLed_On(void);
static void TurbidityLed_Off(void);

static void ADC_Init(void);
static uint16_t ADC_Read(uint8_t channel);
static uint16_t ADC_ReadAverage(uint8_t channel);

static void Turbidity_Measure(void);
static void Turbidity_Decide(void);

/*
 * ============================================================
 * 탁도 모듈 초기화
 * ============================================================
 */
void Turbidity_Init(void)
{
    /*
     * RGB 측정 LED 핀을 출력으로 설정합니다.
     */
    TURBIDITY_LED_DDR |= TURBIDITY_LED_MASK;

    /*
     * 초기 상태에서는 측정 LED를 꺼둡니다.
     */
    TurbidityLed_Off();

    /*
     * ADC 초기화
     */
    ADC_Init();

    /*
     * 내부 상태 초기화
     */
    turbidityFsmState = TURBIDITY_STATE_IDLE;

    waterState = WATER_STATE_NORMAL;
    cleanRequest = CLEAN_REQUEST_OFF;

    ledOffValue = 0;
    ledOnValue = 0;
    turbidityValue = 0;

    /*
     * 전원 ON 후 첫 측정을 바로 수행하기 위해 TRUE로 설정합니다.
     */
    measureRequest = TRUE;
    afterFeedingCheckRequest = FALSE;

    updateCounter = 0;
}

/*
 * ============================================================
 * 탁도 FSM 업데이트
 * ============================================================
 *
 * main.c의 while(1) 안에서 반복 호출합니다.
 */
void Turbidity_Update(void)
{
    switch (turbidityFsmState)
    {
        case TURBIDITY_STATE_IDLE:
        {
            /*
             * 자동 측정 주기 카운트
             *
             * main.c에서 약 1초마다 Turbidity_Update()가 호출되면
             * TURBIDITY_MEASURE_INTERVAL 값에 따라 자동 측정됩니다.
             */
            updateCounter++;

            if (updateCounter >= TURBIDITY_MEASURE_INTERVAL)
            {
                updateCounter = 0;
                measureRequest = TRUE;
            }

            /*
             * 수동 측정 요청 또는 먹이 배급 후 측정 요청이 있으면
             * RUN 상태로 이동합니다.
             */
            if ((measureRequest == TRUE) ||
                (afterFeedingCheckRequest == TRUE))
            {
                turbidityFsmState = TURBIDITY_STATE_RUN;
            }

            break;
        }

        case TURBIDITY_STATE_RUN:
        {
            /*
             * 실제 탁도 측정 수행
             */
            Turbidity_Measure();

            turbidityFsmState = TURBIDITY_STATE_DECIDE;
            break;
        }

        case TURBIDITY_STATE_DECIDE:
        {
            /*
             * 측정된 DIFF 값을 기준으로 수질 판단
             */
            Turbidity_Decide();

            turbidityFsmState = TURBIDITY_STATE_OUTPUT;
            break;
        }

        case TURBIDITY_STATE_OUTPUT:
        {
            /*
             * 이 상태에서는 waterState와 cleanRequest가
             * 이미 최신 값으로 갱신되어 있습니다.
             *
             * 다른 모듈은 아래 함수로 값을 읽으면 됩니다.
             *
             * Turbidity_GetWaterState();
             * Turbidity_GetCleanRequest();
             * Turbidity_GetData(&data);
             */
            turbidityFsmState = TURBIDITY_STATE_DONE;
            break;
        }

        case TURBIDITY_STATE_DONE:
        {
            /*
             * 요청 플래그 정리 후 IDLE로 복귀
             */
            measureRequest = FALSE;
            afterFeedingCheckRequest = FALSE;

            turbidityFsmState = TURBIDITY_STATE_IDLE;
            break;
        }

        default:
        {
            /*
             * 예외 상황에서는 안전하게 IDLE로 복귀
             */
            turbidityFsmState = TURBIDITY_STATE_IDLE;
            break;
        }
    }
}

/*
 * ============================================================
 * 수동 측정 요청
 * ============================================================
 */
void Turbidity_RequestMeasure(void)
{
    measureRequest = TRUE;
}

/*
 * ============================================================
 * 먹이 배급 후 측정 요청
 * ============================================================
 */
void Turbidity_RequestAfterFeedingCheck(void)
{
    afterFeedingCheckRequest = TRUE;
}

/*
 * ============================================================
 * 측정용 RGB LED ON
 * ============================================================
 */
static void TurbidityLed_On(void)
{
    TURBIDITY_LED_PORT |= TURBIDITY_LED_MASK;
}

/*
 * ============================================================
 * 측정용 RGB LED OFF
 * ============================================================
 */
static void TurbidityLed_Off(void)
{
    TURBIDITY_LED_PORT &= ~TURBIDITY_LED_MASK;
}

/*
 * ============================================================
 * ADC 초기화
 * ============================================================
 */
static void ADC_Init(void)
{
    /*
     * ADMUX 설정
     *
     * REFS0 = 1:
     *   AVCC를 ADC 기준전압으로 사용합니다.
     *
     * 조도센서 값은 PF0 / ADC0에서 읽습니다.
     */
    ADMUX = (1 << REFS0);

    /*
     * ADCSRA 설정
     *
     * ADEN = 1:
     *   ADC 활성화
     *
     * ADPS2:0 = 111:
     *   분주비 128
     *
     * 16MHz / 128 = 125kHz
     * ADC 동작에 적절한 클럭입니다.
     */
    ADCSRA = (1 << ADEN)
           | (1 << ADPS2)
           | (1 << ADPS1)
           | (1 << ADPS0);
}

/*
 * ============================================================
 * ADC 1회 읽기
 * ============================================================
 */
static uint16_t ADC_Read(uint8_t channel)
{
    /*
     * 기준전압 설정은 유지하고,
     * MUX 비트만 변경하여 원하는 ADC 채널을 선택합니다.
     */
    ADMUX = (ADMUX & 0xE0) | (channel & 0x1F);

    /*
     * ADC 변환 시작
     */
    ADCSRA |= (1 << ADSC);

    /*
     * 변환이 끝날 때까지 대기
     */
    while (ADCSRA & (1 << ADSC));

    /*
     * 10비트 ADC 결과 반환
     */
    return ADC;
}

/*
 * ============================================================
 * ADC 평균값 읽기
 * ============================================================
 */
static uint16_t ADC_ReadAverage(uint8_t channel)
{
    uint32_t sum = 0;

    for (uint8_t i = 0; i < TURBIDITY_ADC_SAMPLE_COUNT; i++)
    {
        sum += ADC_Read(channel);
        _delay_ms(TURBIDITY_ADC_SAMPLE_DELAY_MS);
    }

    return (uint16_t)(sum / TURBIDITY_ADC_SAMPLE_COUNT);
}

/*
 * ============================================================
 * 탁도 측정
 * ============================================================
 *
 * 플로우차트 기준 순서:
 *
 * 1. 측정 LED OFF
 * 2. LED OFF 상태에서 주변광 기준값 측정
 * 3. 측정 LED ON
 * 4. LED ON 상태에서 조도값 측정
 * 5. 측정 LED OFF
 * 6. DIFF = LED ON 값 - LED OFF 값
 */
static void Turbidity_Measure(void)
{
    /*
     * ------------------------------------------------------------
     * 1. 측정 LED OFF
     * ------------------------------------------------------------
     */
    TurbidityLed_Off();

    /*
     * 주변광 값이 안정되도록 대기
     */
    _delay_ms(TURBIDITY_LED_OFF_WAIT_MS);

    /*
     * ------------------------------------------------------------
     * 2. LED OFF 상태 조도센서 값 측정
     *    이 값은 주변광 기준값입니다.
     * ------------------------------------------------------------
     */
    ledOffValue = ADC_ReadAverage(TURBIDITY_ADC_CHANNEL);

    /*
     * ------------------------------------------------------------
     * 3. 측정 LED ON
     * ------------------------------------------------------------
     */
    TurbidityLed_On();

    /*
     * LED 빛과 센서값이 안정되도록 대기
     */
    _delay_ms(TURBIDITY_LED_ON_WAIT_MS);

    /*
     * ------------------------------------------------------------
     * 4. LED ON 상태 조도센서 값 측정
     * ------------------------------------------------------------
     */
    ledOnValue = ADC_ReadAverage(TURBIDITY_ADC_CHANNEL);

    /*
     * ------------------------------------------------------------
     * 5. 측정 LED OFF
     * ------------------------------------------------------------
     */
    TurbidityLed_Off();

    /*
     * ------------------------------------------------------------
     * 6. 빛 통과량 계산
     *
     * DIFF = LED ON 평균값 - LED OFF 주변광 기준값
     *
     * 값이 클수록 빛이 잘 통과한 것이므로 수질이 좋다고 판단합니다.
     * 값이 작을수록 물이 탁해서 빛이 적게 통과한 것입니다.
     * ------------------------------------------------------------
     */
    if (ledOnValue > ledOffValue)
    {
        turbidityValue = ledOnValue - ledOffValue;
    }
    else
    {
        turbidityValue = 0;
    }
}

/*
 * ============================================================
 * 수질 판단
 * ============================================================
 */
static void Turbidity_Decide(void)
{
    if (turbidityValue >= TURBIDITY_GOOD_THRESHOLD)
    {
        /*
         * DIFF >= 80
         * 수질 좋음
         */
        waterState = WATER_STATE_GOOD;
        cleanRequest = CLEAN_REQUEST_OFF;
    }
    else if (turbidityValue >= TURBIDITY_NORMAL_THRESHOLD)
    {
        /*
         * 50 <= DIFF < 80
         * 수질 보통
         */
        waterState = WATER_STATE_NORMAL;
        cleanRequest = CLEAN_REQUEST_OFF;
    }
    else
    {
        /*
         * DIFF < 50
         * 수질 나쁨
         *
         * 레일 모듈이 이 값을 보고 수거 동작을 실행할 수 있습니다.
         */
        waterState = WATER_STATE_BAD;
        cleanRequest = CLEAN_REQUEST_ON;
    }
}

/*
 * ============================================================
 * Getter 함수들
 * ============================================================
 */
WaterState Turbidity_GetWaterState(void)
{
    return waterState;
}

CleanRequestState Turbidity_GetCleanRequest(void)
{
    return cleanRequest;
}

TurbidityFsmState Turbidity_GetFsmState(void)
{
    return turbidityFsmState;
}

uint16_t Turbidity_GetValue(void)
{
    return turbidityValue;
}

uint16_t Turbidity_GetLedOffValue(void)
{
    return ledOffValue;
}

uint16_t Turbidity_GetLedOnValue(void)
{
    return ledOnValue;
}

/*
 * ============================================================
 * 구조체 포인터 방식 데이터 전달
 * ============================================================
 */
void Turbidity_GetData(TurbidityData *data)
{
    if (data == 0)
    {
        return;
    }

    data->waterState = waterState;
    data->cleanRequest = cleanRequest;
    data->fsmState = turbidityFsmState;

    data->turbidityValue = turbidityValue;
    data->ledOffValue = ledOffValue;
    data->ledOnValue = ledOnValue;
}


void turbidity(TurbidityData *turbidityData)
{
    
    /*
     * ============================================================
     * 모듈 초기화
     * ============================================================
     */
    //Turbidity_Init();
    Turbidity_Update();
    Turbidity_GetData(turbidityData);
    _delay_ms(1000);

}