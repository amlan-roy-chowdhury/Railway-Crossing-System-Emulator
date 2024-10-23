#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <sys/utsname.h>

// Function prototypes
void *train_sensor(void *arg);
void *led_control(void *arg);
void *crossing_guard_control(void *arg);
void collision_detection();
void set_gpio_direction(int gpio_pin);
void set_gpio_value(int gpio_pin, int value);

// GPIO pins - prompted by user
int sensor1_gpio, sensor2_gpio, led1_gpio, led2_gpio, servo_gpio, buzzer_gpio;

// Shared status variables
bool train_approaching = false;
bool collision_detected = false;
pthread_mutex_t lock;

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
//Add POSIX threads for different processes

void *train_sensor(void *arg) {
    int sensor_gpio = *(int *)arg;
    while (1) {
        // Simulate sensor detection (replace with actual sensor GPIO reading)
        int train_detected = rand() % 2;  // Randomly simulates train detection
        if (train_detected) {
            pthread_mutex_lock(&lock);
            train_approaching = true;
            printf("Train detected on sensor %d\n", sensor_gpio);
            pthread_mutex_unlock(&lock);
            sleep(1); // Debouncing time
        }
        usleep(500000); // Check every half second
    }
}

// Thread to control LEDs
void *led_control(void *arg) {
    while (1) {
        if (collision_detected) {
            // Both LEDs blink rapidly in case of a collision
            set_gpio_value(led1_gpio, 1);
            set_gpio_value(led2_gpio, 1);
            usleep(100000);  // Rapid blinking
            set_gpio_value(led1_gpio, 0);
            set_gpio_value(led2_gpio, 0);
            usleep(100000);
        } else if (train_approaching) {
            // Alternate LED blinking
            set_gpio_value(led1_gpio, 1);
            set_gpio_value(led2_gpio, 0);
            sleep(1);
            set_gpio_value(led1_gpio, 0);
            set_gpio_value(led2_gpio, 1);
            sleep(1);
        } else {
            set_gpio_value(led1_gpio, 0);
            set_gpio_value(led2_gpio, 0);
        }
        usleep(500000); // Adjust as needed
    }