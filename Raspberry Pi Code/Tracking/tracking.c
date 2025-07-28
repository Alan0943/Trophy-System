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

typedef struct {
  int pan;
  int tilt;
  int distance;
} ScanPoint;

typedef struct {
  ScanPoint points[1000];
  int count;
  
  int pan_avg,tilt_avg;
} ScanFrame;

ScanFrame frames[1000];
int frame_count = 0;

void plot_mode(FILE* pipeForGNUPlot) {
	
	
    //~ FILE *pipeForGNUPlot = NULL;

    //~ pipeForGNUPlot = popen("gnuplot -persistent", "w");

    if (pipeForGNUPlot != NULL) {

	fprintf(pipeForGNUPlot, "splot \"scan_results.txt\" using 1:2:3 with pm3d notitle\n");
	
	fflush(pipeForGNUPlot);

        //~ pclose(pipeForGNUPlot);
        
    } else {
        printf("Could not open pipe to Gnuplot.\n");
    }
	
		
}


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

// manually control servos
void manual_mode() {
    int ch;
    int speed = 11;

    // Make getch non-blocking
    nodelay(stdscr, TRUE);

    while (1) {
	ch = getch();

        if (ch != ERR) {
            if (ch == 'q') {
                break;  // exit manual mode
            }
            switch (ch) {
                case KEY_UP:    tilt -= speed; break;
                case KEY_DOWN:  tilt += speed; break;
                case KEY_LEFT:  pan += speed;  break;
                case KEY_RIGHT: pan -= speed;  break;
            }
        }

        // Clamp servo PWM range
        if (pan >= 2500) pan = 2500;
        if (pan <= 500)  pan = 500;
        if (tilt >= 2500) tilt = 2500;
        if (tilt <= 500)  tilt = 500;

        // Move servos
        gpioServo(SERVO_PAN, pan);
        gpioServo(SERVO_TILT, tilt);

        // Always clear & redraw display
        clear();
        mvprintw(0, 0, "MANUAL MODE - press q to exit");
        mvprintw(1, 0, "Pan PWM:   %d", pan);
        mvprintw(2, 0, "Tilt PWM:  %d", tilt);

        // Always try reading TF Luna
        if (serialDataAvail(serial_port)) {
            int distance, strength;
            float temp;
            int data[9];

            read_tfluna_data(serial_port, data);

            distance = data[2] + data[3] * 256;
            strength = data[4] + data[5] * 256;
            temp = data[6] + data[7] * 256;
            temp = (temp / 8.0f) - 256.0f;

            mvprintw(3, 0, "TF Luna:");
            mvprintw(4, 0, "Distance: %d cm", distance);
            mvprintw(5, 0, "Signal Strength: %d", strength);
            mvprintw(6, 0, "Temperature: %.2f C", temp);
        }

        refresh();

        usleep(10000);  // ~20ms loop time
    }

    nodelay(stdscr, FALSE);
}


void servo_coordinates() {
    

    int curr_count = frames[frame_count].count;
    
    int high_bar = current_tracking_distance + 10;
    int low_bar = current_tracking_distance - 10;

    int pan_sum = 0, pan_count = 0;
    int sum_distance = 0;
    
    for (int i = 0; i < frames[frame_count].count; i++) {
	int pan = frames[frame_count].points[i].pan;
	int tilt = frames[frame_count].points[i].tilt;
	int distance = frames[frame_count].points[i].distance;
	
	
	if (distance > low_bar && distance < high_bar) {
	    pan_sum += pan;
	    pan_count++;
	    sum_distance += distance;
	}
	
	
    }
    
    //~ if (frame_count > 5) {
	//~ }
    
    if (pan_count > 0) {
	current_tracking_distance = sum_distance / pan_count;
	int pan_avg = pan_sum / pan_count;

	frames[frame_count].pan_avg = pan_avg;

	clear();
	mvprintw(1, 0, "Yes Data:");
	mvprintw(2, 0, "Current Pan: %d cm", pan);
	mvprintw(3, 0, "Average Pan: %d cm", pan_avg);
	mvprintw(3, 0, "Average Distance: %d cm", current_tracking_distance);


	refresh();
	
	
	if (pan_avg > pan) {
	    pan = pan + 33;}
	}
	else {
	    pan = pan - 33;}
    

    frame_count++;
    
}

void scanning_mode(int p, int t, int width) {
    
    FILE *fp = fopen("scan_results.txt", "w");
    if (!fp) {
	perror("Could not open scan_results.txt for writing");
	return;
    }
    
    int ch;
    
    int stop_p = p + width;
    int orig_p = p;
    
    int curr_count = 0;

    // Make getch non-blocking
    nodelay(stdscr, TRUE);
    

    while (1) {

	
	

	// tilt up    
	for (int angle = t; angle <= t + width; angle++) {
	    gpioServo(SERVO_TILT, angle);
	    usleep(1000);
	    
	    if (serialDataAvail(serial_port)) {
		int distance, strength;
		float temp;
		int data[9];

		read_tfluna_data(serial_port, data);

		distance = data[2] + data[3] * 256;
		strength = data[4] + data[5] * 256;
		temp = data[6] + data[7] * 256;
		temp = (temp / 8.0f) - 256.0f;

		//~ clear();
		//~ mvprintw(1, 0, "TF Luna:");
		//~ mvprintw(2, 0, "Distance: %d cm", distance);
		//~ mvprintw(3, 0, "Signal Strength: %d", strength);
		//~ mvprintw(4, 0, "Temperature: %.2f C", temp);
		//~ refresh();
		
		//~ // add data to DistanceStruct
		//~ data_array[struct_count].pan_struct = pan_count;
		//~ data_array[struct_count].tilt_struct = angle;
		//~ data_array[struct_count].distance_struct = distance;
		//~ struct_count++;
		
		if (strength < 1000) {
		    continue;}
		
		// update results file
		//~ fprintf(fp, "%d\t%d\t%d\n", p, angle, distance);
                //~ fflush(fp); // force write immediately
		curr_count = frames[frame_count].count;
		frames[frame_count].points[curr_count].pan = p;
		frames[frame_count].points[curr_count].tilt = angle;
		frames[frame_count].points[curr_count].distance = distance;
		frames[frame_count].count ++;
	    }
	
	}
        
	// pan
	gpioServo(SERVO_PAN, p);
        
        p = p + dir;
        
        if (p >= stop_p) {
            dir = -11;
        }
        
        if (p <= orig_p) {
            dir = 11;
	    
	    
            break;
        }

	// tilt down
        for (int angle = t + width; angle >= t; angle--) {
            gpioServo(SERVO_TILT, angle);
            usleep(1000);
            
	    if (serialDataAvail(serial_port)) {
		int distance, strength;
		float temp;
		int data[9];

		read_tfluna_data(serial_port, data);

		distance = data[2] + data[3] * 256;
		strength = data[4] + data[5] * 256;
		temp = data[6] + data[7] * 256;
		temp = (temp / 8.0f) - 256.0f;

		//~ clear();
		//~ mvprintw(1, 0, "TF Luna:");
		//~ mvprintw(2, 0, "Distance: %d cm", distance);
		//~ mvprintw(3, 0, "Signal Strength: %d", strength);
		//~ mvprintw(4, 0, "Temperature: %.2f C", temp);
		//~ refresh();
		
		// add data to DistanceStruct
		//~ data_array[struct_count].pan_struct = pan_count;
		//~ data_array[struct_count].tilt_struct = angle;
		//~ data_array[struct_count].distance_struct = distance;
		//~ struct_count++;
		
		if (strength < 1000) {
		    continue;}
		    
		curr_count = frames[frame_count].count;
		frames[frame_count].points[curr_count].pan = p;
		frames[frame_count].points[curr_count].tilt = angle;
		frames[frame_count].points[curr_count].distance = distance;
		frames[frame_count].count ++;

		
		// update results file
		//~ fprintf(fp, "%d\t%d\t%d\n", p, angle, distance);
                //~ fflush(fp); // force write immediately
		

	    }
            
        }
		
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
    
    initscr();
    keypad(stdscr, TRUE);
    
    noecho();
    cbreak();
    
    int ch; // character input
    
    // init file
    FILE *fp = fopen("scan_results.txt", "w");
    if (!fp) {
	perror("Could not open scan_results.txt");
    }

    // init gnuplot
    FILE *pipeForGNUPlot = popen("gnuplot -persistent", "w");
    if (!pipeForGNUPlot) {
	perror("Could not open pipe to Gnuplot");
	fclose(fp);
    }

    fprintf(pipeForGNUPlot, "set title \"Lidar Scan - Top Down Filled\"\n");
    fprintf(pipeForGNUPlot, "set xlabel \"Pan (PWM)\"\n");
    fprintf(pipeForGNUPlot, "set ylabel \"Tilt (PWM)\"\n");
    fprintf(pipeForGNUPlot, "set view map\n");
    fprintf(pipeForGNUPlot, "set pm3d map\n");
    fprintf(pipeForGNUPlot, "set palette defined (0 \"blue\", 1 \"green\", 2 \"yellow\", 3 \"red\")\n");
    fprintf(pipeForGNUPlot, "set dgrid3d 30,30 qnorm 2\n");
    //~ fprintf(pipeForGNUPlot, "pause -1 \"Press Enter to exit\"\n");


    while (1) {
		
	nodelay(stdscr, FALSE);  // ✅ make getch() non-blocking
	
	clear();
	mvprintw(0,0,"Press 1 to Enter Manual Mode");
	mvprintw(1,0,"Press 2 to Enter Scanning Mode");
	mvprintw(2,0,"Press 3 to View Results");
	mvprintw(3,0, "Press 4 to View Plot");
	mvprintw(4,0,"Presss q to Enter Main Menu");
	mvprintw(5,0,"Press Ctrl C to quit.\n");
	refresh();
	
	ch = getch();
	
	int move_pan, move_tilt;

	if (ch != ERR) {
	    switch (ch) {
		case '1':				
		    manual_mode();
		    break;
		case '2': 
		    
		    while (1) {
			
		    ch = getch();

		    if (ch != ERR) {
			if (ch == 'q') {
			    break;  // exit manual mode
			}
			    }
		    

		    scanning_mode(pan, tilt, 33);

		    servo_coordinates();

		    
		    //~ plot_mode(pipeForGNUPlot);
		    //~ delay(1000);
		    }
		    
		    //pclose(pipeForGNUPlot);

    
		    break;
		
		case '3':
		    break;
		case '4':
		    //~ plot_mode();
		    break;
				
		}
	}


    }
    
    return 0;
}
