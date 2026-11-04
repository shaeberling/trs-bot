
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#define SOLENOID_PIN 15

int main(void) {
    stdio_init_all();
    cyw43_arch_init();               // the W's LED is on the Wi-Fi chip
    gpio_init(SOLENOID_PIN);
    gpio_set_dir(SOLENOID_PIN, GPIO_OUT);
    gpio_put(SOLENOID_PIN, 0);

    sleep_ms(2000);                  // time to open the serial port
    for (int i = 0; i < 10; i++) {
        printf("press %d\n", i);
        gpio_put(SOLENOID_PIN, 1);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(40);
        gpio_put(SOLENOID_PIN, 0);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(500);
    }
    printf("done\n");
    while (true) {
        sleep_ms(1000);
    }
}
