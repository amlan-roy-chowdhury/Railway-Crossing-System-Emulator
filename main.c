#include "railway_system.h"

static void signal_handler(int signo) {
    if (signo == SIGINT) {
        printf("\nReceived interrupt signal. Cleaning up...\n");
        system_running = false;
    }
}

int main(void) {
    pthread_t button_thread, led_thread, servo_thread, buzzer_thread;
    
    // Print system info and team details
    print_system_info();
    printf("\nTeam Members:\n");
    printf("Nanda Kishore Nallagopu G01447294\n");
    printf("Amlan Choudhary G01465085\n\n");
    
    // Get GPIO configuration
    get_gpio_configuration();
    
    // Initialize system
    initialize_system();
    
    // Set up signal handler
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        printf("Error setting up signal handler\n");
        cleanup_system();
        return 1;
    }
    
    // Create threads
    if (pthread_create(&button_thread, NULL, button_thread_function, NULL) != 0 ||
        pthread_create(&led_thread, NULL, led_thread_function, NULL) != 0 ||
        pthread_create(&servo_thread, NULL, servo_thread_function, NULL) != 0 ||
        pthread_create(&buzzer_thread, NULL, buzzer_thread_function, NULL) != 0) {
        
        printf("Error creating threads\n");
        cleanup_system();
        return 1;
    }
    
    // Wait for threads to complete
    pthread_join(button_thread, NULL);
    pthread_join(led_thread, NULL);
    pthread_join(servo_thread, NULL);
    pthread_join(buzzer_thread, NULL);
    
    // Cleanup
    cleanup_system();
    
    return 0;
}