/** 
 *   cr_fd: 
 *
 *
 *   (c) Copyright 2010 Federico Bareilles <bareilles@gmail.com>,
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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pakbus.h"
#include <my_stdio.h>


static CR_FD *cr_fd_make(void)
{
	CR_FD *d = (CR_FD *) malloc(sizeof(CR_FD));
	
	CR_FD_SIZE(d) = 0;
	CR_FD_DATE(d) = CR_FD_NAME(d) =	NULL;
	CR_FD_NEXT(d) = NULL;
	memset(CR_FD_ATTR(d), 0, 12);

	return d;
}


int cr_fd_store(CR_FD **d, void *buff, size_t buff_size)
{
	CR_FD *dp;
	int len = 0;
	int i;
	unsigned char *fsize = NULL;

	if (*d == NULL) *d = cr_fd_make();
	dp = *d;
	while( CR_FD_NEXT(dp) != NULL) dp = CR_FD_NEXT(dp);
	
	len += asprintf(&CR_FD_NAME(dp),"%s", (char *) (buff+len));
	len++;
	fsize = (unsigned char *) buff + len;
	CR_FD_SIZE(dp) = (((int32_t) *(fsize)   & 0xff) << 24) |
			(((int32_t) *(fsize+1) & 0xff) << 16) |
			(((int32_t) *(fsize+2) & 0xff) << 8) |
			((int32_t) *(fsize+3) & 0xff);
	len += 4;
	len += asprintf(&CR_FD_DATE(dp),"%s", (char *) (buff+len));
	len++;
	i = 0;
	while((unsigned char) *((unsigned char *)buff+len) != 0x00) { 
		CR_FD_ATTR(dp)[i++] = *((unsigned char *)buff+len);
		len++;
	}
	if((unsigned char) *((unsigned char *)buff+len) == 0x00) len++;
	CR_FD_NEXT(dp) = cr_fd_make();

	if (len < buff_size)
		len += cr_fd_store(&CR_FD_NEXT(dp), 
				   (void *) ((char *) buff+len),
				   buff_size - len);
	return len;
}


void cr_fd_free(CR_FD **cr_fd)
{
	CR_FD *d = *cr_fd;
	CR_FD *dp; 

	while(d != NULL){
                dp = d;
                d = CR_FD_NEXT(d);
		/* free dp: */
		if( CR_FD_NAME(dp) != NULL) free(CR_FD_NAME(dp));
		if( CR_FD_DATE(dp) != NULL) free(CR_FD_DATE(dp));
		free(dp);
        }
	*cr_fd = NULL;

	return;
}
