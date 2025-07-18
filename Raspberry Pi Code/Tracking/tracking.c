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

int pan = 1620; // global pan pwm variable
int tilt = 1970; // global tilt pwm variable

// used for servo scanning
int pan_count = 1666;
int dir = 5;
int struct_count = 0;

typedef struct {
    int id;
    int pan_struct;
    int tilt_struct;
    int distance_struct;
} DistanceStruct;

DistanceStruct data_array[8000];

void plot_mode() {
	
	
	FILE *pipeForGNUPlot = NULL;

    pipeForGNUPlot = popen("gnuplot -persistent", "w");

    if (pipeForGNUPlot != NULL) {
		fprintf(pipeForGNUPlot, "set title \"Lidar Scan - Top Down Filled\"\n");
		fprintf(pipeForGNUPlot, "set xlabel \"Pan (PWM)\"\n");
		fprintf(pipeForGNUPlot, "set ylabel \"Tilt (PWM)\"\n");
		fprintf(pipeForGNUPlot, "set view map\n");
		fprintf(pipeForGNUPlot, "set pm3d map\n");
		fprintf(pipeForGNUPlot, "set palette defined (0 \"blue\", 1 \"green\", 2 \"yellow\", 3 \"red\")\n");
		fprintf(pipeForGNUPlot, "set dgrid3d 30,30 qnorm 2\n");
		fprintf(pipeForGNUPlot, "splot \"scan_results.txt\" using 1:2:3 with pm3d notitle\n");
		fprintf(pipeForGNUPlot, "pause -1 \"Press Enter to exit\"\n");


        pclose(pipeForGNUPlot);
        
    } else {
        printf("Could not open pipe to Gnuplot.\n");
    }
	

		
}

void results_mode() {
    int ch;
    nodelay(stdscr, TRUE);

    FILE *fp = fopen("scan_results.txt", "w");
    if (fp == NULL) {
        mvprintw(0, 0, "ERROR: Could not open file to write results.");
        refresh();
        sleep(2);
        return;
    }

    int row = 2; // start rows below header
    int max_rows, max_cols;
    getmaxyx(stdscr, max_rows, max_cols);

    clear();
    mvprintw(0, 0, "RESULTS MODE - %d points collected.", struct_count);
    mvprintw(1, 0, "Press q to return. Also saving to scan_results.txt");

    for (int i = 0; i < struct_count; i++) {
        // Write each entry to file: PAN TILT DISTANCE
        fprintf(fp, "%d\t%d\t%d\n",
                data_array[i].pan_struct,
                data_array[i].tilt_struct,
                data_array[i].distance_struct);

        // Display on screen
        int col = (i % 4) * 20; // 4 columns max, 20 chars wide for neat spacing

        if (row >= max_rows - 1) {
            mvprintw(max_rows - 1, 0, "Press any key to continue...");
            refresh();
            nodelay(stdscr, FALSE);
            getch();
            nodelay(stdscr, TRUE);
            clear();
            mvprintw(0, 0, "RESULTS MODE - %d points collected.", struct_count);
            mvprintw(1, 0, "Press q to return. Also saving to scan_results.txt");
            row = 2;
        }

        mvprintw(row, col, "P:%d T:%d D:%d",
                 data_array[i].pan_struct,
                 data_array[i].tilt_struct,
                 data_array[i].distance_struct);

        if ((i + 1) % 4 == 0) row++;
    }

    fclose(fp);

    refresh();
    
	// ✅ Clear data after viewing
    struct_count = 0;
    memset(data_array, 0, sizeof(data_array));

    nodelay(stdscr, FALSE); // restore blocking input

    // Wait for q to quit results mode
    while (1) {
        ch = getch();
        if (ch == 'q') {
            break;
        }
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
    int speed = 10;

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

        if (pan >= 2000) pan = 2000;
        if (pan <= 1000) pan = 1000;
        if (tilt >= 2000) tilt = 2000;
        if (tilt <= 1000) tilt = 1000;

        gpioServo(SERVO_PAN, pan);
        gpioServo(SERVO_TILT, tilt);

        clear();
        mvprintw(0, 0, "MANUAL MODE - press q to exit");
        mvprintw(2, 0, "Pan PWM:   %d", pan);
        mvprintw(3, 0, "Tilt PWM:  %d", tilt);
        refresh();

        usleep(20000); // small delay
    }

    // restore blocking mode when you return
    nodelay(stdscr, FALSE);
}

void scanning_mode() {
	int ch;
	
	int pan_count = 1666;

    // Make getch non-blocking
    nodelay(stdscr, TRUE);
    

    while (1) {
        ch = getch();

        if (ch != ERR) {
            if (ch == 'q') {
                break;  // exit manual mode
            }
		}
		
		
		for (int angle = 2000; angle <= 2033; angle++) {
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

				clear();
				mvprintw(1, 0, "TF Luna:");
				mvprintw(2, 0, "Distance: %d cm", distance);
				mvprintw(3, 0, "Signal Strength: %d", strength);
				mvprintw(4, 0, "Temperature: %.2f C", temp);
				refresh();
				
				// add data to DistanceStruct
				data_array[struct_count].pan_struct = pan_count;
				data_array[struct_count].tilt_struct = angle;
				data_array[struct_count].distance_struct = distance;
				struct_count++;
			}
            
        }
        
		gpioServo(SERVO_PAN, pan_count);
        
        pan_count = pan_count + dir;
        
        if (pan_count > 1700) {
            dir = -5;
        }
        
        if (pan_count < 1666) {
            dir = 5;
            break;
        }

        for (int angle = 2033; angle >= 2000; angle--) {
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

				clear();
				mvprintw(1, 0, "TF Luna:");
				mvprintw(2, 0, "Distance: %d cm", distance);
				mvprintw(3, 0, "Signal Strength: %d", strength);
				mvprintw(4, 0, "Temperature: %.2f C", temp);
				refresh();
				
				// add data to DistanceStruct
				data_array[struct_count].pan_struct = pan_count;
				data_array[struct_count].tilt_struct = angle;
				data_array[struct_count].distance_struct = distance;
				struct_count++;
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

		if (ch != ERR) {
			switch (ch) {
				case '1':				
					manual_mode();
					break;
				case '2':
					scanning_mode(); 
					break;
				case '3':
					results_mode();
					break;
				case '4':
					plot_mode();
					break;
					
			}
		}


    }
    
    return 0;
}
