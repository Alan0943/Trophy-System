// input libraries
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ncurses.h>

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

//~ int pan = 1620; // global pan pwm variable
//~ int tilt = 1970; // global tilt pwm variable

// used for servo scanning
int pan = 1666;
int working_pan;


int tilt = 2000;

int dir = 11;

int current_tracking_distance = 100;


// tf luna read function
void read_tfluna_data(int serial_port, int *data) {
    for (int i = 0; i < 9; i++) {
        data[i] = serialGetchar(serial_port);
    }
}

// Signal handler for clean exit
void handle_sigint(int sig) {
    //printf("\nCaught Ctrl+C, closing serial port and exiting.\n");

    // Stop servos
    gpioServo(SERVO_PAN, 0);
    gpioServo(SERVO_TILT, 0);
    gpioTerminate();

    // Close TF Luna serial
    serialClose(serial_port);
    
    // End curses mode
    endwin();

    exit(0);
}


//~ void servo_coordinates() {
    

    //~ int curr_count = frames[frame_count].count;
    
    //~ int high_bar = current_tracking_distance + 10;
    //~ int low_bar = current_tracking_distance - 10;

    //~ int pan_sum = 0, pan_count = 0;
    //~ int sum_distance = 0;
    
    //~ for (int i = 0; i < frames[frame_count].count; i++) {
	//~ int pan = frames[frame_count].points[i].pan;
	//~ int tilt = frames[frame_count].points[i].tilt;
	//~ int distance = frames[frame_count].points[i].distance;
	
	
	//~ if (distance > low_bar && distance < high_bar) {
	    //~ pan_sum += pan;
	    //~ pan_count++;
	    //~ sum_distance += distance;
	//~ }
	
	
    //~ }
    
    //~ if (frame_count > 5) {
	//~ }
    
    //~ if (pan_count > 0) {
	//~ current_tracking_distance = sum_distance / pan_count;
	//~ int pan_avg = pan_sum / pan_count;

	//~ frames[frame_count].pan_avg = pan_avg;

	//~ clear();
	//~ mvprintw(1, 0, "Yes Data:");
	//~ mvprintw(2, 0, "Current Pan: %d cm", pan);
	//~ mvprintw(3, 0, "Average Pan: %d cm", pan_avg);
	//~ mvprintw(3, 0, "Average Distance: %d cm", current_tracking_distance);


	//~ refresh();
	
	
	//~ if (pan_avg > pan) {
	    //~ pan = pan + 33;}
	//~ }
	//~ else {
	    //~ pan = pan - 33;}
    

    //~ frame_count++;
    
//~ }

void scanning_mode(int p, int t, int width) {
    
    while (1) {
    gpioServo(SERVO_TILT, t);
    usleep(100000);
    printf("hi");
    gpioServo(SERVO_TILT, 2500);
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
    


		
	
	//~ clear();
	//~ mvprintw(0,0,"Press 1 to Enter Manual Mode");
	//~ mvprintw(1,0,"Press 2 to Enter Scanning Mode");
	//~ mvprintw(2,0,"Press 3 to View Results");
	//~ mvprintw(3,0, "Press 4 to View Plot");
	//~ mvprintw(4,0,"Presss q to Enter Main Menu");
	//~ mvprintw(5,0,"Press Ctrl C to quit.\n");
	//~ refresh();
	
	//~ ch = getch();
	
	int move_pan, move_tilt;

		    
    scanning_mode(pan, 2000, 33);

		    //~ servo_coordinates();

		    
		    //~ plot_mode(pipeForGNUPlot);
		    //~ delay(1000);
		    
		    //pclose(pipeForGNUPlot);



				
		
	


    
    
    return 0;
}
