
#include <stdio.h>
#include <signal.h>
#include <pigpio.h>
#include <unistd.h>  // for usleep

#define SERVO_PIN1 18  // GPIO 18
#define SERVO_PIN2 17  // GPIO 17

volatile int running = 1;

// Maps an angle to PWM for SERVO_PIN1 (min: 500, max: 2500, center: 1700)
int map_angle_to_pwm_servo1(int angle) {
    // Servo 1: -90° to +90° maps to 500 to 2500, center is 1700
    // So full range is 2000 PWM units, but offset by center.
    // However, since the center is 1700, the range is asymmetric.
    // We'll map -90 to 500, 0 to 1700, +90 to 2500

    float slope = (2500 - 500) / 180.0;  // PWM per degree
    return (int)(1500 + (angle * slope));
}

// Maps an angle to PWM for SERVO_PIN2 (min: 500, max: 2500, center: 1500)
int map_angle_to_pwm_servo2(int angle) {
    float slope = (2500 - 500) / 180.0;
    return (int)(1500 + (angle * slope));
}



// Signal handler for clean exit
void intHandler(int dummy) {
    running = 0;
}

int main() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialization failed\n");
        return 1;
    }

    signal(SIGINT, intHandler);  // Register Ctrl+C handler

    printf("Starting servo sweep using mapped angles. Press Ctrl+C to stop.\n");

    int one_val = 1666;
    int dir = 5;

    while (running) {
    // Sweep from -90° to +90°

        
                for (int angle = 2000; angle <= 2033 && running; angle++) {
            int pwm2 = map_angle_to_pwm_servo2(angle);
            gpioServo(SERVO_PIN2, angle);
            printf("Servo 2 Angle: %d°, PWM: %d\n", angle, pwm2);
            usleep(1000);
        }
        
                gpioServo(SERVO_PIN1, one_val);
        
        one_val = one_val + dir;
        
        if (one_val > 1700) {
            dir = -5;
        }
        
        if (one_val < 1666) {
            dir = 5;
        }

        // Sweep Servo 2 back down from 50° to 45°
        for (int angle = 2033; angle >= 2000 && running; angle--) {
            int pwm2 = map_angle_to_pwm_servo2(angle);
            gpioServo(SERVO_PIN2, angle);
            printf("Servo 2 Angle: %d°, PWM: %d\n", angle, pwm2);
            usleep(1000);
        }
        



    //~ // Return to 0°, 0°
    //~ int pwm1_center = map_angle_to_pwm_servo1(15);
    //~ int pwm2_center = map_angle_to_pwm_servo2(50);
    //~ gpioServo(SERVO_PIN1, pwm1_center);
    //~ gpioServo(SERVO_PIN2, pwm2_center);
    //~ usleep(200000);  // Hold at center
}

    // Stop sending pulses and cleanup
    gpioServo(SERVO_PIN1, 0);
    gpioServo(SERVO_PIN2, 1);
    gpioTerminate();

    printf("\nServo control stopped cleanly.\n");
    return 0;
}
