/*
	UART communication on Raspberry Pi using C (WiringPi Library)
	http://www.electronicwings.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wiringPi.h>
#include <wiringSerial.h>

int serial_port;

void handle_sigint(int sig) {
    printf("\nCaught Ctrl+C, closing serial port and exiting.\n");
    serialClose(serial_port);
    exit(0);
}

void read_tfluna_data(int serial_port, int *data) {
    for (int i = 0; i < 9; i++) {
        data[i] = serialGetchar(serial_port);
    }
}

int main() {
    if ((serial_port = serialOpen("/dev/serial0", 115200)) < 0) {
        fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
        return 1;
    }

    if (wiringPiSetup() == -1) {
        fprintf(stdout, "Unable to start wiringPi: %s\n", strerror(errno));
        return 1;
    }

    signal(SIGINT, handle_sigint);  // Catch Ctrl+C

    while (1) {
        if (serialDataAvail(serial_port)) {
            int distance, strength;
            float temp;
            int data[9];

            read_tfluna_data(serial_port, data);

            distance = data[2] + data[3]*256;
            strength = data[4] + data[5]*256;
            temp = data[6] + data[7]*256;
            temp = (temp / 8.0f) - 256.0f;

            printf("Distance: %d cm\n", distance);
            printf("Signal Strength: %d\n", strength);
            printf("Temperature: %.2f Celsius\n", temp);
            fflush(stdout);
        }

        delay(10); // 10ms delay
    }


}


