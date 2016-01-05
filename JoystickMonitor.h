#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <fcntl.h>
#include <linux/joystick.h>

#define JOY_DEV  "/dev/input/js0"

class JoystickMonitor
{
public:
   JoystickMonitor() :
      axis(NULL),
      button(NULL),
      num_of_buttons(0),
      num_of_axis(0)
   {
      if ((joy_fd = open(JOY_DEV, O_RDONLY)) == -1) {
         printf("Couldn't open joystick ");
         while (1) {
            usleep(100000);
         }
      }

      ioctl(joy_fd, JSIOCGAXES, &num_of_axis);            //        GET THE NUMBER OF AXIS ON JS
      ioctl(joy_fd, JSIOCGBUTTONS, &num_of_buttons);      //        GET THE NUMBER OF BUTTONS ON THE JS
      ioctl(joy_fd, JSIOCGNAME(80), &name_of_joystick);   //        GET THE NAME OF THE JS

      axis = (int *) calloc(num_of_axis, sizeof(int));
      button = (char *) calloc(num_of_buttons, sizeof(char));

      printf("%d", num_of_axis);
      printf("%d", num_of_buttons);
      
      //fcntl(joy_fd, F_SETFL, O_NONBLOCK); //Let's keep blocking mode 
   }
   
   void Update(){
      //  READ THE JOYSTICK STATE, IT WILL BE RETURNED IN THE JS_EVENT STRUCT
      read(joy_fd, &js, sizeof(struct js_event));

      // GET THE NUMBER OF SECONDS SINCE EPOCH
      ftime(&seconds);
              
      // CHECK THE EVENT
      switch (js.type & ~JS_EVENT_INIT) {
      case JS_EVENT_AXIS:
         axis[js.number] = js.value;
         break;
      case JS_EVENT_BUTTON:
         button[js.number] = js.value;
         break;
      }
   }
   
   int GetAxisValue(int index){
      return axis[index];
   }
   
   int GetButtonValue(int index) {
      return button[index];
   }
   
private:
   struct timeb seconds;
   int cnt;
   
   int joy_fd; 
   
   int* axis; 
   char* button; 
   int num_of_buttons;
   int num_of_axis;

   char* name_of_joystick[80];
   
   struct js_event js;
   time_t theTime;  
   
};
