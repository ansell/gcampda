/** 
 *
 *   Common stdio function implementation.
 *
 *   (c) Copyright 2004 Federico Bareilles <fede@iar.unlp.edu.ar>,
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
 *   TODO: ??? 
 **/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#if !defined(__APPLE__)
#  include <malloc.h>
#endif
#include <string.h>
#include "my_stdio.h"


/**
   NOTE: The functions dprintf() and vdprintf() (as found in the
   glibc2 library).

   These functions are GNU extensions, not in C or POSIX.  Clearly,
   the names were badly chosen.  Many systems (like MacOS) have
   incompatible functions called dprintf(), usually some debugging
   version of printf(), perhaps with a prototype like

   void dprintf (int level, const char *format, ...);

   This is my own implementation of fd_printf():

**/

int fd_printf(int fd, const char *fmt, ...)
{
	va_list ap;
	int l = -1;
	int n, size = 80;
	char *buff = NULL;

	if ((buff = malloc (size)) == NULL) return l;
	while (1) {
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		n = vsnprintf (buff, size, fmt, ap);
		va_end(ap);
		if (n > -1 && n < size) {
			break;
		}
		else {
			/* Else try again with more space. */
			if (n > -1)    /* glibc 2.1 */
				size = n+1; /* precisely what is needed */
			else           /* glibc 2.0 */
				size *= 2;  /* twice the old size */
			if ((buff = realloc (buff, size)) == NULL)
				return l;
		}
	}

	if (buff != NULL) {
		l = write(fd, buff, n);
		free(buff);
	}

	return l;
}
	
/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
char * stripwhite (char *string)
{
	char *s, *t;

	for (s = string; WHITESPACE (*s); s++);
    	if (*s == 0) return (s);
	t = s + strlen (s) - 1;
	while (t > s && WHITESPACE (*t)) t--;
	*++t = '\0';
	
	return s;
}


/* End */
