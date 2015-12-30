#include<unistd.h>
#include<string.h>
#include<stdio.h>
#include<fcntl.h>

class LinuxFile
{
private:
	int m_Handle;

public:
	LinuxFile(const char *pFile, int flags = O_RDWR)
	{
		m_Handle = open(pFile, flags);
	}

	~LinuxFile()
	{
		if (m_Handle != -1)
			close(m_Handle);
	}

	size_t Write(const void *pBuffer, size_t size)
	{
		return write(m_Handle, pBuffer, size);
	}

	size_t Read(void *pBuffer, size_t size)
	{
		return read(m_Handle, pBuffer, size);
	}

	size_t Write(const char *pText)
	{
		return Write(pText, strlen(pText));
	}

	size_t Write(int number)
	{
		char szNum[32];
		snprintf(szNum, sizeof(szNum), "%d", number);
		return Write(szNum);
	}
};

class LinuxGPIOExporter
{
protected:
	int m_Number;

public:
	LinuxGPIOExporter(int number)
		: m_Number(number)
	{
		LinuxFile("/sys/class/gpio/export", O_WRONLY).Write(number);
	}

	~LinuxGPIOExporter()
	{
		LinuxFile("/sys/class/gpio/unexport", 
			O_WRONLY).Write(m_Number);
	}
};

class LinuxGPIO : public LinuxGPIOExporter
{
public:
	LinuxGPIO(int number)
		: LinuxGPIOExporter(number)
	{
	}

	void SetValue(bool value)
	{
		char szFN[128];
		snprintf(szFN,
			sizeof(szFN), 
			"/sys/class/gpio/gpio%d/value",
			m_Number);
		LinuxFile(szFN).Write(value ? "1" : "0");
	}

	void SetDirection(bool isOutput)
	{
		char szFN[128];
		snprintf(szFN,
			sizeof(szFN), 
			"/sys/class/gpio/gpio%d/direction",
			m_Number);
		LinuxFile(szFN).Write(isOutput ? "out" : "in");
	}
};

/*int main(int argc, char *argv[])
{
	LinuxGPIO gpio27(27);
	gpio27.SetDirection(true);
	bool on = true;
	for (;;)
	{
		printf("Switching %s the LED...\n", on ? "on" : "off");
		gpio27.SetValue(on);
		on = !on;
		sleep(1);
	}
}*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <curses.h>
#include <fcntl.h>
#include <linux/joystick.h>

#define NUM_THREADS        3
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
	
	LinuxGPIO gpio27(27);
	gpio27.SetDirection(true);

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
                      
		wmove(stdscr, 12, 0);
		
		bool led_state = false;
		if (axis[1] > 100)
		{			
			addstr("Down");
		}
		else if (axis[1] < -100)
		{
			led_state = true;
			wmove(stdscr, 12, 0);
			addstr("Up");
		}
		else
		{
			addstr("      ");
		}
		
		gpio27.SetValue(led_state);
		
		wmove(stdscr, 15, 0);
		refresh();
	}
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>

int UdpServer()
{
	int sockfd, n;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t len;
	char mesg[1000];
	int returnv;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(1234);
	bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	for (;;)
	{
		len = sizeof(cliaddr);
		n = recvfrom(sockfd, mesg, 1000, 0, (struct sockaddr *)&cliaddr, &len);
		printf("-------------------------------------------------------\n");
		mesg[n] = 0;
		printf("Received the following:\n");
		printf("%s", mesg);
		printf("-------------------------------------------------------\n");
		returnv = sendto(sockfd, mesg, n, 0, (struct sockaddr *)&cliaddr, len);
		printf("Sent %d bytes.\n", returnv);
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
	
	if (thread_id == 2)
	{
		UdpServer();
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
