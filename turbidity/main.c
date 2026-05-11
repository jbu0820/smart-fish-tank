#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

/*
 * ============================================================
 * 측정 설정값
 * ============================================================
 */
#define LED_ON_WAIT_MS        2000
#define LED_OFF_WAIT_MS       1000
#define ADC_SAMPLE_COUNT      30
#define ADC_SAMPLE_DELAY_MS   5

/*
 * ============================================================
 * 수질 판단 기준값
 * ============================================================
 *
 * 실험값 기준:
 * 맑은 물: DIFF 약 90~100
 * 탁한 물: DIFF 약 30~40
 */
#define TURBIDITY_GOOD_THRESHOLD       80
#define TURBIDITY_NORMAL_THRESHOLD     50

/*
 * ============================================================
 * RGB LED 모듈 핀 설정
 * ============================================================
 *
 * RGB LED 모듈:
 * -  → GND
 * R  → PB1
 * G  → PB2
 * B  → PB0
 */
#define TURBIDITY_LED_DDR     DDRB
#define TURBIDITY_LED_PORT    PORTB

#define TURBIDITY_LED_PIN_R   PB1
#define TURBIDITY_LED_PIN_G   PB2
#define TURBIDITY_LED_PIN_B   PB0

#define TURBIDITY_LED_MASK    ((1 << TURBIDITY_LED_PIN_R) | \
                               (1 << TURBIDITY_LED_PIN_G) | \
                               (1 << TURBIDITY_LED_PIN_B))

/*
 * 조도센서: PF0 / ADC0
 */
#define LIGHT_SENSOR_CHANNEL  0

static void UART0_Init(void)
{
    UBRR0H = 0;
    UBRR0L = 103;          // 16MHz, 9600bps

    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

static void UART0_SendChar(char data)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

static void UART0_PrintString(const char *str)
{
    while (*str)
    {
        UART0_SendChar(*str);
        str++;
    }
}

static void UART0_PrintNumber(uint16_t num)
{
    char buffer[6];
    int i = 0;

    if (num == 0)
    {
        UART0_SendChar('0');
        return;
    }

    while (num > 0)
    {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    while (i > 0)
    {
        UART0_SendChar(buffer[--i]);
    }
}

static void ADC_Init(void)
{
    /*
     * AVCC를 ADC 기준전압으로 사용
     */
    ADMUX = (1 << REFS0);

    /*
     * ADC Enable, Prescaler 128
     * 16MHz / 128 = 125kHz
     */
    ADCSRA = (1 << ADEN)
           | (1 << ADPS2)
           | (1 << ADPS1)
           | (1 << ADPS0);
}

static uint16_t ADC_Read(uint8_t channel)
{
    /*
     * REFS0 설정은 유지하고 ADC 채널만 변경
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

    return ADC;
}

static uint16_t ADC_ReadAverage(uint8_t channel)
{
    uint32_t sum = 0;

    for (uint8_t i = 0; i < ADC_SAMPLE_COUNT; i++)
    {
        sum += ADC_Read(channel);
        _delay_ms(ADC_SAMPLE_DELAY_MS);
    }

    return (uint16_t)(sum / ADC_SAMPLE_COUNT);
}

static void TurbidityLed_On(void)
{
    TURBIDITY_LED_PORT |= TURBIDITY_LED_MASK;
}

static void TurbidityLed_Off(void)
{
    TURBIDITY_LED_PORT &= ~TURBIDITY_LED_MASK;
}

static void PrintWaterState(uint16_t diffValue)
{
    if (diffValue >= TURBIDITY_GOOD_THRESHOLD)
    {
        UART0_PrintString("STATE=GOOD");
    }
    else if (diffValue >= TURBIDITY_NORMAL_THRESHOLD)
    {
        UART0_PrintString("STATE=NORMAL");
    }
    else
    {
        UART0_PrintString("STATE=BAD CLEAN_REQUEST=1");
    }
}

int main(void)
{
    uint16_t ledOffValue;
    uint16_t ledOnValue;
    uint16_t diffValue;

    /*
     * RGB LED 핀 출력 설정
     */
    TURBIDITY_LED_DDR |= TURBIDITY_LED_MASK;

    /*
     * 초기 LED OFF
     */
    TurbidityLed_Off();

    UART0_Init();
    ADC_Init();

    UART0_PrintString("Turbidity flowchart test start\r\n");
    UART0_PrintString("Flow: LED OFF -> ambient -> LED ON -> measure -> DIFF\r\n\r\n");

    while (1)
    {
        /*
         * ========================================================
         * 1. 측정 LED OFF
         * ========================================================
         */
        TurbidityLed_Off();
        UART0_PrintString("LED OFF\r\n");

        /*
         * 주변광이 안정될 때까지 대기
         */
        _delay_ms(LED_OFF_WAIT_MS);

        /*
         * ========================================================
         * 2. 주변광 기준값 측정
         * ========================================================
         */
        ledOffValue = ADC_ReadAverage(LIGHT_SENSOR_CHANNEL);

        /*
         * ========================================================
         * 3. 측정 LED ON
         * ========================================================
         */
        TurbidityLed_On();
        UART0_PrintString("LED ON\r\n");

        /*
         * LED 빛이 안정될 때까지 대기
         */
        _delay_ms(LED_ON_WAIT_MS);

        /*
         * ========================================================
         * 4. LED ON 상태 조도값 측정
         * ========================================================
         */
        ledOnValue = ADC_ReadAverage(LIGHT_SENSOR_CHANNEL);

        /*
         * ========================================================
         * 5. 측정 LED OFF
         * ========================================================
         */
        TurbidityLed_Off();

        /*
         * ========================================================
         * 6. 빛 통과량 계산
         *
         * DIFF = LED ON 평균값 - LED OFF 주변광 기준값
         * ========================================================
         */
        if (ledOnValue > ledOffValue)
        {
            diffValue = ledOnValue - ledOffValue;
        }
        else
        {
            diffValue = 0;
        }

        /*
         * ========================================================
         * 7. 결과 출력
         * ========================================================
         */
        UART0_PrintString("OFF=");
        UART0_PrintNumber(ledOffValue);

        UART0_PrintString(" ON=");
        UART0_PrintNumber(ledOnValue);

        UART0_PrintString(" DIFF=");
        UART0_PrintNumber(diffValue);

        UART0_PrintString(" ");
        PrintWaterState(diffValue);

        UART0_PrintString("\r\n\r\n");

        _delay_ms(1000);
    }
}
