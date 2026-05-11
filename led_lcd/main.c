// main.c
//#include "led.h"
//#include "button.h"
//
//#include <avr/io.h>
//#include <util/delay.h>
//#include <stdio.h>
//#include <stdint.h>
//#include <stdbool.h>
//#include <stdarg.h>
//#include <stdlib.h
/*=======================================================
프로젝트 (스마트 어항)
========================================================*/
#include "i2c_lcd.h"
#include "rgb_led.h"
#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h>

char rx_field[10];
uint8_t rx_idx = 0;

void UART0_Init(void) {
    UCSR0A |= (1 << U2X0);
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0); 
    UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);
    UBRR0H = 0;
    UBRR0L = 207;                           
}

int main(void) {
    I2C_Init();
    LCD_Init();
    LED_Init();
    UART0_Init();

    int sensor_value = 0;
    SystemStatus currentStatus = STATUS_GOOD;
    SystemStatus lastStatus = 99;
    
    uint16_t blink_cnt = 0;
    uint8_t blink_state = 0;

    while (1) {
        // --- [핵심] 비차단 수신 체크 ---
        if (UCSR0A & (1 << RXC0)) { // 수신된 데이터가 "지금" 있다면
            char c = UDR0;          // 딱 한 글자 읽기
            
            if (c == '\n' || c == '\r') { // 엔터가 들어오면
                if (rx_idx > 0) {
                    rx_field[rx_idx] = '\0';
                    sensor_value = atoi(rx_field); // 숫자 변환
                    
                    // 상태 판정
                    if (sensor_value <= 30) currentStatus = STATUS_GOOD;
                    else if (sensor_value <= 70) currentStatus = STATUS_CHECK;
                    else currentStatus = STATUS_WARNING;
                    
                    rx_idx = 0; // 버퍼 초기화
                }
            } else if (rx_idx < 9) { // 글자 쌓기
                rx_field[rx_idx++] = c;
            }
        }

        // --- 여기서부터는 수신 여부와 상관없이 계속 실행됨 ---

        if (currentStatus == STATUS_WARNING) {
            LED_SetStatus(STATUS_WARNING);
            _delay_ms(1);
            if (++blink_cnt >= 500) {
                blink_cnt = 0;
                blink_state = !blink_state;
                LCD_Clear();
                if (blink_state) {
                    LCD_SetCursor(0, 0); LCD_WriteString("!! WARNING !!");
                    LCD_SetCursor(1, 0); LCD_WriteString("Check Water Tank");
                }
            }
            lastStatus = STATUS_WARNING;
        } 
        else if (currentStatus == STATUS_CHECK) {
            LED_SetStatus(STATUS_CHECK);
            if (lastStatus != STATUS_CHECK) {
                LCD_Clear();
                LCD_SetCursor(0, 0); LCD_WriteString("Water: CLOUDY");
                LCD_SetCursor(1, 0); LCD_WriteString("System Operating");
                lastStatus = STATUS_CHECK;
            }
        } 
        else { // STATUS_GOOD
            LED_SetStatus(STATUS_GOOD);
            if (lastStatus != STATUS_GOOD) {
                LCD_Clear();
                LCD_SetCursor(0, 0); LCD_WriteString("Water: CLEAN");
                LCD_SetCursor(1, 0); LCD_WriteString("Next Feed: 18:00");
                lastStatus = STATUS_GOOD;
            }
        }
    }
}
/*========================================================
//UART통신
//======================================================*/
//void uart0_init()
//{
//    UCSR0A |= (1<<U2X0);                    // 2배속 모드
//    UCSR0B |= (1<<RXEN0) | (1<<TXEN0);      // 수신 가능, 송신 가능
//    UCSR0C |= (1<<UCSZ01) | (1<<UCSZ00);    // 8비트 모드, 패리티비트 없음, 스톱비트 1비트
//    UBRR0H = 0;
//    UBRR0L = 207;                           // 9600bps
//}
//
//void uart0_transmit(char data)
//{
//    while(!(UCSR0A & (1<<UDRE0)));  // 송신 가능하냐?, UDR이 비어있느냐?
//    UDR0 = data;
//}
//
//unsigned uart0_recevie(void)
//{
//    while(!(UCSR0A & (1<<RXC0)));   // 수신대기
//    return UDR0;
//}
//
//
//int main()
//{
//    uart0_init();
//
//    while (1)
//    {
//        uart0_transmit(uart0_recevie());
//    }    
//}
/*========================================================
//FND로 7-세그먼트만들기
========================================================*/
//int main()
//{
//    uint8_t fndNumber[]=
//    {
//        0x3f,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x27,0x7F,0x67
//    };
//
//    int count = 0;
//
//    DDRC = 0xFF;    //FND를 연결한 포드
//
//    while(1)
//    {
//        PORTC = fndNumber[count];
//        count = (count +1) %10;
//
//        _delay_ms(500);
//    }
//}
/*========================================================
//주파수확인(250HZ확인)PWM Mode-> 서보모터 제어
========================================================*/
//int main()
//{
//    DDRB |= (1<<PB5); //출력
//    TCCR1A |= (1<<COM1A1) |(1<<WGM11);
//    TCCR1B |= (1<<WGM13) | (1<<WGM12) | (1<<CS11) | (1<<CS10);
//    TCCR1C = 0;
//
//    OCR1A = 1249;    //25%
//    ICR1 = 4999;
//
//    while (1)
//    {
//        OCR1A = 600;
//        _delay_ms(1000);
//        OCR1A = 375;
//        _delay_ms(1000);
//        OCR1A = 150;
//        _delay_ms(1000);
//        OCR1A = 375;
//        _delay_ms(1000);
//    }
//    
//}
/*========================================================
//주파수확인(250HZ확인)PWM Mode-> 오른쪽으로 움직임
========================================================*/
//int main()
//{
//    DDRB |= (1<<PB5); //출력
//    TCCR1A |= (1<<COM1A1) |(1<<WGM11);
//    TCCR1B |= (1<<WGM13) | (1<<WGM12) | (1<<CS11) | (1<<CS10);
//    TCCR1C = 0;
//
//    OCR1A = 1249;    //25%
//    ICR1 = 4999;
//
//    while (1)
//    {
//       
//    }
//    
//}
/*========================================================
//주파수확인(250HZ확인)PWM Mode-> 오른쪽으로 움직임
========================================================*/
//int main(){
//
//    DDRB = 0xFF;            // 출력으로 설정
//    DDRB |= (1<< PB4); // PB4를 출력으로 설정
//
//    TCCR0 |= (1<< WGM00) |(1<<COM01) | (1 << WGM01) | (1 << CS02); // Fast PWM 모드, 비반전 모드, 분주비 64 
//
//    OCR0 = 64;
//
//    while(1){
//       for(uint8_t i =0 ; i <= 255; i++){
//        OCR0 = i;
//        _delay_ms(10);
//       }
//        
//    }
//}
/*========================================================
//주파수확인(250HZ확인)PWM Mode
========================================================*/
//int main(){
//
//    DDRB = 0xFF;            // 출력으로 설정
//    DDRB |= (1<< PB4); // PB4를 출력으로 설정
//
//    TCCR0 |= (1<< WGM00) |(1<<COM01) | (1 << WGM01) | (1 << CS02); // Fast PWM 모드, 비반전 모드, 분주비 64 
//
//    OCR0 = 64;
//
//    while(1)
//    {
//       
//    
//        
//    }
//}
/*========================================================
//주파수확인(250HZ확인)Normal Mode
========================================================*/
//int main()
//{
//    DDRB = 0xff;    //이쪽으로 출력
//    PORTB = 0;
//    TCCR0 |= (1<<CS02) | (1<<CS00); //Normal Mode
//    TCNT0 = 6;
//    while(1)
//    {
//        while ((TIFR &0x01) == 0);
//        PORTB = ~PORTB;
//        TCNT0  = 6;
//        TIFR = 0x01;
//    } 
//}
/*========================================================
//주파수확인(125HZ확인)CTC Mode
========================================================*/
//int main()
//{
//    //DDRB = 0x10;            //0b 00010000 -> PB4를 출력으로 설정
//    DDRB |= (1<<PB4);
//    //TCCR0 = 0b00011100;     //0x1C
//    TCCR0 |= (1<<COM00) |(1<<WGM01) | (1<<CS02);
//
//    OCR0 = 124;
//    while (1)
//    {
//        while ((TIFR &0x02) == 0);      //0인지 1인지 확인하는방법->(TIFR &OCF0)
//        {
//            TIFR = 0x02;
//            OCR0 =124;
//        }
//            
//    }
//    
//}
/*========================================================
//주파수확인(250HZ확인)CTC Mode
========================================================*/
//int main()
//{
//    //DDRB = 0x10;            //0b 00010000 -> PB4를 출력으로 설정
//    DDRB |= (1<<PB4);
//    //TCCR0 = 0b00011100;     //0x1C
//    TCCR0 |= (1<<COM00) |(1<<WGM01) | (1<<CS02) | (1<<CS00);
//
//    OCR0 = 249;
//    while (1)
//    {
//        while ((TIFR &0x02) == 0);      //0인지 1인지 확인하는방법->(TIFR &OCF0)
//        {
//            TIFR = 0x02;        //TIFR초기화
//            OCR0 =249;
//        }
//            
//    }
//    
//}
/*========================================================
//button.c,button.h를 활용한 버튼
========================================================*/
//int main()
//{
//    LED_DDR = 0xFF;     // LED Bar 출력 설정
//
//    BUTTON btnOn;
//    BUTTON btnOff;
//    BUTTON btnTog;
//
//    ButtonInit(&btnOn, &BUTTON_DDR, &BUTTON_PIN, BUTTON_ON);
//    ButtonInit(&btnOff, &BUTTON_DDR, &BUTTON_PIN, BUTTON_OFF);
//    ButtonInit(&btnTog, &BUTTON_DDR, &BUTTON_PIN, BUTTON_TOGGLE);
//
//    while (1)
//    {
//        if(ButtonGetState(&btnOn) == ACT_RELEASE)
//        {
//            LED_PORT = 0xff;
//        }
//        if(ButtonGetState(&btnOff) == ACT_RELEASE)
//        {
//            LED_PORT = 0x00;
//        } 
//        if(ButtonGetState(&btnTog) == ACT_RELEASE)
//        {
//            LED_PORT ^= 0xff;
//        }
//    }    
//}
/*========================================================
//
========================================================*/
//레지스터 설정
//#define LED_DDR         DDRD
//#define LED_PORT        PORTD
//#define BUTTON_DDR      DDRG
//#define BUTTON_PIN      PING
//
////pinnum는 결국 buton num
//#define BUTTON_ON       2
//#define BUTTON_OFF      3
//#define BUTTON_TOGGLE   4
//
//enum {PUSHED, RELEASED};
//enum 
//{
//    NO_ACT, 
//    ACT_PUSH, 
//    ACT_RELEASE
//};
//
//typedef struct
//{
//    volatile uint8_t    *ddr;
//    volatile uint8_t    *pin;
//    uint8_t             btnPin;
//    uint8_t             prevState;
//}BUTTON;
//
//void ButtonInit(BUTTON *button, volatile uint8_t *ddr, volatile uint8_t *pin, uint8_t pinNum)
//{
//    button->ddr = ddr;
//    button->pin = pin;
//    button->btnPin = pinNum;
//    button->prevState = RELEASED;               // 초기화 상태로 아무것도 안누른 상태
//    *button->ddr &= ~(1 << button->btnPin);     // 버튼에 대한 핀을 입력으로 설정
//}
//
//uint8_t ButtonGetState(BUTTON *button)
//{
//    uint8_t curState = *button->pin & (1 << button->btnPin);    // 현재 버튼의 상태를 읽어옴
//
//    if((curState == PUSHED) && (button->prevState == RELEASED))     // 안누른 상태에서 누르면...
//    {
//        _delay_ms(50);                  // debounced code
//        button->prevState = PUSHED;     // 버튼을 누른 상태로 변환
//        return ACT_PUSH;                // 버튼을 눌렀다고 반환
//    }
//    else if((curState != PUSHED) && (button->prevState == PUSHED)) // 누른상태에서 떼면??
//    {
//        _delay_ms(50);
//        button->prevState = RELEASED;   // 버튼을 뗀 상태로 변환
//        return ACT_RELEASE;             // 버튼을 떼었다고 반환
//    }
//    return NO_ACT;                      // 아무것도 안했다고 반환
//}
//
//
//int main()
//{
//    LED_DDR = 0xFF;     // LED Bar 출력 설정
//
//    BUTTON btnOn;
//    BUTTON btnOff;
//    BUTTON btnTog;
//
//    ButtonInit(&btnOn, &BUTTON_DDR, &BUTTON_PIN, BUTTON_ON);
//    ButtonInit(&btnOff, &BUTTON_DDR, &BUTTON_PIN, BUTTON_OFF);
//    ButtonInit(&btnTog, &BUTTON_DDR, &BUTTON_PIN, BUTTON_TOGGLE);
//
//    while (1)
//    {
//        if(ButtonGetState(&btnOn) == ACT_RELEASE)
//        {
//            LED_PORT = 0xff;
//        }
//        if(ButtonGetState(&btnOff) == ACT_RELEASE)
//        {
//            LED_PORT = 0x00;
//        } 
//        if(ButtonGetState(&btnTog) == ACT_RELEASE)
//        {
//            LED_PORT ^= 0xff;
//        }
//    }    
//}
/*========================================================
//버튼을 누루면 좌우로 한칸씩 이동하는코드
========================================================*/
//int main()
//{
//    DDRD = 0xff; // LED바가 연결된 포트를 출력으로 설정
//    DDRG = 0x00; // 버튼이 연결된 포트 G 전체를 입력으로 설정
//
//    uint8_t ledData = 0x01; // main함수에 종속된 지역변수를 선언
//
//    // 버튼을 입력받는 입력값 때문에 초기화를 암함!!
//    uint8_t buttonData; // 버튼값을 입력받을 main에 종속된 변수를 선언
//
//    // int flag = 0;
//    PORTD = 0x00; // LED바의 포트를 꺼진 상태로 출발
//
//    while (1)
//    {
//        buttonData = PING;
//
//        if ((buttonData & (1 << 2)) == 0)
//        {
//            ledData = (ledData >> 7) | (ledData << 1);
//            PORTD = ledData;
//            _delay_ms(200);
//        }
//        if ((buttonData & (1 << 3)) == 0)
//        {
//            ledData = (ledData >> 1) | (ledData << 7);
//            PORTD = ledData;
//            _delay_ms(200);
//        }
//        if ((buttonData & (1 << 4)) == 0)
//        {
//            PORTD = 0x00;
//        }
//    }
//}

///@brief 입력 버튼 1개를 PING4번핀만 받음
///@return
/*========================================================
//버튼을 누루면 LED켜지는 코드
========================================================*/
//int main()
//{
//     DDRD= 0xff;         //LED바가 연결된 포트 (출력 포트 설정)
//
//     DDRG &= ~(1<<4);    //DDRG의 4번에 연결된 포트를 입력으로 설정
//
//     while(1)
//     {
//         if (PING &(1 << 4)) //PORTG의 4번핀이 Hight일때(입력:버튼)
//         {
//             PORTD = 0x00;   //LED
//         }
//         else
//         {
//             PORTD = 0xFF;   //LED
//         }
//
//     }
// }
/*========================================================
//
========================================================*/
// int main()
//{
//     LED led;        // LED라는 구조를 가진 led변수 선언
//     led.port = &PORTD;
//     led.pin = 0;
//
//     for (uint8_t i = 0; i < 8; i++)
//     {
//         led.pin = i;
//         ledInit(&led);
//     }
//
//     //ledInit(&led);
//     //DDRD = 0xff;
//
//     while (1)
//     {
//         ledOn(&led);
//         _delay_ms(300);
//
//         ledOff(&led);
//
//         led.pin++;
//
//         if (led.pin > 7)
//         {
//             led.pin = 0;
//         }
//
//         _delay_ms(200);
//     }
// }
/*========================================================
//LED구조체을 이용한 것(led.h,led.c)
========================================================*/
// int main()
//{
//     LED led;        //LED라는 구조를 가진 led변수 선언
//     led.port = &PORTD;
//     led.pin = 0;
//
//     ledInit(&led);
//
//     while (1)
//     {
//         ledOn(&led);
//         _delay_ms(200);
//         ledOff(&led);
//         _delay_ms(200);
//     }
// }
/*========================================================
//LED배열을 이용한 것
========================================================*/
// uint8_t ledArr[]=
//{
//     0x00,
//     0x80,   // 1000 0000
//     0xC0,   // 1100 0000
//     0xE0,   // 1110 0000
//     0xF0,   // 1111 0000
//     0xF8,   // 1111 1000
//     0xFC,   // 1111 1100
//     0xFE,   // 1111 1110
//     0xFF,   // 1111 1111
//     0x7F,   // 0111 1111
//     0x3F,   // 0011 1111
//     0x1F,   // 0001 1111
//     0x0F,   // 0000 1111
//     0x07,   // 0000 0111
//     0x03,   // 0000 0011
//     0x01    // 0000 0001
// };
//
// int main()
//{
//     DDRD = 0xFF;
//
//     uint8_t arrSize = sizeof(ledArr)/sizeof(ledArr[0]);
//
//     while (1)
//     {
//         for (uint8_t i = 0; i < arrSize; i++)
//         {
//             PORTD = ledArr[i];
//             _delay_ms(200);
//         }
//     }
// }
/*========================================================
포인터로이용한 LED
========================================================*/
// #define LED_DDR     DDRD
// #define LED_PORT    PORTD
//
//// C언어는 내가 만든 함수가 main함수보다 위에 있어야 함
//
//// LED 출력 함수
// void GPIO_Output(uint8_t data)
//{
//     LED_PORT = data;    // LED 포트에 주어진 data를 대입
// }
//
//// LED 초기화 함수
// void ledInit()
//{
//     LED_DDR = 0xFF;     // 포트 D에 있는 모든 핀을 출력으로 설정
// }
//
//// LED 를 좌우로 이동하는 함수
// void ledShift(uint8_t i, uint8_t *data)
//{
//    *data = (1 << i) | (1 << (7 - i));   // 좌우 방향에 해당하는 비트를 설정
// }
//
// int main()
//{
//     ledInit();                  // LED 초기화 함수를 호출
//     uint8_t ledData = 0x01;     // 초기 LED 데이터를 설정
//
//     while (1)
//     {
//         for (int i = 0; i < 8; i++)
//         {
//             ledShift(i, &ledData);  // 좌우 이동함수를 호출
//             GPIO_Output(ledData);   // LED출력 함수를 호출
//             _delay_ms(1000);        // 1초 지연
//         }
//     }
// }
/*========================================================
//LED기본움직임 (LED좌우 왔다갔다.)
========================================================*/
//int main()
//{
//     //DDRD = 0xff;//16진수로 대입
//     //DDRD = 0b11111111;//2진수로 대입
//     DDRD |= 0b11111111;//2진수로 대입 복합연산자
//     //DDRD = DDRD | 0b11111111;//위에꺼 풀어씀
//
//     while(1)
//     {
//         //PORTD = 0b11111111;
//         //_delay_ms(20);
//         //PORTD = 0x00;
//         //_delay_ms(20);
//         for (uint8_t i =0; i < 7; i++)
//         {
//             PORTD =(0b00000010 << i);
//             _delay_ms(200);
//         }
//         for (uint8_t i = 0; i < 7; i++)
//         {
//             PORTD = (0b01000000 >> i);
//             _delay_ms(200);
//         }
//     }
// }
