/** 
 *   Rutinas para la interface serie.
 *
 *   (c) Copyright 2007 Federico Bareilles <fede@iar.unlp.edu.ar>,
 *   Instituto Argentino de Radio Astronomia (IAR).
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *     
 *   The author does NOT admit liability nor provide warranty for any
 *   of this software. This material is provided "AS-IS" in the hope
 *   that it may be useful for others.
 *
 **/


#define _GNU_SOURCE

#include <string.h>
#include <termios.h>
#include <sys/file.h>
#include <my_stdio.h>


int rs232_open(char *name)
{
        int fd;
        struct termios tio;

        /* Open Serial Port : 8 bits data, 1 bit stop, no parity, 9600
           baud */
        if ((fd = open(name, O_RDWR | O_NOCTTY)) < 0 ){
                SYSLOG(LOG_ERR, "Cannot open serial port (%s).", name);
                return -1;
        }

        memset(&tio, 0, sizeof(tio));
        tio.c_cflag &= ~CRTSCTS ;

	/**
	 CBAUD is a Linux extension to the POSIX termios.h Terminal I/O
         definitions.  See O'Reilly, POSIX Programmer's Guide, Chapter 8.
         Use the POSIX cfsetispeed()/cfsetospeed() functions to set the
         input/output BAUD rate if CBAUD is not defined.
	**/

        /* Zero out the baud rate portion of c_cflag */
#ifdef CBAUD
        tio.c_cflag &= ~CBAUD;
        /* Set baud rate to 115200 */
        tio.c_cflag |= B115200;
#else
	cfsetispeed( &tio, B115200 );
        cfsetospeed( &tio, B115200 );

#endif
        tio.c_cflag &= ~(CSIZE|PARENB);
        tio.c_cflag |= CS8 ;
        //tio.c_cflag |= PARENB ;
        /* Ignore modem control lines */
        tio.c_cflag |= CLOCAL;
        /* Enable receiver */
        tio.c_cflag |= CREAD;

        /* Turn of Canonical Processing and echo */
        tio.c_lflag &= ~ICANON;
        tio.c_lflag &= ~ECHO;
        /* Set MIN to one and TIME to zero */
        tio.c_cc[VMIN]  = 1;
        tio.c_cc[VTIME] = 0;

        /* Ignore BREAK condition on input */
        tio.c_iflag |= IGNBRK;
        /* ignore framing errors and parity errors */
        tio.c_iflag |=IGNPAR;
        /* Disable NL to CR mapping */
        tio.c_iflag &= ~ICRNL;
        /* Disable XON/XOFF flow control on output and input */
        tio.c_iflag &= ~(IXON|IXOFF);

        /* now clean the modem line and activate the settings for modem */
        tcflush(fd, TCOFLUSH|TCIFLUSH);
        tcsetattr(fd, TCSANOW, &tio);

        return fd;
}

