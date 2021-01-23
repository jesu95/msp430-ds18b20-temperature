#include "ds18b20.h"

void main(void) {
    volatile float temp = 0;
    //volatile uint64_t rom;

    WDT_A_hold(WDT_A_BASE);
    ds18b20_init_port();
    Init_LCD();
    set_clock();
    PMM_unlockLPM5();

    while(1){
        temp = ds18b20_get_temp();
        show_temp(temp);
    }
}
