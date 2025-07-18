#include <stdio.h>
#include <signal.h>
#include <pigpio.h>
#include <unistd.h>  // for usleep

#define SERVO_PIN1 18  // GPIO 18
#define SERVO_PIN2 17  // GPIO 17

volatile int running = 1;

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

	gpioServo(SERVO_PIN1, 1700);
	gpioServo(SERVO_PIN2, 2000);
	
	usleep(500000);

    printf("Starting servo sweep. Press Ctrl+C to stop.\n");

	int pan, tilt;
	int pan_min = 1700, pan_max = 1750;
	int tilt_min = 1980, tilt_max = 2000;

	int pan_step = 1;   // adjust for resolution + speed
	int tilt_step = 1; // bigger step for faster coverage

	int pan_dir = 1;

	pan = pan_min;
	tilt = tilt_min;

	while (running) {
		// Sweep PAN
		gpioServo(SERVO_PIN1, pan);

		pan += pan_step * pan_dir;

		if (pan >= pan_max) {
			pan = pan_max;
			pan_dir = -1;

			// One line done, move TILT
			tilt += tilt_step;
			if (tilt > tilt_max) tilt = tilt_min;  // loop back to top
		}
		else if (pan <= pan_min) {
			pan = pan_min;
			pan_dir = 1;

			// One line done, move TILT
			tilt += tilt_step;
			if (tilt > tilt_max) tilt = tilt_min;
		}

		gpioServo(SERVO_PIN2, tilt);

		usleep(2000); // adjust speed
	}

		// Stop sending pulses and cleanup
		gpioServo(SERVO_PIN1, 0);
		gpioServo(SERVO_PIN2, 0);
		gpioTerminate();

		printf("\nServo control stopped cleanly.\n");
		return 0;
	}
