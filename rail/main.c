#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#define RELAY1 PB0
#define RELAY2 PB1

void relay_init(void) {
    DDRB |= (1 << RELAY1) | (1 << RELAY2);

    // 기본 OFF
    PORTB |= (1 << RELAY1);
    PORTB |= (1 << RELAY2);
}

void motor_stop(void) {
    PORTB &= ~(1 << RELAY1);
    PORTB &= ~(1 << RELAY2);
}

void motor_forward(void) {
    PORTB |= (1 << RELAY1);
    PORTB &= ~(1 << RELAY2);
}

void motor_reverse(void) {
    PORTB &= ~(1 << RELAY1);
    PORTB |= (1 << RELAY2);
}

// cleanRequest를 받아서 동작, 완료되면 done_sig = 1
void rail_run(uint8_t cleanRequest, uint8_t *done_sig) {
    if (done_sig == 0) {
        return;
    }

    *done_sig = 0;

    if (cleanRequest == 0) {
        return;
    }

    motor_forward();
    _delay_ms(500);

    motor_stop();
    _delay_ms(500);

    motor_reverse();
    _delay_ms(1000);

    motor_stop();
    _delay_ms(500);

    *done_sig = 1;
}

int main(void) {
    uint8_t cleanRequest = 0;
    uint8_t done_sig = 0;

    relay_init();

    while (1) {
        // 예시: 상위 main에서 cleanRequest를 1로 줌
        cleanRequest = 1;

        rail_run(cleanRequest, &done_sig);

        if (done_sig == 1) {
            cleanRequest = 0;
        }

        _delay_ms(100);
    }
}

// #include <avr/io.h>
// #include <util/delay.h>

// void uart0_init(){

//     UCSR0A |= (1 << U2X0);                          // U2X mode set
//     UCSR0B |= (1 << RXEN0) | (1 << TXEN0);          // Set enable recieve and transmit 
//     UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);        // Set 8bits mode, no parity bit, one stop bit
//     UBRR0H = 0; 
//     UBRR0L = 207;                                   // bitrates : 9600pbs
// }


// void uart0_transmit(char data){

//     while(!(UCSR0A & (1 << UDRE0)));                // 송신이 가능한지, UDR이 비어있는지
//     UDR0 = data;
// }

// unsigned uart0_receive(void){

//     while(!(UCSR0A & (1 << RXC0)));
//     return UDR0;
// }
// int main(){

//     uart0_init();

//     while(1){
//         uart0_transmit(uart0_receive());
//     }
// }




// //fnd Code
// // int main(){
// //     uint8_t fndnumber[] ={
// //         0x3f, 0x06, 0x5B,0x4F, 0x66, 0x6D, 0x7D, 0x27, 0x7F, 0x67
// //     };

// //     int count = 0;
// //     DDRC = 0xFF;

// //     while(1){
// //         PORTC = fndnumber[count];       //fnd 연결 포트 
// //         count = (count + 1) % 10;
// //         _delay_ms(500);

// //     }
// // }



// //DC motor (speen angle)
// // int main(){

// //     DDRB |= (1 << PB5); // PB5를 출력으로 설정
// //     TCCR1A |= (1 << COM1A1) | (1 << WGM11); // 비반전 모드, Fast PWM 모드
// //     TCCR1B |= (1 << WGM13) | (1 << WGM12) | (1 << CS11) | (1 << CS10); // Fast PWM
// //     TCCR1C = 0;

// //     //OCR1A = 499; // 10% 듀티 사이클 (ICR1의 25%)
// //     ICR1 = 4999; 
    
// //     while(1){
// //         OCR1A = 500; // 10% 듀티 사이클
// //         _delay_ms(1000); // 1초 대기
// //         OCR1A = 375;
// //         _delay_ms(1000); // 1초 대기
// //         //OCR1A = 2375;
// //          //_delay_ms(1000); // 1초 대기

// //     }
// // }




// // 8bit counter pwmmode
// // int main(){

// //     DDRB = 0xFF;            // 출력으로 설정
// //     DDRB |= (1<< PB4); // PB4를 출력으로 설정

// //     TCCR0 |= (1<< WGM00) |(1<<COM01) | (1 << WGM01) | (1 << CS02); // Fast PWM 모드, 비반전 모드, 분주비 64 

// //     //OCR0 = 64;

// //     while(1){
// //         while(TIFR & (1<< OCF0));
// //         TIFR = 0x02;       // TIFR 초기화
// //         OCR0 += 64;         // 
// //         if(OCR0 > 255){
// //             OCR0 = 0;       // OCR0이 255를 초과하면 0으로 초기화
// //         }

// //     }
// // }









// //Normal Mode
// // int main(){

// //     DDRB = 0xFF;            // 출력으로 설정
// //     PORTB = 0;              // 초기값 0으로 설정
// //     TCCR0 |= (1 << CS02) | (1 << CS00);
// //     TCNT0 = 6;
// //     while(1){
// //         while(TIFR & (1<< TOV0));
// //         PORTB = ~PORTB; // PORTD의 모든 비트를 반전
// //         TCNT0 = 6;      // TCNT0 초기화
// //         TIFR = 0x01;       // TIFR 초기화 (TOV0 플래그 클리어)


// //     }

// // }


// // CTC Mode 
// // int main(){

// //     //DDRB = 0x10;            //0b 00010000, PB4를 출력으로 설정
// //     DDRB = (1 << PB4);
// //     //TCCR0 = 0b00011100;     //0x1C
// //     TCCR0 |= (1<<COM00) | (1<<WGM01) | (1<< CS02) | (1 << CS00); //오류가 가장 작은 선언형태 (이걸 default로 쓸것)
// //     OCR0 = 249;

// //     while(1){
// //         while(TIFR & 0x02); // TCNT0 == OCF0 일때 => counter가 124가 됬을때
// //         // while(TIFR & (1 << OCF0)); // 위의 코드와 동일한 의미, OCF0는 TIFR의 1번 비트에 해당하는 매크로 상수
// //         TIFR = 0x02;       // TIFR 초기화 
// //         OCR0 = 249;         // 레지스터이기에 혹시 모르니 한번 더 써준것 

// //     }
// // }



// // #include "button.h"


// // int main(){

// //     LED_DDR = 0xFF;

// //     BUTTON btnOn;
// //     BUTTON btnOff;
// //     BUTTON btnToggle;

// //     ButtonInit(&btnOn, &BUTTON_DDR, &BUTTON_PIN, BUTTON_ON);
// //     ButtonInit(&btnOff, &BUTTON_DDR, &BUTTON_PIN, BUTTON_OFF);
// //     ButtonInit(&btnToggle, &BUTTON_DDR, &BUTTON_PIN, BUTTON_TOGGLE);

// //     while(1){
// //         if(ButtonGetState(&btnOn) == ACT_RELEASE){
// //             LED_PORT  = 0xFF; // 모든 LED 켜기
// //         }
// //         if(ButtonGetState(&btnOff) == ACT_RELEASE){
// //             LED_PORT = 0x00; // 모든 LED 끄기
// //         }
// //         if(ButtonGetState(&btnToggle) == ACT_RELEASE){
// //             LED_PORT ^= 0xFF; // 모든 LED 토글
// //         }
// //     }

// // }



// // int main(){

// //     DDRD = 0xff; 

// //     DDRG = 0;
// //     //DDRG &= ~(1<<4);  // DDRG의 4번에 연결된 포트를 입력으로 설정 (0~4번까지 다 입력으로 설정)

// //     uint8_t buttonData ;
// //     uint8_t ledData = 0x01;

  
// //     ///@brief 
// //     ///@return

// //     while(1){
// //         buttonData = PING;
        

// //         if((buttonData & (1 << 2)) == 0){
// //             ledData = (ledData >> 7) | (ledData<<1);
// //             PORTD  = ledData;
            
// //             _delay_ms(200);
// //         }
// //         if((buttonData & (1 << 3)) == 0){
// //             ledData = (ledData >> 1) | (ledData<<7);
// //             PORTD  = ledData;
// //             _delay_ms(200);

// //         }
// //         if((buttonData & (1 << 4 )) ==0 ){
// //             PORTD = 0xff;
// //             _delay_ms(200);
// //             PORTD = 0x00;
// //             ledData = 0x01;
// //         }

// //     }
// // }
    


//     // LED led;                //LED라는 구조를 가진 led변수 선언 
//     // led.port = &PORTD;
//     // led.pin = 0;
//     // for (uint8_t i = 0; i < 8; i++){
//     //     led.pin = i;
//     //     ledInit(&led);
//     // }
    
//     // while(1){
//     //     ledOn(&led);
//     //     _delay_ms(200);    
//     //     ledOff(&led);
//     //     _delay_ms(200);
//     //     led.pin++;
//     //     led.pin = (led.pin) % 8;
//     // }


// // uint8_t ledArr[] = {
// //     0x00,       // 0000 0000
// //     0x80,       // 1000 0000
// //     0xC0,       // 1100 0000
// //     0xE0,       // 1110 0000
// //     0xF0,       // 1111 0000
// //     0xF8,       // 1111 1000
// //     0xFC,       // 1111 1100
// //     0xFE,       // 1111 1110
// //     0xFF,       // 1111 1111
// //     0x7F,       // 0111 1111
// //     0x3F,       // 0011 1111
// //     0x1F,       // 0001 1111
// //     0x0F,       // 0000 1111
// //     0x07,       // 0000 0111
// //     0x03,       // 0000 0011
// //     0x01        // 0000 0001
// // };

// // int main(){
// //     DDRD = 0xFF;

// //     uint8_t arrSize = sizeof(ledArr)/sizeof (ledArr[0]);
// //     while(1){
// //         for (uint8_t i = 0; i < arrSize; i++)
// //         {
// //         PORTD = ledArr[i];
// //         _delay_ms(200);
// //         }
    
// //     }
    
// // }
// //#define LED_PORT PORTD
// //#define LED_DDR DDRD

// // void GPIO_Output(uint8_t data)
// // {
// //     LED_PORT = data;    // LED 포트에 주어진 data를 대입
// // }

// // void ledInit(){
// //     LED_DDR = 0xFF;
// // }


// // //LED shift func
// // void ledshift(uint8_t i, uint8_t *data){
// //     *data = (1 << i) | (1 << (7-i)); // 좌우 방향을 해당하는 비트설정
// // }


// // int main(){

// //     ledInit();                  //Led init
// //     uint8_t ledData = 0x01;     //Led intial data 

// //     while(1){
// //         for(int i = 0; i < 8; i++){
// //             ledshift(i, &ledData);  // Call shift func 
// //             GPIO_Output(ledData);   // Call Led output
// //             _delay_ms(1000);        // delay 1s
// //         }
// //     }

// // }
// //LED code 
// // int main(){
    
// //     DDRD |= 0b11111111; 

// //     while(1){

// //         for (uint8_t i = 0; i < 8; i++){
// //             PORTD = (0b00000001 << i);
// //             _delay_ms(200);
// //         }
// //         for (uint8_t j = 0; j < 8; j++){
// //             PORTD = (0b10000000 >> j);
// //             _delay_ms(200);
// //         }
// //     }

// // }
