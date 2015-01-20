/** 
 *
 *   Common math function implementation.
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
 *   TODO: ??? 
 **/

#define _GNU_SOURCE

#include <math.h>


double my_round(double x)
{
	double f = floor(x);

	return ((x-f) < 0.5)? f : f + 1.0;
}


double grid_norm(double x, int n)
{
        return my_round(x/(double)n) * n;
}


/* End */
