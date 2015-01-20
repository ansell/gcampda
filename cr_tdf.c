/** 
 *   cr_tdf: 
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


static int32_t char_read_int32(unsigned char *buff)
{
	return CHAR2INT32(buff);
}


static CR_TDF_FIELD *cr_tdf_field_make(void)
{
	CR_TDF_FIELD *f = (CR_TDF_FIELD *) malloc(sizeof(CR_TDF_FIELD));

	CR_TDF_FIELD_NUMBER(f) = 1;
	CR_TDF_FIELD_RO(f) = 0;
	CR_TDF_FIELD_TYPE(f) = 0;
	CR_TDF_FIELD_NAME(f) = NULL;
	CR_TDF_FIELD_ALIAS(f) = NULL;
	CR_TDF_FIELD_PROCESING(f) = NULL;
	CR_TDF_FIELD_UNITS(f) = NULL;
	CR_TDF_FIELD_DESCRIP(f) = NULL;
	CR_TDF_FIELD_BEGIN_INDEX(f) = 0;
	CR_TDF_FIELD_DIM_ARRAY(f) = NULL;
	CR_TDF_FIELD_DIM_NUM(f) = 0;
	CR_TDF_FIELD_NEXT(f) = NULL;

	return f;
}


static CR_TDF *cr_tdf_make(void)
{
	CR_TDF *t = (CR_TDF *) malloc(sizeof(CR_TDF));
	
	CR_TDF_NUMBER(t) = 1;
	CR_TDF_SIGNATURE(t) = -1;
	CR_TDF_NAME(t) = NULL;
	CR_TDF_NR(t) = 0;
	CR_TDF_TIME_TYPE(t) = 0;
	CR_TDF_TIME_INTO(t) = 0;
	CR_TDF_INTERVAL(t) = 0;
	CR_TDF_FIELD_LIST(t) = NULL;
	CR_TDF_LAST_RECORD_READ(t) = 0;
	CR_TDF_NEXT(t) = NULL;

	return t;
}


int cr_tdf_field_store(CR_TDF_FIELD **f, void *buff, size_t buff_size)
{
	CR_TDF_FIELD *fp;
	int len = 0;
	int l;
	int field_number = 0;

	if (*f == NULL) *f = cr_tdf_field_make();
	fp = *f;
	while( CR_TDF_FIELD_NEXT(fp) != NULL) {
		field_number = CR_TDF_FIELD_NUMBER(fp); 
		fp = CR_TDF_FIELD_NEXT(fp);
	}
	CR_TDF_FIELD_NUMBER(fp) = field_number + 1;
	
	CR_TDF_FIELD_RO(fp) = (*((unsigned char *)buff+len) & 0x80) >> 7;
	CR_TDF_FIELD_TYPE(fp) = *((unsigned char *)buff+len) & 0x7f;
	len += 1;
	len += asprintf(&CR_TDF_FIELD_NAME(fp),"%s", (char *) (buff+len));
	len++;
	do {
		l = asprintf(&CR_TDF_FIELD_ALIAS(fp),
				    "%s", (char *) (buff+len));
		len++;
		if (l > 0) {
			len += l;
			SYSLOG(LOG_NOTICE, 
			       "Alias ignored: %s", CR_TDF_FIELD_ALIAS(fp));
			free(CR_TDF_FIELD_ALIAS(fp));
			CR_TDF_FIELD_ALIAS(fp) = NULL;
		}
	} while (l > 0);
	len += asprintf(&CR_TDF_FIELD_PROCESING(fp),"%s", (char *) (buff+len));
	len++;
	len += asprintf(&CR_TDF_FIELD_UNITS(fp),"%s", (char *) (buff+len));
	len++;
	len += asprintf(&CR_TDF_FIELD_DESCRIP(fp),"%s", (char *) (buff+len));
	len++;
	CR_TDF_FIELD_BEGIN_INDEX(fp) = char_read_int32(buff+len);
	len += 4;

	l = 0;
	while(char_read_int32(buff+len+(l<<2)) != 0) l++;
	CR_TDF_FIELD_DIM_NUM(fp) = l;
	CR_TDF_FIELD_DIM_ARRAY(fp) = (uint32_t *) 
		malloc(sizeof(uint32_t) * CR_TDF_FIELD_DIM_NUM(fp));
	for (l = 0; l< CR_TDF_FIELD_DIM_NUM(fp);l++) {
		*(CR_TDF_FIELD_DIM_ARRAY(fp)+l) = char_read_int32(buff+len);
		len += 4;
	}
	len += 4;

	if (*((unsigned char *)buff+len) != 0) {
		CR_TDF_FIELD_NEXT(fp) = cr_tdf_field_make();
		len += cr_tdf_field_store(&fp, buff+len, buff_size - len);
	}

	return len;
}


int cr_tdf_store(CR_TDF **t, void *buff, size_t buff_size)
{
	CR_TDF *tp;
	int len = 0;
	int tbl_number = 0;

	if (*t == NULL) *t = cr_tdf_make();
	tp = *t;
	while( CR_TDF_NEXT(tp) != NULL) {
		tbl_number = CR_TDF_NUMBER(tp); 
		tp = CR_TDF_NEXT(tp);
	}
	CR_TDF_NUMBER(tp) = tbl_number + 1;
	len += asprintf(&CR_TDF_NAME(tp),"%s", (char *) (buff+len));
	len++;
	CR_TDF_NR(tp) = char_read_int32(buff+len);
	len += 4;
	CR_TDF_TIME_TYPE(tp) = *((unsigned char *)buff+len);
	len++;
	CR_TDF_TIME_INTO(tp) = char_read_int32(buff+len);
	len += 4;
	if(char_read_int32(buff+len) != 0) {
		SYSLOG(LOG_NOTICE, "CR_TDF_TIME_INTO nanosec ignored: %d",
		      char_read_int32(buff+len));
	}
	len += 4;
	CR_TDF_INTERVAL(tp) = char_read_int32(buff+len);
	len += 4;
	if(char_read_int32(buff+len) != 0) {
		SYSLOG(LOG_NOTICE, "CR_TDF_INTERVAL nanosec ignored: %d",
		      char_read_int32(buff+len));
	}
	len += 4;
	len += cr_tdf_field_store(&CR_TDF_FIELD_LIST(tp), 
				  buff+len, buff_size - len);
	len++;
	CR_TDF_SIGNATURE(tp) = calc_sig_for(buff , len, 0xaaaa);

	if (len < buff_size) {
		CR_TDF_NEXT(tp) = cr_tdf_make();
		len += cr_tdf_store(&tp, (void *) ((char *) buff+len),
				    buff_size - len);
	}

	return len;
}



void cr_tdf_field_free(CR_TDF_FIELD **cr_tdf_field)
{
	CR_TDF_FIELD *f = *cr_tdf_field;
	CR_TDF_FIELD *fp; 

	while(f != NULL){
                fp = f;
                f = CR_TDF_FIELD_NEXT(f);
		if( CR_TDF_FIELD_NAME(fp) != NULL) free(CR_TDF_FIELD_NAME(fp));
		if( CR_TDF_FIELD_ALIAS(fp) != NULL) free(CR_TDF_FIELD_ALIAS(fp));
		if( CR_TDF_FIELD_PROCESING(fp) != NULL) 
			free(CR_TDF_FIELD_PROCESING(fp));
		if( CR_TDF_FIELD_UNITS(fp) != NULL) free(CR_TDF_FIELD_UNITS(fp));
		if( CR_TDF_FIELD_DESCRIP(fp) != NULL) 
			free(CR_TDF_FIELD_DESCRIP(fp));
		if( CR_TDF_FIELD_DIM_ARRAY(fp) != NULL) 
			free(CR_TDF_FIELD_DIM_ARRAY(fp));
		free(fp);
        }
	*cr_tdf_field = NULL;
	
	return;
}


void cr_tdf_free(CR_TDF **cr_tdf)
{
	CR_TDF *t = *cr_tdf;
	CR_TDF *tp; 

	while(t != NULL){
                tp = t;
                t = CR_TDF_NEXT(t);
		if( CR_TDF_NAME(tp) != NULL) free(CR_TDF_NAME(tp));
		if( CR_TDF_FIELD_LIST(tp) != NULL) 
			cr_tdf_field_free(&CR_TDF_FIELD_LIST(tp));
		free(tp);
        }
	*cr_tdf = NULL;

	return;
}


void cr_tdf_field_list(CR_TDF_FIELD *f)
{
	int i;

	while(f != NULL){
		printf("\t%d %s ", 
		       CR_TDF_FIELD_NUMBER(f),
		       CR_TDF_FIELD_NAME(f));
		if( *CR_TDF_FIELD_UNITS(f) != 0)
			printf("[%s] ", CR_TDF_FIELD_UNITS(f));
		if ( CR_TDF_FIELD_DIM_ARRAY(f)[0] == 1) {
			printf("not array");
		} else {
			printf("array ");
			for(i=0;i<CR_TDF_FIELD_DIM_NUM(f);i++)
				printf("[%d]", CR_TDF_FIELD_DIM_ARRAY(f)[i]);
		}
		printf(", type: %2d", CR_TDF_FIELD_TYPE(f));
		printf("\n");
		f = CR_TDF_FIELD_NEXT(f);
	}

	return;
}


void cr_tdf_list(CR_TDF *t, int field_flag)
{
	
	while(t != NULL){
		printf("Table %d %-8s %6d (0x%04x), time into: %d, interval: %d\n", 
		       CR_TDF_NUMBER(t), CR_TDF_NAME(t), 
		       CR_TDF_NR(t), CR_TDF_SIGNATURE(t),
		       CR_TDF_TIME_INTO(t), CR_TDF_INTERVAL(t));
		if(field_flag) 
			cr_tdf_field_list(CR_TDF_FIELD_LIST(t));
		t = CR_TDF_NEXT(t);
	}

	return;
}


CR_TDF *cr_tdf_find_table(CR_TDF *t, char *table_name)
{
	CR_TDF *find = NULL;

	while(t != NULL && find == NULL){
		if (strcmp(table_name, CR_TDF_NAME(t)) == 0)
			find = t;
		t = CR_TDF_NEXT(t);
	}

	return find;
}


void cr_tdf2dl_conf(CR_TDF *t, DL_CONF *d)
{
	DL_TBL_NUMBER(d) = CR_TDF_NUMBER(t);
	DL_TBL_SIGNATURE(d) = CR_TDF_SIGNATURE(t);
	asprintf(&DL_TBL_NAME(d), "%s", CR_TDF_NAME(t));
	DL_TBL_FLAG(d) = 0;
	DL_TBL_LAST_RECORD(d) = 0;

	return;
}
