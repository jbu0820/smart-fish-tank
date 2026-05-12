#define F_CPU 16000000UL

#include <util/delay.h>

#include "system.h"
#include "feeder.h"

int main(void)
{
    system_init();

    while (1)
    {
        feeder_update();

        // 다른 팀원 기능 추가 위치
        // water_sensor_update();
        // lcd_update();
        // cleaning_rail_update();

        _delay_ms(500);
    }
}
