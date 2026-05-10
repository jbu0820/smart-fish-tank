// #include "button.h"


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "common.h"
#include "system_timer.h"
#include "turbidity.h"

int main(void)
{
    Turbidity_Init();
    SystemTimer_Init();

    sei();

    while (1)
    {
        Turbidity_Update();

        _delay_ms(10);
    }
}





// #include <avr/io.h>
// #include <util/delay.h>
// #include <stdint.h>

// #include "uart.h"

// static void ADC_Init(void)
// {
//     /*
//      * 기준전압 AVCC 사용
//      */
//     ADMUX = (1 << REFS0);

//     /*
//      * ADC 활성화, 분주비 128
//      */
//     ADCSRA = (1 << ADEN)
//            | (1 << ADPS2)
//            | (1 << ADPS1)
//            | (1 << ADPS0);
// }

// static uint16_t ADC_Read(uint8_t channel)
// {
//     /*
//      * ADC 채널 선택
//      * ADC0 = PF0
//      */
//     ADMUX = (ADMUX & 0xE0) | (channel & 0x1F);

//     /*
//      * 변환 시작
//      */
//     ADCSRA |= (1 << ADSC);

//     /*
//      * 변환 완료 대기
//      */
//     while (ADCSRA & (1 << ADSC));

//     return ADC;
// }

// int main(void)
// {
//     uint16_t adcValue;

//     UART0_Init();
//     ADC_Init();

//     UART0_PrintString("LDR ADC Test Start\r\n");

//     while (1)
//     {
//         adcValue = ADC_Read(0);   // PF0 / ADC0

//         UART0_PrintString("ADC = ");
//         UART0_PrintNumber(adcValue);
//         UART0_PrintString("\r\n");

//         _delay_ms(500);
//     }
// }















// void uart0_init()
// {
//     UCSR0A |= (1<<U2X0);                    // 2배속 모드 교수님사진 ucsrna부분
//     UCSR0B |= (1<<RXEN0) | (1<<TXEN0);      // 수신가능 송신가능
//     UCSR0C |= (1<<UCSZ01) | (1<<UCSZ00);    // 8비트 모드, 패리티비트 없음, 스톱비트1비트
//     UBRR0H = 0;
//     UBRR0L = 207;                           // 9600bps
// }

// void uart0_transmit(char data)
// {
//     while(!(UCSR0A & (1<<UDRE0)));          // 송신 가능한지, UDR이 비어있는지
//     UDR0 = data;
// }

// unsigned uart0_receive(void)
// {
//     while(!(UCSR0A & (1<<RXC0)));           // 수신대기
//     return UDR0;
// }

// int main()
// {
//     uart0_init();

//     while (1)
//     {
//         uart0_transmit(uart0_receive());
//     }
// }






// int main()
// {
//     uint8_t fndNumber[]=
//     {
//         0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x27, 0x7F, 0x67
//     };

//     int count = 0;

//     DDRC = 0xFF;    //FND를 연결한 포트

//     while(1)
//     {
//         PORTC = fndNumber[count];
//         count = (count + 1) % 10;
        
//         _delay_ms(500);
//     }
// }







// int main()
// {
//     DDRB |=(1<<PB5);     // 출력
//     TCCR1A |=(1<<COM1A1)|(1<<WGM11);
//     TCCR1B |=(1<<WGM13)|(1<<WGM12)|(1<<CS11)|(1<<CS10);
//     TCCR1C |= 0;

//     //OCR1A = 1249;   // 25%
//     ICR1 =4999;

//     while(1)
//     {
//         OCR1A = 620;
//         _delay_ms(2000);
//         OCR1A = 375;
//         _delay_ms(2000);
//         OCR1A = 150;
//         _delay_ms(2000);
//     }
// }







// int main()
// {
//     //DDRB = 0b00010000;
//     DDRB |= (1<<PB4);       // PB4를 출력으로 설정
//     TCCR0 |= (1<<WGM00)|(1<<COM01)|(1<<WGM01)|(1<<CS02);    // Fast PWM,비반전모드,분주비64

//     //OCR0 = 64;

//     while(1)
//     {
//         for (uint8_t i = 0; i <= 255; i++)
//         {
//             OCR0 = i;
//             _delay_ms(10);
//         }
//     }
// }




// int main()
// {
//     DDRB = 0xff;        // 이쪽으로 출력
//     PORTB = 0;
//     TCCR0 |= (1<<CS02) | (1<<CS00); //NORMAL MODE
//     TCNT0 = 6;

//     while(1)
//     {
//         while((TIFR & 0x01) ==0);
//         PORTB = ~PORTB;     //수동으로 토글 필요
//         TCNT0 = 6;
//         TIFR = 0x01;
//     }

// }




// CTC MODE
// int main()
// {
//     //DDRB = 0x10;            // 0b 00010000 -> PB4를 출력으로 설정 : 핀 사진을 참조
//     DDRB |= (1<<PB4);
//     //TCCR0 = 0b00011100;     // 0x1C;
//     TCCR0 |= (1<<COM00) | (1<<WGM01) | (1<<CS02) | (1<<CS00);   //레지스터 값에 상관없이 민 값으로 1을 만들수있음(or을 쓴 이유)
    
//     OCR0 = 249;

//     while(1)
//     {
//         while((TIFR & 0x02) == 0);  // TIFR 레지스터의 OCF0가 1인지0인지

//         TIFR = 0x02;
//         OCR0 = 249;
//     }
// }





// int main()
// {
//     LED_DDR = 0xFF;     // LED Bar 출력으로 설정
    
//     BUTTON btnOn;
//     BUTTON btnOff;
//     BUTTON btnTog;

//     ButtonInit(&btnOn, &BUTTON_DDR, &BUTTON_PIN, BUTTON_ON);
//     ButtonInit(&btnOff, &BUTTON_DDR, &BUTTON_PIN, BUTTON_OFF);
//     ButtonInit(&btnTog, &BUTTON_DDR, &BUTTON_PIN, BUTTON_TOGGLE);

//     while(1)
//     {
//         if(ButtonGetState(&btnOn) == ACT_RELEASE)
//         {
//             LED_PORT = 0xff;
//         }
//         if(ButtonGetState(&btnOff) == ACT_RELEASE)
//         {
//             LED_PORT = 0x00;
//         }
//         if(ButtonGetState(&btnTog) == ACT_RELEASE)
//         {
//             LED_PORT ^= 0xff; //(비트xor)
//         }
//     }
// }





// int main()
// {
//     DDRD = 0xFF;    // connencted LED bar port setting output
//     DDRG = 0x00;    //  버튼이 연결된 포트G 전체를 입력으로 설정
    
//     uint8_t ledData = 0x01;     //main함수에 종속된 함수 지역변수로 선언
    
//     // 버튼을 입력 받는 입력값 때문에 초기화를 안함!
//     uint8_t buttonData;      //버튼값을 입력받을 main에 종속된 변수를 선언
    
//     //int flag = 0;

//     PORTD =0x00;    //LED바의 포트를 꺼진 상태로 출발 

//     while (1)
//     {
//         buttonData = PING;  //buttonData에 핀의 입력을 읽어옴

//         if((buttonData & (1<<2)) == 0)
//         {
//             PORTD = ledData;
//             ledData = (ledData >> 7) | (ledData << 1);
//             _delay_ms(200);
//         }
        
//         if((buttonData & (1<<3)) == 0)
//         {
//             PORTD = ledData;
//             ledData = (ledData >> 1) | (ledData << 7);
//             _delay_ms(200);
//         }
//         if((buttonData & (1<<4)) == 0)
//         {
//             PORTD = 0x00;
//         }
//     }
// }




// /// @brief one input button PING 4pin only receive 
// /// @return 
// int main()
// {
//     DDRD = 0xff;        // LED linked port(output port setting)

//     DDRG &= ~(1<<4);    // 4pin of DDRG = input is setting 

//     while (1)
//     {
//         if(PING & (1 << 4)) // When PORTG 4pin is HIGH 
//         {
//             PORTD = 0x00;   // LED Off
//         }
//         else    // NOT TRUE O,  FALSE x
//         {
//             PORTD = 0xFF;   // LED On
//         }
//     }
// }