/** 
 *   Parsed tools por configuration whitch bracket.
 *
 *   Included in libiarcontrol from v 0.1.5
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

#include <string.h>
#include <stdlib.h>
#include "my_stdio.h"
#include "parse_tools.h"


#ifdef CONFIG_BRACKET_SUPPORT


static int bracket_delete_and_find_equal(char *line, int *length, int *eq)
{
	int bra_count = 0;
	int i,f = 0;
	char *t;
	
	*eq = 0;
	for( t = line,i=0; i < *length; t++, i++){
		if (*t == '{') {
			if(bra_count == 0) {
				*t = 0;
				line = t;
				f = i;
			}
			bra_count++;
		} else if (*t == '}') {
			bra_count--;
			if(bra_count == 0) {
				*t = 0;
				*length = i - f;
				break;
			}
		} else if (*t == '=' && bra_count == 1) {
			*eq = 1;
		}
	}

	return bra_count ;

}


static int expand_conf_braket_array_line(struct configuration *conf,
					    char *etag, char *edata)
{
	
	struct configuration *elmt = NULL;
	struct configuration *new;
	int len = strlen(edata);
	char tag[strlen(etag)+len+1];
	char *t, *h;
	char *ldata,*ltag,*lcomment;
	int n, l;
	int bra_count;
	int n_elemnts = 0;
	char ns[8];
	int done = 0;
	int ret = 0;
	int eq;

	h = edata;
	/* Find if bracket match, delete this and find '=': */
	if ( bracket_delete_and_find_equal(h, &len, &eq) != 0 ) {
		return -1;
	}
	h++;
	sprintf(tag,"%s.n",etag);
	elmt = find_conf_tag(conf, tag);
	if(elmt == NULL) {
		elmt = add_conf_tag(conf,tag,"1",NULL);
		elmt->flag |= FCF_ARR_PARSED;
		n = 1;
	} else {
		n = atoi(elmt->data) + 1;
		sprintf(ns,"%d", n);
		change_conf_tag(elmt,tag,ns);
	}

	new = elmt;
	while (!done && !ret) {
		t = h;
		l = 0;
		if (eq) {
			bra_count = 0;
			while ((*t != CONFIG_ELMT_SEP || bra_count > 0) && 
			       l < len){ 
				if ( *t == '{')  bra_count++;
				else if ( *t == '}')  bra_count--;
				t++;
				l++;
			}
		} else {
			while (*t != CONFIG_VEC_SEP && l < len){ 
				t++;
				l++;
			}
		}
		if(l == len) { 
			done = 1;
		}else {
			*t = 0;
		}

		if ( eq  && parse_line(h, &ltag, &ldata, &lcomment) == 0) {
			if ( *ldata != '{' ) {
				sprintf(tag, "%s.%d.%s",etag, n - 1, ltag);
				new = add_mod_conf_tag(new, tag, ldata, NULL);
				new->flag |= FCF_ARR_PARSED;
			} else {
				sprintf(tag, "%s.%d.%s",etag, n - 1, ltag);
				ret = expand_conf_braket_array_line(elmt, tag, 
								    ldata);
			}
		} else if (! eq) {
			sprintf(tag, "%s.%d",etag, n_elemnts);
			new = add_mod_conf_tag(elmt, tag, h, NULL);
			new->flag |= FCF_ARR_PARSED;
			sprintf(tag, "%s.n",etag);
			sprintf(ns,"%d", n_elemnts + 1);
			change_conf_tag(elmt,tag,ns);
		}
		n_elemnts++;
		h = t + 1;
		len -=l;
	}

	return ret;
}
	

static int expand_conf_braket_array_element(struct configuration *conf,
					    struct configuration *elmt)
{
	int ret;
	int len = strlen(elmt->data);
	char *line;

	line = (char *) malloc(len + 1);
	strncpy(line, elmt->data, len);
	ret = expand_conf_braket_array_line(conf, elmt->tag, line);
	free(line);

	return ret;
}



int expand_conf_braket_array(struct configuration *conf)
{
	struct configuration *p = conf;
	int ret = 0;

	while(p->next != NULL && !ret) {
		if ((p->flag & FCF_ARRAY) && !(p->flag & FCF_DELETED) ) {
			ret = expand_conf_braket_array_element(conf, p);
		}
		p = p->next;
	}

	return ret;
}

#else 

int expand_conf_braket_array(struct configuration *conf)
{
	
	/*PVERB(PERR,*/
	fprintf(stderr,
	      "expand_conf_braket_array: Not included in this version.\n");

	return -1;
}

#endif
