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

       
        _delay_ms(500);
    }
}
