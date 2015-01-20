/** 
 *   Socket initialization (AF_INET and PF_UNIX); server
 *   side. Originally development for antenna position system.
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

#ifndef _SOCKET_SRV_H
#define _SOCKET_SRV_H

int socket_init( int port );
int socket_init_local( char *name );
int socket_wait( int s );

#endif  /* ifndef _SOCKET_SRV_H */
