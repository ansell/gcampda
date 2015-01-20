/** 
 *   I/O Runtime 
 *
 *   (c) Copyright 2007 Federico Bareilles <fede@iar.unlp.edu.ar>,
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


#ifndef _MY_IO_H
#define _MY_IO_H

int hw_close(int fd);
int hw_read(int fd, void *buff, size_t len);
int hw_write(int fd, void *buff, size_t count);
int hw_set_timeout(int sec);


#endif /* ifndef _MY_IO_H */
