#include "railway_system.h"

// Global variables
volatile SystemState current_state = IDLE;
volatile bool system_running = true;
GPIO_Config gpio_config;
pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;
bool led_active = false;
bool gate_down = false;

// GPIO Functions
void export_gpio(int gpio_pin) {
    char buffer[PATH_SIZE];
    FILE *file;

    snprintf(buffer, sizeof(buffer), "/sys/class/gpio/gpio%d", gpio_pin);
    if (access(buffer, F_OK) != 0) {
        file = fopen("/sys/class/gpio/export", "w");
        if (file == NULL) {
            printf("Error exporting GPIO %d: %s\n", gpio_pin, strerror(errno));
            return;
        }
        fprintf(file, "%d", gpio_pin);
        fclose(file);
        usleep(100000);
    }
}

void unexport_gpio(int gpio_pin) {
    FILE *file = fopen("/sys/class/gpio/unexport", "w");
    if (file == NULL) {
        printf("Error unexporting GPIO %d: %s\n", gpio_pin, strerror(errno));
        return;
    }
    fprintf(file, "%d", gpio_pin);
    fclose(file);
}

void set_gpio_direction(int gpio_pin, const char *direction) {
    char path[PATH_SIZE];
    FILE *file;

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio_pin);
    file = fopen(path, "w");
    if (file == NULL) {
        printf("Error setting GPIO %d direction: %s\n", gpio_pin, strerror(errno));
        return;
    }
    fprintf(file, "%s", direction);
    fclose(file);
}

void set_gpio_value(int gpio_pin, int value) {
    char path[PATH_SIZE];
    FILE *file;

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio_pin);
    file = fopen(path, "w");
    if (file == NULL) {
        printf("Error setting GPIO %d value: %s\n", gpio_pin, strerror(errno));
        return;
    }
    fprintf(file, "%d", value);
    fclose(file);
}

int read_gpio_value(int gpio_pin) {
    char path[PATH_SIZE];
    FILE *file;
    char value_str[3];

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio_pin);
    file = fopen(path, "r");
    if (file == NULL) {
        printf("Error reading GPIO %d value\n", gpio_pin);
        return -1;
    }
    if (fgets(value_str, sizeof(value_str), file) == NULL) {
        printf("Error reading GPIO %d value\n", gpio_pin);
        fclose(file);
        return -1;
    }
    fclose(file);
    return atoi(value_str);
}

// Servo Functions
int set_pwm(const char* format, int pwm_num, const char* value) {
    char path[PATH_SIZE];
    snprintf(path, sizeof(path), format, pwm_num);
    
    FILE* fp = fopen(path, "w");
    if (fp == NULL) {
        printf("Error opening PWM file %s\n", path);
        return -1;
    }
    fprintf(fp, "%s", value);
    fclose(fp);
    return 0;
}

int initialize_servo(void) {
    if (set_pwm(SERVO_PWM_PERIOD, gpio_config.pwm_num, PWM_PERIOD) < 0 ||
        set_pwm(SERVO_PWM_DUTY_CYCLE, gpio_config.pwm_num, SERVO_UP_POSITION) < 0 ||
        set_pwm(SERVO_PWM_ENABLE, gpio_config.pwm_num, "1") < 0) {
        return -1;
    }
    return 0;
}

void cleanup_servo(void) {
    set_pwm(SERVO_PWM_DUTY_CYCLE, gpio_config.pwm_num, SERVO_UP_POSITION);
    usleep(500000);
    set_pwm(SERVO_PWM_ENABLE, gpio_config.pwm_num, "0");
}

// System Functions
void initialize_system(void) {
    printf("Initializing GPIO pins...\n");
    export_gpio(gpio_config.b1);
    export_gpio(gpio_config.b2);
    export_gpio(gpio_config.b3);
    export_gpio(gpio_config.b4);
    export_gpio(gpio_config.led_1);
    export_gpio(gpio_config.led_2);
    export_gpio(gpio_config.buzzer);
    
    usleep(500000);

    set_gpio_direction(gpio_config.b1, "in");
    set_gpio_direction(gpio_config.b2, "in");
    set_gpio_direction(gpio_config.b3, "in");
    set_gpio_direction(gpio_config.b4, "in");
    set_gpio_direction(gpio_config.led_1, "out");
    set_gpio_direction(gpio_config.led_2, "out");
    set_gpio_direction(gpio_config.buzzer, "out");
    
    set_gpio_value(gpio_config.led_1, 0);
    set_gpio_value(gpio_config.led_2, 0);
    set_gpio_value(gpio_config.buzzer, 0);
    
    if (initialize_servo() < 0) {
        printf("Warning: Failed to initialize servo\n");
    }
}

void cleanup_system(void) {
    set_gpio_value(gpio_config.led_1, 0);
    set_gpio_value(gpio_config.led_2, 0);
    set_gpio_value(gpio_config.buzzer, 0);
    cleanup_servo();
    
    unexport_gpio(gpio_config.b1);
    unexport_gpio(gpio_config.b2);
    unexport_gpio(gpio_config.b3);
    unexport_gpio(gpio_config.b4);
    unexport_gpio(gpio_config.led_1);
    unexport_gpio(gpio_config.led_2);
    unexport_gpio(gpio_config.buzzer);
}

void get_gpio_configuration(void) {
    printf("Enter GPIO pin numbers:\n");
    printf("Button B1 (East outer sensor): ");
    scanf("%d", &gpio_config.b1);
    printf("Button B2 (East inner sensor): ");
    scanf("%d", &gpio_config.b2);
    printf("Button B3 (West inner sensor): ");
    scanf("%d", &gpio_config.b3);
    printf("Button B4 (West outer sensor): ");
    scanf("%d", &gpio_config.b4);
    printf("LED 1: ");
    scanf("%d", &gpio_config.led_1);
    printf("LED 2: ");
    scanf("%d", &gpio_config.led_2);
    printf("Buzzer: ");
    scanf("%d", &gpio_config.buzzer);
    printf("\nEnter PWM number for servo (e.g., 4 for pwm-4:0): ");
    scanf("%d", &gpio_config.pwm_num);
}

void print_system_info(void) {
    struct utsname system_info;
    if (uname(&system_info) == 0) {
        printf("System Name: %s\n", system_info.sysname);
        printf("Node Name: %s\n", system_info.nodename);
        printf("Release: %s\n", system_info.release);
        printf("Version: %s\n", system_info.version);
        printf("Machine: %s\n", system_info.machine);
    }
}