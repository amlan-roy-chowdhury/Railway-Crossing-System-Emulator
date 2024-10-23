#include "railway_system.h"

void* button_thread_function(void* arg) {
    (void)arg;
    
    bool b1_active = false;
    bool b2_active = false;
    bool b3_active = false;
    bool b4_active = false;
    bool in_collision = false;
    char input;
    
    enum { NONE, EAST_TO_WEST, WEST_TO_EAST } direction = NONE;
    
    printf("Starting button monitor...\n");
    printf("Ready for train detection\n");
    printf("(Press 'q' and Enter to reset collision state)\n");
    printf("GPIO Configuration:\n");
    printf("B1: GPIO%d\n", gpio_config.b1);
    printf("B2: GPIO%d\n", gpio_config.b2);
    printf("B3: GPIO%d\n", gpio_config.b3);
    printf("B4: GPIO%d\n", gpio_config.b4);

    // Set stdin to non-blocking
    int flags = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, flags | O_NONBLOCK);
    
    while (system_running) {
        int b1 = read_gpio_value(gpio_config.b1);
        int b2 = read_gpio_value(gpio_config.b2);
        int b3 = read_gpio_value(gpio_config.b3);
        int b4 = read_gpio_value(gpio_config.b4);
        
        if (read(STDIN_FILENO, &input, 1) > 0) {
            if (input == 'q' && in_collision) {
                printf("\nResetting collision state...\n");
                in_collision = false;
                current_state = IDLE;
                led_active = false;
                gate_down = false;
                direction = NONE;
                b1_active = b2_active = b3_active = b4_active = false;
                printf("System reset - Ready for new train\n");
                continue;
            }
        }

        pthread_mutex_lock(&state_mutex);
        
        if (!in_collision) {
            // East to West sequence (66->67->68->69)
            if (direction == NONE && !b1_active && b1 == 0) {
                printf("\nB1 pressed - Starting East sequence\n");
                direction = EAST_TO_WEST;
                b1_active = true;
            }
            else if (direction == EAST_TO_WEST && b1_active && !b2_active && b2 == 0) {
                printf("B2 pressed - Activating signals\n");
                led_active = true;
                gate_down = true;
                b2_active = true;
                current_state = TRAIN_AT_CROSSING_EAST;
            }
            else if (direction == EAST_TO_WEST && b2_active && !b3_active && b3 == 0) {
                printf("B3 pressed - Train crossing\n");
                b3_active = true;
                current_state = TRAIN_LEAVING_EAST;
            }
            else if (direction == EAST_TO_WEST && b3_active && !b4_active && b4 == 0) {
                printf("B4 pressed - Train clear\n");
                b4_active = true;
                led_active = false;
                usleep(LED_STOP_DELAY_MS * 1000);
                gate_down = false;
                current_state = IDLE;
                direction = NONE;
                b1_active = b2_active = b3_active = b4_active = false;
            }
            
            // West to East sequence (69->68->67->66)
            if (direction == NONE && !b4_active && b4 == 0) {
                printf("\nB4 pressed - Starting West sequence\n");
                direction = WEST_TO_EAST;
                b4_active = true;
            }
            else if (direction == WEST_TO_EAST && b4_active && !b3_active && b3 == 0) {
                printf("B3 pressed - Activating signals\n");
                led_active = true;
                gate_down = true;
                b3_active = true;
                current_state = TRAIN_AT_CROSSING_WEST;
            }
            else if (direction == WEST_TO_EAST && b3_active && !b2_active && b2 == 0) {
                printf("B2 pressed - Train crossing\n");
                b2_active = true;
                current_state = TRAIN_LEAVING_WEST;
            }
            else if (direction == WEST_TO_EAST && b2_active && !b1_active && b1 == 0) {
                printf("B1 pressed - Train clear\n");
                b1_active = true;
                led_active = false;
                usleep(LED_STOP_DELAY_MS * 1000);
                gate_down = false;
                current_state = IDLE;
                direction = NONE;
                b1_active = b2_active = b3_active = b4_active = false;
            }

            // Collision Detection
            if (!in_collision &&
                ((direction == EAST_TO_WEST && b2_active && !b3_active && b4 == 0) ||
                 (direction == WEST_TO_EAST && b3_active && !b2_active && b1 == 0))) {
                printf("\n*** COLLISION RISK DETECTED! ***\n");
                printf("Train approaching from opposite direction!\n");
                printf("Press 'q' and Enter to reset system\n");
                current_state = COLLISION_RISK;
                led_active = true;
                gate_down = true;
                in_collision = true;
            }
        }
        
        pthread_mutex_unlock(&state_mutex);
        usleep(50000);  // 50ms debounce delay
    }
    
    return NULL;
}

void* led_thread_function(void* arg) {
    (void)arg;
    bool led_state = false;
    
    while (system_running) {
        pthread_mutex_lock(&state_mutex);
        SystemState current = current_state;
        bool is_active = led_active;
        pthread_mutex_unlock(&state_mutex);
        
        if (current == COLLISION_RISK) {
            // Rapid simultaneous blinking for collision risk
            set_gpio_value(gpio_config.led_1, 1);
            set_gpio_value(gpio_config.led_2, 1);
            usleep(COLLISION_BLINK_INTERVAL_MS * 1000);
            set_gpio_value(gpio_config.led_1, 0);
            set_gpio_value(gpio_config.led_2, 0);
            usleep(COLLISION_BLINK_INTERVAL_MS * 1000);
        } else if (is_active) {
            // Alternating pattern for normal operation
            set_gpio_value(gpio_config.led_1, led_state ? 1 : 0);
            set_gpio_value(gpio_config.led_2, led_state ? 0 : 1);
            usleep(LED_BLINK_INTERVAL_MS * 1000);
            led_state = !led_state;
        } else {
            // LEDs off when inactive
            set_gpio_value(gpio_config.led_1, 0);
            set_gpio_value(gpio_config.led_2, 0);
            usleep(LED_BLINK_INTERVAL_MS * 1000);
        }
    }
    return NULL;
}

void* servo_thread_function(void* arg) {
    (void)arg;
    bool previous_gate_state = false;
    
    while (system_running) {
        pthread_mutex_lock(&state_mutex);
        bool current_gate_state = gate_down;
        pthread_mutex_unlock(&state_mutex);
        
        if (current_gate_state != previous_gate_state) {
            if (current_gate_state) {
                printf("Moving gate to DOWN position\n");
                set_pwm(SERVO_PWM_DUTY_CYCLE, gpio_config.pwm_num, SERVO_DOWN_POSITION);
            } else {
                printf("Moving gate to UP position\n");
                set_pwm(SERVO_PWM_DUTY_CYCLE, gpio_config.pwm_num, SERVO_UP_POSITION);
            }
            previous_gate_state = current_gate_state;
            usleep(500000);  // Wait for servo movement
        }
        
        usleep(100000);  // Check gate state every 100ms
    }
    return NULL;
}

void* buzzer_thread_function(void* arg) {
    (void)arg;
    
    while (system_running) {
        pthread_mutex_lock(&state_mutex);
        SystemState current = current_state;
        pthread_mutex_unlock(&state_mutex);
        
        if (current == COLLISION_RISK) {
            set_gpio_value(gpio_config.buzzer, 1);
            usleep(100000);  // Buzzer on for 100ms
            set_gpio_value(gpio_config.buzzer, 0);
            usleep(100000);  // Buzzer off for 100ms
        } else {
            set_gpio_value(gpio_config.buzzer, 0);
            usleep(100000);
        }
    }
    return NULL;
}