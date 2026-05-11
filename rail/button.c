#include "button.h"


void ButtonInit(BUTTON *button, volatile uint8_t *ddr, volatile uint8_t *pin, uint8_t pinNum){

    button->ddr = ddr;
    button->pin = pin;
    button->btnPin = pinNum;
    button->prevState = RELEASED;           // 초기상태로 아무것도 안누른상태
    *button->ddr &= (1 << button->btnPin);  // 버튼에 대한 핀을 입력 상태로 설정 
}

uint8_t ButtonGetState(BUTTON *button){

    uint8_t curState =  *button->pin & (1 << button->btnPin); // 현재 버튼의 상태를 읽어옴, 해당 핀의 비트값이 1이면 눌리지 않은 상태, 0이면 눌린 상태
    

    if((curState == PUSHED) && (button->prevState == RELEASED)){

        _delay_ms(50);
        button->prevState = PUSHED;         // debouncing 처리 후 버튼의 상태를 PUSHED로 업데이트
        return ACT_PUSH;                    // 버튼을 누른 상태로 변환

    }
    else if ((curState != PUSHED) && (button->prevState == PUSHED)){ // 버튼이 눌린 상태에서 뗀 상태로 변화한 경우
        _delay_ms(50);
        button->prevState = RELEASED;       //버튼을 뗀 상태로 변환
        return ACT_RELEASE;                 // 버튼을 뗀 상태로 변환
    }
    return NO_ACT;                          // 버튼의 상태에 변화가 없는 경우 NO_ACT 반환
}