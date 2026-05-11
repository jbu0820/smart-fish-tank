#include <avr/io.h>
#include <util/delay.h>

#define LED_DDR         DDRD
#define LED_PORT        PORTD
#define BUTTON_DDR      DDRG
#define BUTTON_PIN      PING

#define BUTTON_ON       2
#define BUTTON_OFF      3
#define BUTTON_TOGGLE   4

enum {PUSHED,   RELEASED};
enum {NO_ACT,   ACT_PUSH,   ACT_RELEASE}; // 기본적으로 0번

typedef struct
{
    volatile uint8_t *ddr; // register 값을 직접 읽어오는 용도로 사용하기 위해 volatile 로 선언 
    volatile uint8_t *pin;
    uint8_t          btnPin;
    uint8_t          prevState;
} BUTTON;

void ButtonInit(BUTTON *button, volatile uint8_t *ddr, volatile uint8_t *pin, uint8_t pinNum);
uint8_t ButtonGetState(BUTTON *button);

