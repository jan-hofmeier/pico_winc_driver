#include "pico/multicore.h"
#include "driver/winc_driver_app.h"
#include "pico_winc_simulator/winc_simulator_app.h"

void core1_entry() {
    winc_simulator_app_main();
}

int main() {
    multicore_launch_core1(core1_entry);
    winc_driver_app_main();
    return 0;
}
