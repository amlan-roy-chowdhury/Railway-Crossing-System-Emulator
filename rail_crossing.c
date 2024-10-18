#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <sys/utsname.h>

// Function to set the GPIO pin as an output
void set_gpio_direction(int gpio_pin) {
    char gpio_direction_path[100];
    sprintf(gpio_direction_path, "/sys/class/gpio/gpio%d/direction", gpio_pin);
    FILE *file = fopen(gpio_direction_path, "w");
    if (file == NULL) {
        perror("Error setting GPIO direction to out");
        exit(1);
    }
    fprintf(file, "out");
    fclose(file);
}

// Function to set the GPIO pin value (on or off)
void set_gpio_value(int gpio_pin, int value) {
    char gpio_value_path[100];
    sprintf(gpio_value_path, "/sys/class/gpio/gpio%d/value", gpio_pin);
    FILE *file = fopen(gpio_value_path, "w");
    if (file == NULL) {
        perror("Error setting GPIO pin value");
        exit(1);
    }
    fprintf(file, "%d", value);
    fclose(file);
}