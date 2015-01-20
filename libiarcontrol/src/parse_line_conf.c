/** 
 *   Camera Control for STAR I (originally).
 *
 *   Included in libiarcontrol v 0.1.5
 *
 *   (c) Copyright 2005 Federico Bareilles <fede@iar.unlp.edu.ar>,
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

#include <string.h>
#include <stdlib.h>
#include "my_stdio.h"


int parse_line(char *line, char **tag, char **data, char **comment)
{
	int i, j;
	int len;

	len = strlen(line);
	for( i=0; (i < len) && (line[i] != '=');i++);
	/**
	  Lines starting which '#' and '*' considering comments.  An '#'
	  in any position is considering comments, and ignoring the
	  rest of line.
	**/
	if (i < len && *line != '#' && line != 0 && *line != '*') {
		line[i] = 0;
		*tag = stripwhite(line);
		i++;
		for( j=i; (j < len) && (line[j] != '#');j++);		
		if( j <len){
			line[j] = 0;
			j++;
			*comment = stripwhite(line+j);
		}else{
			*comment = NULL;
		}
		*data = stripwhite(line+i);

	}else{
		*tag = NULL;
		*data = NULL;
		if (*line == '#' || *line == '*' ) {
			*comment = line+1;
		} else {
			*comment = NULL;
		}
			
		return -1;
	}

	return 0;

}

