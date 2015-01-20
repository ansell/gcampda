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

#ifndef _IB_FD_H
#define _IB_FD_H

#if defined(__APPLE__)
#  include <stdlib.h>
#else
#  include <malloc.h>
#endif

#include <stdint.h> /* for uintXX_t definition */

typedef struct _ib_fd {
	int fd;   /* file descriptor */
	uint32_t iar_bus_status; /* internal use in IAR Bus */
	void *user_data; /* Anonymous user data pointer. */
	/* uint32_t type; */ 
} IBFD;

#define IBFD_GET(f) (f)->fd
#define IBFD_STATUS(f) (f)->iar_bus_status
#define IBFD_IF_CON_MSG(f) (IBFD_STATUS(f) & IB_DEV_CON_MSG)
#define IBFD_GET_USER_DATA(f) ((f)->user_data)


IBFD *ib_fd_new (void);
void ib_fd_destroy (IBFD *fd);

#endif  /* ifndef _IB_FD_H */




