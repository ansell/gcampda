/** 
 *   I/O Runtime 
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
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <my_stdio.h>
#include <errno.h>


static int global_timeout = 2;


//#define ECONNRESET      104     /* Connection reset by peer */

static ssize_t soft_read_time_out(struct timeval tv, int fd, void *buf,
                                  size_t count)
{
        fd_set rfds;
        int fd_max = fd + 1;
        int retval = 1, l = 0;
        struct timeval tvl;

        while ( l < count ) {
		memcpy(&tvl, &tv, sizeof(struct timeval));
                FD_ZERO(&rfds);
                FD_SET(fd, &rfds);
                retval = select(fd_max, &rfds, NULL, NULL, &tvl);
                if (retval == 1) {
                        if( FD_ISSET(fd, &rfds) ) {
				retval = read(fd, buf+l, count - l);
				if ( retval <= 0 ) { /* FIXME: */
					SYSLOG(LOG_CRIT, 
					       "Lost conection: %s (%d)",
					       strerror(errno), errno);
					return -2;
				}
                                l += retval;
			}
                } else{
			PDBG("time out reading %d/%d.", l, count);
                        return -1;
		}
        }

        return l;
}

int hw_close(int fd)
{
        return close(fd);
}


int hw_read(int fd, void *buff, size_t len)
{
	struct timeval tv = {
		.tv_sec = global_timeout,
		.tv_usec = 0
	};

	return 	soft_read_time_out(tv, fd, buff, len);
}


int hw_write(int fd, void *buff, size_t count)
{
	fd_set wfds;
	struct timeval tv, tvr;
	int retval;
	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);
	tv.tv_sec = 1;//0;
	tv.tv_usec = 0;//20000;

	tvr.tv_sec = 0;
	tvr.tv_usec = 20000;
	retval = select(fd+1, NULL, &wfds, NULL, &tv);
	if (retval && FD_ISSET(fd, &wfds)) {
		retval = write(fd, buff, count);
#if 0
#ifdef VERBOSE_MODE
		if (verbose_flag & PINFO) {
			int __i;
			fprintf(stderr, __FILE__ " write -> ");
			for(__i=0;__i<count;__i++)
				fprintf(stderr, "%02x ",
					*(((u_char *)buff)+__i) );
			fprintf(stderr, "\n");
		}
#endif
#endif	
	} else {
		SYSLOG(LOG_WARNING, "time out.");
	}

	return retval;
}


int hw_set_timeout(int sec)
{
	global_timeout = sec;

	return 0;
}
