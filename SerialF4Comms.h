#pragma once

#include <termios.h>		//Used for UART
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <vector>

class SerialF4Comms 
{
public:
   SerialF4Comms() {
      //-------------------------
	   //----- SETUP USART 0 -----
	   //-------------------------
	   //At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
      uart0_filestream = -1;
	
      //OPEN THE UART
      //The flags (defined in fcntl.h):
      //	Access modes (use 1 of these):
      //		O_RDONLY - Open for reading only.
      //		O_RDWR - Open for reading and writing.
      //		O_WRONLY - Open for writing only.
      //
      //	O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
      //											if there is no input immediately available (instead of blocking). Likewise, write requests can also return
      //											immediately with a failure status if the output can't be written immediately.
      //
      //	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
      uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
      if (uart0_filestream == -1) {
      	//ERROR - CAN'T OPEN SERIAL PORT
         printf("Error - Unable to open UART.  Ensure it is not in use by another application\r\n");
         sleep(1);
      }
      else {
         printf("Opened UART0\r\n");
         sleep(1);
      }
	
      //CONFIGURE THE UART
      //The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
      //	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
      //	CSIZE:- CS5, CS6, CS7, CS8
      //	CLOCAL - Ignore modem status lines
      //	CREAD - Enable receiver
      //	IGNPAR = Ignore characters with parity errors
      //	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
      //	PARENB - Parity enable
      //	PARODD - Odd parity (else even)
      struct termios options;
      tcgetattr(uart0_filestream, &options);
      options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
      options.c_iflag = IGNPAR;
      options.c_oflag = 0;
      options.c_lflag = 0;
      tcflush(uart0_filestream, TCIFLUSH);
      tcsetattr(uart0_filestream, TCSANOW, &options);
   }
   
   void SendPacket(bool m1_forward, bool m1_reverse, bool m2_forward, bool m2_reverse) {
      uint8_t motor_command_flags = 0;
      motor_command_flags |= (m1_forward << 0);
      motor_command_flags |= (m1_reverse << 1);
      motor_command_flags |= (m2_forward << 2);
      motor_command_flags |= (m2_reverse << 3);
      
      std::vector<char> packet;
      packet.push_back('R');
      packet.push_back('S');
      packet.push_back(motor_command_flags);
      
      if (uart0_filestream != -1)
      {
         int count = write(uart0_filestream, packet.data(), packet.size());		//Filestream, bytes to write, number of bytes to write
         if (count < 0)
         {
            printf("UART TX error\n");
         }
      }
   }
   
private:
   int uart0_filestream;
};
