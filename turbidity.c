#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdint.h>

#include "common.h"
#include "turbidity.h"

/*
 * ============================================================
 * 핀 설정
 * ============================================================
 *
 * 팀원들과 핀 충돌이 있으면 이 부분만 수정하면 됩니다.
 */

/*
 * 조도센서 ADC 채널
 * ATmega128A 기준 PF0 = ADC0
 */
#define TURBIDITY_ADC_CHANNEL          0

/*
 * 탁도 측정용 LED
 * PB0 사용
 */
#define TURBIDITY_LED_DDR              DDRB
#define TURBIDITY_LED_PORT             PORTB
#define TURBIDITY_LED_PIN              PB0

/*
 * 수질 상태 표시 LED
 * PD0: 초록
 * PD1: 노랑
 * PD2: 빨강
 */
#define WATER_LED_DDR                  DDRD
#define WATER_LED_PORT                 PORTD

#define WATER_GREEN_LED_PIN            PD0
#define WATER_YELLOW_LED_PIN           PD1
#define WATER_RED_LED_PIN              PD2

/*
 * 레일 동작신호 출력 핀
 * PD3 사용
 *
 * 수질 나쁨이면 HIGH
 * 수질 좋음/보통이면 LOW
 */
#define RAIL_SIGNAL_DDR                DDRD
#define RAIL_SIGNAL_PORT               PORTD
#define RAIL_SIGNAL_PIN                PD3

/*
 * ============================================================
 * 측정 설정값
 * ============================================================
 */
#define TURBIDITY_MEASURE_INTERVAL_MS  5000UL
#define TURBIDITY_LED_STABLE_TIME_MS   50
#define TURBIDITY_ADC_SAMPLE_COUNT     10
#define TURBIDITY_ADC_SAMPLE_DELAY_MS  5

/*
 * ============================================================
 * 수질 판단 기준값
 * ============================================================
 *
 * 이 값은 실제 측정 후 조정해야 합니다.
 */
#define TURBIDITY_GOOD_THRESHOLD       600
#define TURBIDITY_NORMAL_THRESHOLD     300

/*
 * ============================================================
 * 조도센서 회로 방향 설정
 * ============================================================
 *
 * 1:
 *   빛이 강할수록 ADC 값이 커지는 회로
 *
 * 0:
 *   빛이 강할수록 ADC 값이 작아지는 회로
 */
#define LIGHT_INCREASES_ADC            1

/*
 * ============================================================
 * 내부 상태 변수
 * ============================================================
 */

static TurbidityFsmState turbidityFsmState = TURBIDITY_STATE_IDLE;

static WaterState currentWaterState = WATER_STATE_GOOD;
static CleanRequestState currentCleanRequest = CLEAN_REQUEST_OFF;

static uint16_t turbidityValue = 0;
static uint16_t ambientValue = 0;
static uint16_t ledOnAverageValue = 0;

/*
 * ISR에서 증가하고 main 루프에서 읽거나 초기화합니다.
 * 따라서 volatile + ATOMIC_BLOCK 사용이 필요합니다.
 */
static volatile uint32_t measureTimerMs = 0;

/*
 * 플래그 변수
 *
 * ISR 또는 다른 모듈에서 변경될 가능성을 고려해 volatile 처리합니다.
 */
static volatile uint8_t firstMeasureRequestFlag = TRUE;
static volatile uint8_t manualMeasureRequestFlag = FALSE;
static volatile uint8_t afterFeedingCheckRequestFlag = FALSE;

/*
 * ============================================================
 * 내부 함수 선언
 * ============================================================
 */

static void ADC_Init(void);
static uint16_t ADC_Read(uint8_t channel);

static uint8_t Turbidity_IsMeasureStartCondition(void);
static uint16_t Turbidity_MeasureLightAmount(void);
static void Turbidity_DecideWaterState(void);
static void Turbidity_UpdateOutput(void);
static void Turbidity_ClearRequestFlagsAndTimer(void);

static void TurbidityLed_On(void);
static void TurbidityLed_Off(void);

static void WaterStatusLed_AllOff(void);
static void WaterStatusLed_GreenOn(void);
static void WaterStatusLed_YellowOn(void);
static void WaterStatusLed_RedOn(void);

static void RailSignal_On(void);
static void RailSignal_Off(void);

/*
 * ============================================================
 * 외부 공개 함수 구현
 * ============================================================
 */

void Turbidity_Init(void)
{
    ADC_Init();

    /*
     * 출력 핀 설정
     */
    TURBIDITY_LED_DDR |= (1 << TURBIDITY_LED_PIN);

    WATER_LED_DDR |= (1 << WATER_GREEN_LED_PIN);
    WATER_LED_DDR |= (1 << WATER_YELLOW_LED_PIN);
    WATER_LED_DDR |= (1 << WATER_RED_LED_PIN);

    RAIL_SIGNAL_DDR |= (1 << RAIL_SIGNAL_PIN);

    /*
     * 초기 출력 상태
     */
    TurbidityLed_Off();
    WaterStatusLed_AllOff();
    RailSignal_Off();

    /*
     * 내부 상태 초기화
     */
    turbidityFsmState = TURBIDITY_STATE_IDLE;

    currentWaterState = WATER_STATE_GOOD;
    currentCleanRequest = CLEAN_REQUEST_OFF;

    turbidityValue = 0;
    ambientValue = 0;
    ledOnAverageValue = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        measureTimerMs = 0;

        firstMeasureRequestFlag = TRUE;
        manualMeasureRequestFlag = FALSE;
        afterFeedingCheckRequestFlag = FALSE;
    }
}

void Turbidity_Update(void)
{
    switch (turbidityFsmState)
    {
        case TURBIDITY_STATE_IDLE:
        {
            /*
             * IDLE:
             * 측정 시작 조건을 기다리는 상태
             */
            if (Turbidity_IsMeasureStartCondition() == TRUE)
            {
                turbidityFsmState = TURBIDITY_STATE_RUN;
            }
            break;
        }

        case TURBIDITY_STATE_RUN:
        {
            /*
             * RUN:
             * LED OFF 주변광 측정
             * LED ON 평균값 측정
             * 주변광 보정 후 빛 통과량 계산
             */
            turbidityValue = Turbidity_MeasureLightAmount();

            turbidityFsmState = TURBIDITY_STATE_DECIDE;
            break;
        }

        case TURBIDITY_STATE_DECIDE:
        {
            /*
             * DECIDE:
             * 측정값으로 수질 상태 판단
             */
            Turbidity_DecideWaterState();

            turbidityFsmState = TURBIDITY_STATE_OUTPUT;
            break;
        }

        case TURBIDITY_STATE_OUTPUT:
        {
            /*
             * OUTPUT:
             * LED, cleanRequest, 레일 신호 갱신
             */
            Turbidity_UpdateOutput();

            turbidityFsmState = TURBIDITY_STATE_DONE;
            break;
        }

        case TURBIDITY_STATE_DONE:
        {
            /*
             * DONE:
             * 플래그와 타이머 정리 후 IDLE 복귀
             */
            Turbidity_ClearRequestFlagsAndTimer();

            turbidityFsmState = TURBIDITY_STATE_IDLE;
            break;
        }

        default:
        {
            /*
             * 비정상 상태가 들어오면 안전하게 IDLE로 복귀
             */
            turbidityFsmState = TURBIDITY_STATE_IDLE;
            break;
        }
    }
}

void Turbidity_TimerTick1ms(void)
{
    /*
     * Timer0 ISR에서 1ms마다 호출됩니다.
     *
     * 측정 주기 이상으로 계속 증가할 필요는 없으므로
     * 최대값 근처에서 증가를 멈춥니다.
     */
    if (measureTimerMs < TURBIDITY_MEASURE_INTERVAL_MS)
    {
        measureTimerMs++;
    }
}

void Turbidity_RequestMeasure(void)
{
    /*
     * 버튼 등 외부 입력으로 수동 측정을 요청할 때 사용합니다.
     */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        manualMeasureRequestFlag = TRUE;
    }
}

void Turbidity_RequestAfterFeedingCheck(void)
{
    /*
     * 먹이 배급 후 일정 시간이 지난 뒤 측정을 요청할 때 사용합니다.
     */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        afterFeedingCheckRequestFlag = TRUE;
    }
}

WaterState Turbidity_GetWaterState(void)
{
    return currentWaterState;
}

CleanRequestState Turbidity_GetCleanRequest(void)
{
    return currentCleanRequest;
}

TurbidityFsmState Turbidity_GetFsmState(void)
{
    return turbidityFsmState;
}

uint16_t Turbidity_GetValue(void)
{
    return turbidityValue;
}

uint16_t Turbidity_GetAmbientValue(void)
{
    return ambientValue;
}

uint16_t Turbidity_GetLedOnAverageValue(void)
{
    return ledOnAverageValue;
}

void Turbidity_GetData(TurbidityData *data)
{
    if (data == 0)
    {
        return;
    }

    data->waterState = currentWaterState;
    data->cleanRequest = currentCleanRequest;
    data->fsmState = turbidityFsmState;

    data->turbidityValue = turbidityValue;
    data->ambientValue = ambientValue;
    data->ledOnAverageValue = ledOnAverageValue;
}

/*
 * ============================================================
 * FSM 내부 함수
 * ============================================================
 */

static uint8_t Turbidity_IsMeasureStartCondition(void)
{
    uint32_t timerCopy;
    uint8_t firstFlagCopy;
    uint8_t manualFlagCopy;
    uint8_t afterFeedingFlagCopy;

    /*
     * ISR과 공유하는 값들을 원자적으로 복사합니다.
     */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        timerCopy = measureTimerMs;
        firstFlagCopy = firstMeasureRequestFlag;
        manualFlagCopy = manualMeasureRequestFlag;
        afterFeedingFlagCopy = afterFeedingCheckRequestFlag;
    }

    if (firstFlagCopy == TRUE)
    {
        return TRUE;
    }

    if (timerCopy >= TURBIDITY_MEASURE_INTERVAL_MS)
    {
        return TRUE;
    }

    if (manualFlagCopy == TRUE)
    {
        return TRUE;
    }

    if (afterFeedingFlagCopy == TRUE)
    {
        return TRUE;
    }

    return FALSE;
}

static uint16_t Turbidity_MeasureLightAmount(void)
{
    uint32_t sum = 0;

    /*
     * 1. 측정 LED OFF
     * 주변광 기준값 측정
     */
    TurbidityLed_Off();
    _delay_ms(TURBIDITY_LED_STABLE_TIME_MS);

    ambientValue = ADC_Read(TURBIDITY_ADC_CHANNEL);

    /*
     * 2. 측정 LED ON
     * LED 빛이 물을 통과한 뒤 조도센서로 들어옴
     */
    TurbidityLed_On();
    _delay_ms(TURBIDITY_LED_STABLE_TIME_MS);

    /*
     * 3. 조도센서 값 10회 측정
     */
    for (uint8_t i = 0; i < TURBIDITY_ADC_SAMPLE_COUNT; i++)
    {
        sum += ADC_Read(TURBIDITY_ADC_CHANNEL);
        _delay_ms(TURBIDITY_ADC_SAMPLE_DELAY_MS);
    }

    /*
     * 4. 평균값 계산
     */
    ledOnAverageValue = (uint16_t)(sum / TURBIDITY_ADC_SAMPLE_COUNT);

    /*
     * 5. 측정 LED OFF
     */
    TurbidityLed_Off();

    /*
     * 6. 주변광 보정
     *
     * LED ON 값 = 주변광 + LED 통과빛
     * LED OFF 값 = 주변광
     *
     * 통과광 값 = LED ON 평균값 - 주변광 기준값
     */
#if LIGHT_INCREASES_ADC

    if (ledOnAverageValue > ambientValue)
    {
        return ledOnAverageValue - ambientValue;
    }
    else
    {
        return 0;
    }

#else

    if (ambientValue > ledOnAverageValue)
    {
        return ambientValue - ledOnAverageValue;
    }
    else
    {
        return 0;
    }

#endif
}

static void Turbidity_DecideWaterState(void)
{
    /*
     * 빛 통과량이 크면 물이 맑은 상태,
     * 빛 통과량이 작으면 물이 탁한 상태로 판단합니다.
     */
    if (turbidityValue >= TURBIDITY_GOOD_THRESHOLD)
    {
        currentWaterState = WATER_STATE_GOOD;
        currentCleanRequest = CLEAN_REQUEST_OFF;
    }
    else if (turbidityValue >= TURBIDITY_NORMAL_THRESHOLD)
    {
        currentWaterState = WATER_STATE_NORMAL;
        currentCleanRequest = CLEAN_REQUEST_OFF;
    }
    else
    {
        currentWaterState = WATER_STATE_BAD;
        currentCleanRequest = CLEAN_REQUEST_ON;
    }
}

static void Turbidity_UpdateOutput(void)
{
    WaterStatusLed_AllOff();

    if (currentWaterState == WATER_STATE_GOOD)
    {
        WaterStatusLed_GreenOn();
        RailSignal_Off();
    }
    else if (currentWaterState == WATER_STATE_NORMAL)
    {
        WaterStatusLed_YellowOn();
        RailSignal_Off();
    }
    else
    {
        WaterStatusLed_RedOn();
        RailSignal_On();
    }
}

static void Turbidity_ClearRequestFlagsAndTimer(void)
{
    /*
     * 측정 완료 후 요청 플래그와 타이머를 정리합니다.
     *
     * ISR과 공유하는 값이므로 ATOMIC_BLOCK 사용.
     */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        firstMeasureRequestFlag = FALSE;
        manualMeasureRequestFlag = FALSE;
        afterFeedingCheckRequestFlag = FALSE;

        measureTimerMs = 0;
    }
}

/*
 * ============================================================
 * ADC 관련 함수
 * ============================================================
 */

static void ADC_Init(void)
{
    /*
     * ADMUX
     *
     * REFS0 = 1
     * 기준전압을 AVCC로 사용합니다.
     *
     * 5V 기준:
     * 0V → ADC 0
     * 5V → ADC 1023
     */
    ADMUX = (1 << REFS0);

    /*
     * ADCSRA
     *
     * ADEN = 1
     * ADC 활성화
     *
     * ADPS2, ADPS1, ADPS0 = 1
     * 분주비 128
     *
     * 16MHz 기준:
     * ADC 클럭 = 16MHz / 128 = 125kHz
     */
    ADCSRA = (1 << ADEN)
           | (1 << ADPS2)
           | (1 << ADPS1)
           | (1 << ADPS0);
}

static uint16_t ADC_Read(uint8_t channel)
{
    /*
     * ADC 채널 선택
     *
     * ADMUX 상위 비트의 기준전압 설정은 유지하고,
     * 하위 MUX 비트만 채널 번호로 변경합니다.
     */
    ADMUX = (ADMUX & 0xE0) | (channel & 0x1F);

    /*
     * ADC 변환 시작
     */
    ADCSRA |= (1 << ADSC);

    /*
     * 변환 완료 대기
     */
    while (ADCSRA & (1 << ADSC));

    /*
     * 10비트 ADC 결과 반환
     */
    return ADC;
}

/*
 * ============================================================
 * 측정 LED 제어
 * ============================================================
 */

static void TurbidityLed_On(void)
{
    TURBIDITY_LED_PORT |= (1 << TURBIDITY_LED_PIN);
}

static void TurbidityLed_Off(void)
{
    TURBIDITY_LED_PORT &= ~(1 << TURBIDITY_LED_PIN);
}

/*
 * ============================================================
 * 상태 LED 제어
 * ============================================================
 */

static void WaterStatusLed_AllOff(void)
{
    WATER_LED_PORT &= ~(1 << WATER_GREEN_LED_PIN);
    WATER_LED_PORT &= ~(1 << WATER_YELLOW_LED_PIN);
    WATER_LED_PORT &= ~(1 << WATER_RED_LED_PIN);
}

static void WaterStatusLed_GreenOn(void)
{
    WATER_LED_PORT |= (1 << WATER_GREEN_LED_PIN);
}

static void WaterStatusLed_YellowOn(void)
{
    WATER_LED_PORT |= (1 << WATER_YELLOW_LED_PIN);
}

static void WaterStatusLed_RedOn(void)
{
    WATER_LED_PORT |= (1 << WATER_RED_LED_PIN);
}

/*
 * ============================================================
 * 레일 동작신호 제어
 * ============================================================
 */

static void RailSignal_On(void)
{
    RAIL_SIGNAL_PORT |= (1 << RAIL_SIGNAL_PIN);
}

static void RailSignal_Off(void)
{
    RAIL_SIGNAL_PORT &= ~(1 << RAIL_SIGNAL_PIN);
}