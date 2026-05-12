#include "rgb_led.h"

void LED_Init(void) {
    LED_DDR |= (1 << PIN_RED) | (1 << PIN_GREEN) | (1 << PIN_BLUE);
    LED_AllOff();
}

void LED_AllOff(void) {
    LED_PORT &= ~((1 << PIN_RED) | (1 << PIN_GREEN) | (1 << PIN_BLUE));
}

void LED_SetStatus(SystemStatus status) {
    LED_AllOff();
    switch (status) {
        case STATUS_GOOD:
            LED_PORT |= (1 << PIN_GREEN);
            break;
        case STATUS_CHECK:
            LED_PORT |= (1 << PIN_RED) | (1 << PIN_GREEN); // 노란색
            break;
        case STATUS_WARNING:
            LED_PORT |= (1 << PIN_RED);
            break;
    }
}