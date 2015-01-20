/** 
 *   Camera Control for STAR I (originally).
 *
 *   Included in libiarcontrol v 0.1.5
 *
 *   (c) Copyright 2006 Federico Bareilles <fede@iar.unlp.edu.ar>,
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>

#include "locks.h"
#include "my_stdio.h"


static int lock_pid(char *name)
{
	int fd;
	char strpid[16];
	int ret = 0;

	fd = open(name, O_RDONLY);
	if (fd > 0) {
		ret = read(fd, strpid, 15);
		strpid[ret] = 0;
		close(fd);
		ret = atoi(strpid);
	}

	return ret;
}




int lock_file(char *filename)
{
	int fd;
	char name[_POSIX_PATH_MAX];
	char proc[_POSIX_PATH_MAX];
	int ret = -1;
	struct stat dummy;

	sprintf(name,"%s%s", filename, LOCK_EXT);
	fd = open(name, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
	if (fd > 0) {
		fd_printf(fd,"%d", getpid());
		close(fd);
		ret = 0;
	}else{ /* verificar que el pid sea valido */
		sprintf(proc,"/proc/%d", lock_pid(name));
		if (stat(proc, &dummy) == 0) {
			ret = 1;
		} else {
			unlink(name);
			ret = lock_file(filename);
		}
	}
	return ret;
}


int unlock_file(char *filename)
{
	char name[_POSIX_PATH_MAX];

	sprintf(name,"%s%s", filename, LOCK_EXT);

	return unlink(name);
}


int if_lock_file(char *filename)
{
	int fd;
	char name[_POSIX_PATH_MAX];
	int ret = 1;

	sprintf(name,"%s%s", filename, LOCK_EXT);
	fd = open(name, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
	if (fd > 0) {
		close(fd);
		unlink(name);
		ret = 0;
	}else{
		ret = lock_pid(name);
	}

	return ret;
}

