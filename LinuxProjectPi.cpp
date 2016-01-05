#include <termios.h>		//Used for UART
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <curses.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include "JoystickMonitor.h"
#include "SerialF4Comms.h"

#define NUM_THREADS        4

#define STRING_BUFFER_SIZE 20
char stringBuffer[STRING_BUFFER_SIZE];

void MapJoystickToRelays(float joystick, bool &up_relay, bool &down_relay)
{
   bool temp_up = false;
   bool temp_down = false;
   if (joystick > 1000)
   {
      temp_up = true;
   }
   else if (joystick < -1000)
   {
      temp_down = true;
   }
   else
   {
      
   }
   up_relay = temp_up;
   down_relay = temp_down;
}

void write_to_screen() {
   while (1) {
      sleep(1);
   }
}

JoystickMonitor *joystickMonitor;
SerialF4Comms *serialF4Comms;

void JoystickMonitorThread()
{
   while (1) {
      joystickMonitor->Update();
      //printf("Joystick Value = %d\r\n", joystickMonitor.GetAxisValue(1));
   }
}

void SendF4Packets() {
   bool m1_forward, m1_reverse, m2_forward, m2_reverse;
   
   wmove(stdscr, 0, 0);
   addstr("Running Joystick to F4 Application");
   
   while (1) {
      MapJoystickToRelays(-joystickMonitor->GetAxisValue(1), m1_forward, m1_reverse);
      MapJoystickToRelays(joystickMonitor->GetAxisValue(2), m2_forward, m2_reverse);
      serialF4Comms->SendPacket(m1_forward, m1_reverse, m2_forward, m2_reverse);
      
      wmove(stdscr, 2, 0);
      sprintf(stringBuffer, "M1_Forward = %d", m1_forward);
      addstr(stringBuffer);
      wmove(stdscr, 2, 20);
      sprintf(stringBuffer, "M1_Reverse = %d", m1_reverse);
      addstr(stringBuffer);
      wmove(stdscr, 4, 0);
      sprintf(stringBuffer, "M2_Forward = %d", m2_forward);
      addstr(stringBuffer);
      wmove(stdscr, 4, 20);
      sprintf(stringBuffer, "M2_Reverse = %d", m2_reverse);
      addstr(stringBuffer);
      refresh();
      
      usleep(100000);
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
		nodelay(stdscr, TRUE);                        //        SETUP NON BLOCKING INPUT
		while (1) {
			if ((ch = getch()) == ERR) usleep(16666);  //        USER HASN'T RESPONDED
			else if (ch == 'q') {                        
				endwin();
				exit(0);                                //        QUIT ALL THREADS
			}
		}
	}
   
   if (thread_id == 2) {
      JoystickMonitorThread();
   }
   
   if (thread_id == 3) {
      SendF4Packets();
   }
}

int main(int argc, char *argv[]) {
   system("sudo chmod 777 /dev/ttyAMA0");
   
   joystickMonitor = new JoystickMonitor();
   serialF4Comms = new SerialF4Comms();
   
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
