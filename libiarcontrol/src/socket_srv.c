/** 
 *   Socket initialisation (AF_INET and PF_UNIX).
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

#define LOCAL_PERROR 1
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include "my_stdio.h"

#undef PERROR
#if LOCAL_PERROR 
#define PERROR(s) {fprintf(stderr, "%s:", __FUNCTION__) ; perror(s);}
#else 
#define PERROR(s)
#endif


/* code to establish a socket */
int socket_init( int port )
{
	int s;
	socklen_t l;
	struct sockaddr_in sa;
	int reuse = 1;

	memset(&sa, 0, sizeof(struct sockaddr_in));  /* clear our address */
	sa.sin_family = PF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(port);

	/* create socket */
	s = socket(PF_INET, SOCK_STREAM, 0);

	if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
			(char *)&reuse,sizeof(reuse)) ) {
		PERROR("setsockopt"); 
		return -1;
	}

	if (s < 0) {
		PERROR("socket");
		return(-1);
	}
	l = sizeof(sa);
	/* bind address to socket */
	if (bind(s, (struct sockaddr *)&sa, l) < 0) {
		PERROR("bind"); 
		close(s);
		return(-1);
	}
	
	/* max # (1) of queued connects */
	listen(s, 1);

	getsockname(s, (struct sockaddr *)&sa, &l);
	PVERB(PINFO, "Wait for connect in port %d",
		ntohs(sa.sin_port));

	return s;
}


int socket_init_local( char *name )
{

	int s;
	socklen_t l;
	struct sockaddr_un sa;
	int reuse = 1;

	memset(&sa, 0, sizeof(struct sockaddr_un));  /* clear our address */
	sa.sun_family = PF_UNIX;
	strcpy(sa.sun_path,  name);

	/* create socket */
	s = socket(PF_UNIX, SOCK_STREAM, 0);

	if (s < 0) {
		PERROR("socket");
		return(-1);
	}

	if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
			(char *)&reuse,sizeof(reuse)) ) {
		PERROR("setsockopt\n"); 
		return -1;
	}

	l = sizeof(sa);
	/* bind address to socket */
	unlink(sa.sun_path);
	if (bind(s, (struct sockaddr *)&sa, l) < 0) {
		PERROR("bind"); 
		close(s);
		return(-1);
	}
	
	/* max # (1) of queued connects */
	listen(s, 1);

	getsockname(s, (struct sockaddr *)&sa, &l);
	PVERB(PINFO, "Wait for connect socket %s",
		sa.sun_path);

	return s;

}


int socket_wait( int s ) /* s: socket created with establish() */
{
	struct sockaddr_in isa;
	socklen_t addrlen;
	int t;
	
	addrlen=sizeof(isa); /* find socket's address for accept() */
	getsockname(s, (struct  sockaddr  *)&isa, &addrlen);
	t = accept(s, (struct sockaddr *)&isa, &addrlen);
	if (t < 0) { /* accept connection if there is one */
		PERROR("accept");
		return -1;
	}

	PVERB(PINFO, "Connect from %s [%d]", inet_ntoa(isa.sin_addr), 
	      ntohs(isa.sin_port ));
	
	return t;
}

