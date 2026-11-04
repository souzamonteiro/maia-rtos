#include <maia/version.h>
#include <stdio.h>
int main(void) {
    if (MAIA_RTOS_VERSION_MAJOR != 0) return 1;
    puts("MaiaRTOS: host metadata contract PASS (no firmware tested)");
    return 0;
}
