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

#ifndef _LOCKS_H
#define _LOCKS_H

#define LOCK_EXT ".lock"

int lock_file(char *filename);
int unlock_file(char *filename);
int if_lock_file(char *filename);


#endif /* ifndef _LOCKS_H */
