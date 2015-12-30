
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <curses.h>
#include <fcntl.h>
#include <linux/joystick.h>

#define NUM_THREADS        2
#define JOY_DEV            "/dev/input/js0"

char axisDesc[15][5] = { "X", "Y", "Z", "R", "A", "B", "C", "D", "E", "F", "G", "H", "I" };
char numAxis[20], numButtons[20], value[20][20], timeSec[20], timeMilli[20];
int num_of_axis;

void write_to_screen() {
	struct timeb seconds;
	int cnt, joy_fd, *axis = NULL, num_of_buttons = 0, x;
	char *button = NULL, name_of_joystick[80];
	struct js_event js;
	time_t theTime;

	wmove(stdscr, 8, 0);
	addstr("Type \"q\" to quit.\n");

	if ((joy_fd = open(JOY_DEV, O_RDONLY)) == -1) {
		wmove(stdscr, 3, 0);
		addstr("Couldn't open joystick ");
		addstr(JOY_DEV);
		wmove(stdscr, 10, 0);
		while (1)usleep(16666);
	}

	ioctl(joy_fd, JSIOCGAXES, &num_of_axis);                                //        GET THE NUMBER OF AXIS ON JS
	ioctl(joy_fd, JSIOCGBUTTONS, &num_of_buttons);                //        GET THE NUMBER OF BUTTONS ON THE JS
	ioctl(joy_fd, JSIOCGNAME(80), &name_of_joystick);        //        GET THE NAME OF THE JS

	axis = (int *) calloc(num_of_axis, sizeof(int));
	button = (char *) calloc(num_of_buttons, sizeof(char));

	sprintf(numAxis, "%d", num_of_axis);
	sprintf(numButtons, "%d", num_of_buttons);

	wmove(stdscr, 0, 0);
	addstr("Joystick detected: ");

	wmove(stdscr, 0, 19);
	addstr(name_of_joystick);

	wmove(stdscr, 1, 0);
	addstr("Number of axis  :");

	wmove(stdscr, 1, 19);
	addstr(numAxis);
        
	wmove(stdscr, 2, 0);
	addstr("Number of buttons:");

	wmove(stdscr, 2, 19);
	addstr(numButtons);

	//        CHANGE THE STATUS FLAG OF THE FILE DESCRIPTOR TO NON-BLOCKING MODE
	fcntl(joy_fd, F_SETFL, O_NONBLOCK);  

	while (1) {
		usleep(16666);
        
//        READ THE JOYSTICK STATE, IT WILL BE RETURNED IN THE JS_EVENT STRUCT
		read(joy_fd, &js, sizeof(struct js_event));

		        //        GET THE NUMBER OF SECONDS SINCE EPOCH
		ftime(&seconds);
		theTime = time(NULL);
		sprintf(timeSec, "%d", seconds.time);                                                
		sprintf(timeMilli, "%d", seconds.millitm);
              
//        CHECK THE EVENT
		switch (js.type & ~JS_EVENT_INIT) {
		case JS_EVENT_AXIS:
			axis[js.number] = js.value;
			break;
		case JS_EVENT_BUTTON:
			button[js.number] = js.value;
			break;
		}
		//        ADD LEADING 0'S TO THE MILLISECOND STRING (IF NECESSARY)
		char temp[10] = "";
		while (strlen(temp) < (3 - strlen(timeMilli)))
			strcat(temp, "0");
		strcat(temp, timeMilli);
		strcpy(timeMilli, temp);

		                //        PRINT THE RESULTS
		wmove(stdscr, 4, 0);
		for (cnt = 0; cnt < num_of_axis; cnt++)
			addstr("            ");
		for (cnt = 0; cnt < num_of_axis && cnt < 13; cnt++) {
			sprintf(value[cnt], "%d", axis[cnt]);
			wmove(stdscr, 4, cnt * 10);
			addstr(axisDesc[cnt]);
			addstr(": ");
			wmove(stdscr, 4, (cnt * 10) + 2);
			addstr(value[cnt]);
		}
                        
		wmove(stdscr, 6, 0);
		addstr(timeSec);
		wmove(stdscr, 6, 10);
		addstr(".");
		wmove(stdscr, 6, 11);
		addstr(timeMilli);
		wmove(stdscr, 6, 15);
		addstr("=");
		wmove(stdscr, 6, 17);
		addstr(ctime(&theTime));
                      
		wmove(stdscr, 10, 0);
		refresh();
	}
}

void *ThreadProcs(void *threadid) {
	int thread_id = (int)threadid;
        
	if (thread_id == 0) {
		write_to_screen();
	}
        
	if (thread_id == 1) {
	//        THIS THREAD WILL MAKE THE PROGRAM EXIT
		int ch;
		nodelay(stdscr, TRUE);                         //        SETUP NON BLOCKING INPUT
		while (1) {
			if ((ch = getch()) == ERR) usleep(16666);  //        USER HASN'T RESPONDED
			else if (ch == 'q') {                        
				endwin();
				exit(0);                               //        QUIT ALL THREADS
			}
		}
	}
}

int main(int argc, char *argv[]) {
	initscr();                                         //        INIT THE SCREEN FOR CURSES
	pthread_t threads[NUM_THREADS];
	int rc, t;
	for (t = 0; t < NUM_THREADS; t++) {                //        MAKE 2 NEW THREADS
		rc = pthread_create(&threads[t], NULL, ThreadProcs, (void *)t);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1); pthread_exit(NULL);
		}
	}
        
	for (t = 0; t < NUM_THREADS; t++)
		pthread_join(threads[t], NULL);                //        WAIT FOR THREADS TO EXIT OR IT WILL RACE TO HERE.

	endwin();
	return 0;
}
