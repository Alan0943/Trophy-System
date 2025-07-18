// joystick libraries
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <errno.h>

// servo libraries
#include <signal.h>
#include <pigpio.h>

#define SERVO_PAN 18  // GPIO 18
#define SERVO_TILT 17  // GPIO 17

volatile int running = 1;

// Signal handler for clean exit
void intHandler (int dummy) {
    running = 0;
}

// converts joystick values to servo pmw values
int JoytoSer (int value) {
	
	int return_value = value + 1000;
	
	if (return_value >= 2000) {
		return 2000;
	}
	
	else if (return_value <= 0) {
		return 1000;
	}
	
	else {
		return return_value;
	}
	
}

int main() {
    const char *device = "/dev/input/event2"; // Change if needed
    struct libevdev *dev = NULL;
    int fd;

	// error check opening device
    fd = open(device, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Failed to open input device");
        return 1;
    }

	// error check
    if (libevdev_new_from_fd(fd, &dev) < 0) {
        fprintf(stderr, "Failed to initialize libevdev from file descriptor\n");
        return 1;
    }
 
	// read event name, should be Logitech Extreme 3D pro, bus 0x3 vendor 0x46d product 0xc215
    printf("Reading from: %s\n", libevdev_get_name(dev));
    printf("Input device ID: bus %#x vendor %#x product %#x\n",
           libevdev_get_id_bustype(dev),
           libevdev_get_id_vendor(dev),
           libevdev_get_id_product(dev));
    
    // error check gpio ports 
	if (gpioInitialise() < 0) {
		fprintf(stderr, "pigpio initialization failed\n");
		return 1;
    }

	// poll for joystick input
    struct input_event ev;
    while (1) {
		
        int rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        
        if (rc == 0) {
            if (ev.type == EV_ABS) {
				
				// pan logic
				if (ev.code == 0) {
					printf("[AXIS ] Code: %d, Value: %d\n", ev.code, ev.value);
					//printf("Value: %d\n", JoytoSer(ev.value));
					gpioServo(SERVO_PAN, JoytoSer(ev.value));
					
					}
					
				// tilt logic
				else if (ev.code == 1) {
					printf("[AXIS ] Code: %d, Value: %d\n", ev.code, ev.value);
					//printf("Value: %d\n", JoytoSer(ev.value));
					gpioServo(SERVO_TILT, JoytoSer(ev.value));
					
					}
            } 
  
        } 
        else if (rc != -EAGAIN) {
            fprintf(stderr, "Error reading event: %d\n", rc);
        }
        usleep(500); // small delay to avoid 100% CPU usage
    }


	// save memory
    libevdev_free(dev);
    close(fd);
    
    // Stop sending pulses and cleanup
    gpioServo(SERVO_PAN, 0);
    gpioServo(SERVO_TILT, 0);
    gpioTerminate();

    printf("\nServo control stopped cleanly.\n");
    return 0;
}
