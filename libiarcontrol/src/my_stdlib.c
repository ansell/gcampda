/** 
 *
 *   Common stdlib function implementation.
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
 *   TODO: ??? 
 **/

#define _GNU_SOURCE

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#if defined(__APPLE__)
  extern char **environ;
#endif

int my_system (char *command) {
           int pid, status;

           if (command == 0)
               return 1;
           pid = fork();
           if (pid == -1)
               return -1;
           if (pid == 0) {
               char *argv[4];
               argv[0] = "sh";
               argv[1] = "-c";
               argv[2] = command;
               argv[3] = 0;
               execve("/bin/sh", argv, environ);
               exit(127);
           }
           do {
               if (waitpid(pid, &status, 0) == -1) {
                   if (errno != EINTR)
                       return -1;
               } else
                   return status;
           } while(1);
}


	

/* End */
