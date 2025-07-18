// input libraries
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

// servo libraries
#include <signal.h>
#include <pigpio.h>

// luna libraries
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringSerial.h>

#define SERVO_PAN 18  // GPIO 18
#define SERVO_TILT 17  // GPIO 17

int serial_port;  // ✅ GLOBAL so the handler can see it

// Signal handler for clean exit
void handle_sigint(int sig) {
    printf("\nCaught Ctrl+C, closing serial port and exiting.\n");

    // Stop servos
    gpioServo(SERVO_PAN, 0);
    gpioServo(SERVO_TILT, 0);
    gpioTerminate();

    // Close TF Luna serial
    serialClose(serial_port);

    exit(0);
}

// tf luna read function
void read_tfluna_data(int serial_port, int *data) {
    for (int i = 0; i < 9; i++) {
        data[i] = serialGetchar(serial_port);
    }
}

int main() {

    // error check luna serial open
    if ((serial_port = serialOpen("/dev/serial0", 115200)) < 0) {
        fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
        return 1;
    }

    // error check wiringPi setup
    if (wiringPiSetup() == -1) {
        fprintf(stdout, "Unable to start wiringPi: %s\n", strerror(errno));
        return 1;
    }

    // error check gpio ports
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialization failed\n");
        return 1;
    }
    
     // Register SIGINT handler
    signal(SIGINT, handle_sigint);

    while (1) {


        // tf luna read
        if (serialDataAvail(serial_port)) {
            int distance, strength;
            float temp;
            int data[9];

            read_tfluna_data(serial_port, data);

            distance = data[2] + data[3] * 256;
            strength = data[4] + data[5] * 256;
            temp = data[6] + data[7] * 256;
            temp = (temp / 8.0f) - 256.0f;

            printf("Distance: %d cm\n", distance);
            printf("Signal Strength: %d\n", strength);
            printf("Temperature: %.2f Celsius\n", temp);
            fflush(stdout);
        }

        usleep(500); // small delay to avoid 100% CPU usage
    }

    return 0;
}
