#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pigpio.h>
#include <pthread.h>

#include <wiringPi.h>
#include <wiringSerial.h>
#include <errno.h>


#define SERVO_PAN 18
#define SERVO_TILT 17

int target_distance = 140;


volatile int running = 1;

volatile int curr_pan;
volatile int curr_tilt;
volatile int global_pan = 1666;
volatile int global_tilt = 2000;
volatile int width = 33;
int dir = 3;

pthread_mutex_t pos_mutex = PTHREAD_MUTEX_INITIALIZER;


int serial_port;  // ✅ GLOBAL so the handler can see it

// Comparison function for qsort
int cmpfunc(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}


void handle_sigint(int sig) {
    running = 0;

}


void* servo2_loop(void* arg) {
    curr_pan = global_pan;
    
    while (running) {


        // tilt up    
        for (int angle = global_tilt; angle <= global_tilt + width; angle++) {
            curr_tilt = angle;
            gpioServo(SERVO_TILT, angle);
            
            pthread_mutex_lock(&pos_mutex);
            curr_tilt = angle;
            pthread_mutex_unlock(&pos_mutex);

            
            usleep(1000);

            }
        
            
        // pan
        gpioServo(SERVO_PAN, curr_pan);
            
        curr_pan = curr_pan + dir;
        
        pthread_mutex_lock(&pos_mutex);
        curr_pan = curr_pan + dir;
        pthread_mutex_unlock(&pos_mutex);
        
        
        if (curr_pan >= global_pan + width) {
            dir = -3;
        }
        
        if (curr_pan <= global_pan) {
            dir = 3;
        
            }

        // tilt down
        for (int angle = global_tilt+width; angle >= global_tilt; angle--) {
                curr_tilt = angle;
                gpioServo(SERVO_TILT, angle);
                usleep(1000);
                
                pthread_mutex_lock(&pos_mutex);
                curr_tilt = angle;
                pthread_mutex_unlock(&pos_mutex);

                
                
            }
		
	}
    
    return NULL;
    
}
    
    
void* tf_luna_read(void* arg) {
    int serial_port = *(int*)arg;

    int data[9];

    while (running) {
        if (serialDataAvail(serial_port) >= 9) {
            for (int i = 0; i < 9; i++) {
                data[i] = serialGetchar(serial_port);
            }

            int distance = data[2] + data[3] * 256;
            int strength = data[4] + data[5] * 256;

            if (strength > 1000 && abs(distance - target_distance) < 10) {
                
                
                int pan_snapshot, tilt_snapshot;

                pthread_mutex_lock(&pos_mutex);
                pan_snapshot = curr_pan;
                tilt_snapshot = curr_tilt;
                pthread_mutex_unlock(&pos_mutex);
                                
                
                
                printf("pan=%d tilt=%d\n", pan_snapshot, tilt_snapshot);

                FILE *fp = fopen("hits.txt", "a");
                if (fp != NULL) {
                    fprintf(fp, "%d,%d\n", pan_snapshot, tilt_snapshot);
                    fclose(fp);
                } else {
                    perror("Failed to open file");
                }

        }
        
            else {

            //~ printf("pan=%d tilt=%d dist=%d strength=%d\n", curr_pan, curr_tilt, distance, strength);
        }
    }
}

    return NULL;
}



int main() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialization failed\n");
        return 1;
    }

    if ((serial_port = serialOpen("/dev/serial0", 115200)) < 0) {
        fprintf(stderr, "Unable to open serial device");
        return 1;
    }

    signal(SIGINT, handle_sigint);

    pthread_t t2, t3;
    pthread_create(&t2, NULL, servo2_loop, NULL);
    pthread_create(&t3, NULL, tf_luna_read, &serial_port);

    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    // Clean shutdown AFTER threads are done
    gpioServo(SERVO_PAN, 0);
    gpioServo(SERVO_TILT, 0);
    gpioTerminate();
    printf("Stopped cleanly.\n");

    return 0;
}

