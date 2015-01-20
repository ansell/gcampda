/** 
 *   Parse line configuration.
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

#ifndef _LINE_CONF_H
#define _LINE_CONF_H

#define LINE_LEN_CONF 160 /* Obsolete: used in stari apication only*/
#define LINE_BASE_LEN_CONF 80

/* Implemented in parse_line_conf.c: */
int parse_line(char *line, char **tag, char **data, char **comment);

#endif /* _LINE_CONF_H */
