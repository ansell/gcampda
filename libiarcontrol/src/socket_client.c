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
#define _GNU_SOURCE
//#define _POSIX_SOURCE 1         /* POSIX compliant source  */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "my_stdio.h"
#ifndef WIN32
# include <netdb.h>
# include <sys/socket.h>
# include <sys/un.h>
#if !defined(__APPLE__)
# include <error.h>
#else
# include <mach/error.h>
#endif
#else
# include <windows.h> /* http://support.microsoft.com/kb/q257460/ */
#endif /* WIN32 */

#include "socket_client.h"


int call_socket(char *hostname, int portnum)
{ 
	struct sockaddr_in sa;
	struct hostent     *hp;
	int s;
#ifdef WIN32
	WSADATA wsaData;
#endif

#ifdef WIN32
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
	{
		PVERB(PERR, "WSAStartup() failed");
		return -2;
	}
#endif
	if ((hp= gethostbyname(hostname)) == NULL) {/* do we know the host's */
		return(-1); 
	}

	memset(&sa,0,sizeof(sa));
	/* set address */
	memcpy((char *)&sa.sin_addr, hp->h_addr, hp->h_length); 
	sa.sin_family= hp->h_addrtype;
	sa.sin_port= htons(portnum);

	if ((s = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0)   /* get socket */
		return(-1);
	if (connect(s,(struct sockaddr *)&sa,sizeof(sa)) < 0) { /* connect */

		close_socket(s);

		return(-1);
	}
	return s;
}


#ifndef WIN32
int call_socket_local(char *name)
{ 
	struct sockaddr_un sa;
	int s;

	memset(&sa,0,sizeof(sa));
	sa.sun_family= PF_UNIX;
	strcpy(sa.sun_path, name);

	if ((s= socket(PF_UNIX, SOCK_STREAM, 0)) < 0)   /* get socket */
		return(-1);
	if (connect(s,(struct sockaddr *)&sa,sizeof(sa)) < 0) { /* connect */
		close(s);
		return(-1);
	}
	return s;
}
#endif /* WIN32 */


int close_socket(int fd)
{
	int ret;

#ifndef WIN32
	ret = close(fd);
#else
	ret = closesocket(fd);
	WSACleanup();
#endif

	return ret;
}
