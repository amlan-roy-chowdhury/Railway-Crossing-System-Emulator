#ifndef RAILWAY_SYSTEM_H
#define RAILWAY_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/utsname.h>

// Constants
#define PATH_SIZE 256
#define SERVO_PWM_PERIOD "/sys/class/pwm/pwm-%d:0/period"
#define SERVO_PWM_DUTY_CYCLE "/sys/class/pwm/pwm-%d:0/duty_cycle"
#define SERVO_PWM_ENABLE "/sys/class/pwm/pwm-%d:0/enable"
#define PWM_PERIOD "20000000"
#define SERVO_UP_POSITION "1000000"
#define SERVO_DOWN_POSITION "2000000"
#define LED_BLINK_INTERVAL_MS 500
#define COLLISION_BLINK_INTERVAL_MS 200
#define GATE_DELAY_MS 1000
#define LED_STOP_DELAY_MS 1000

// System State Enum
typedef enum {
    IDLE,
    TRAIN_APPROACHING_EAST,
    TRAIN_AT_CROSSING_EAST,
    TRAIN_LEAVING_EAST,
    TRAIN_APPROACHING_WEST,
    TRAIN_AT_CROSSING_WEST,
    TRAIN_LEAVING_WEST,
    COLLISION_RISK
} SystemState;

// GPIO Configuration Structure
typedef struct {
    int b1;
    int b2;
    int b3;
    int b4;
    int led_1;
    int led_2;
    int buzzer;
    int pwm_num;
} GPIO_Config;

// Global Variables
extern volatile SystemState current_state;
extern volatile bool system_running;
extern GPIO_Config gpio_config;
extern pthread_mutex_t state_mutex;
extern bool led_active;
extern bool gate_down;

// Setup Function Prototypes
void export_gpio(int gpio_pin);
void unexport_gpio(int gpio_pin);
void set_gpio_direction(int gpio_pin, const char *direction);
void set_gpio_value(int gpio_pin, int value);
int read_gpio_value(int gpio_pin);
int set_pwm(const char* format, int pwm_num, const char* value);
int initialize_servo(void);
void cleanup_servo(void);
void initialize_system(void);
void cleanup_system(void);
void get_gpio_configuration(void);
void print_system_info(void);

// Thread Function Prototypes
void* button_thread_function(void* arg);
void* led_thread_function(void* arg);
void* servo_thread_function(void* arg);
void* buzzer_thread_function(void* arg);

#endif