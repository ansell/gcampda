/** 
 *   ib_fd: IAR Baus, file descriptor extension.  
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
 **/



#define _GNU_SOURCE

#include "ib_fd.h"



IBFD *ib_fd_new (void)
{
	IBFD *dc = (IBFD *) malloc(sizeof(IBFD));
	IBFD_GET(dc) = -1;
	IBFD_STATUS(dc) = 0;
	IBFD_GET_USER_DATA(dc) = NULL;
	
	return dc;
}


void ib_fd_destroy (IBFD *fd)
{
	if( IBFD_GET_USER_DATA(fd) != NULL)
		free(IBFD_GET_USER_DATA(fd));
	if ( fd != NULL)
		free(fd);

	return;
}
