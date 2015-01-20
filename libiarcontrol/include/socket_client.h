/** 
 *   socket_client: IPC multi-platform on virtual BUS; socket
 *   inplementation for client side.
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

#ifndef _SOCKET_CLIENT_H
#define _SOCKET_CLIENT_H

int call_socket(char *hostname, int portnum);
#ifndef WIN32
int call_socket_local(char *name);
#endif
int close_socket(int fd);

/* Explicitly not undefined sock_* for platform checking */
#if defined(__unix__)
# define sock_read(fd, buf, count) read(fd, buf, count)
# define sock_write(fd, buf, count) write(fd, buf, count)
#endif
#if defined(__APPLE__)
# define sock_read(fd, buf, count) read(fd, buf, count)
# define sock_write(fd, buf, count) write(fd, buf, count)
#endif
#if defined(WIN32)
# define sock_read(fd, buf, count) recv(fd, (void *) buf, count, 0)
# define sock_write(fd, buf, count) send(fd, (void *) buf, count, 0)
#endif


#endif  /* ifndef _SOCKET_CLIENT_H */
