/** 
 *   gcampda:
 *
 *
 *   (c) Copyright 2011 Federico Bareilles <bareilles@gmail.com>,
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

#ifndef _GCAMPDA_H
#define _GCAMPDA_H
#include <stdint.h> /* for uintXX_t definition */

#include <limits.h>
#include <my_stdio.h>
#include <parse_tools.h>

#define DEFAULT_CONF_FILE "/etc/gcampda.conf" /* "/etc/gcampda.conf" */


typedef struct _gc_conf {
        char device[_POSIX_PATH_MAX];
	int port;
        char data_dir[_POSIX_PATH_MAX];
	int time_zone;
	uint16_t logger_addr; /* PakBusAddress of logger */
	uint16_t soft_addr;
	uint16_t security_code;
        int user_id; /* not implement yet */
        int group_id; /* not implement yet */
	int fd;
	int logger_ready;
	int logger_pause;
	int chk_last_record_flag;
	unsigned char tran_nbr;
	FILE *stream_out;
	char *station_name;
	char *tstamp_fmt;
	unsigned char hop_metric;
}GC_CONF;

#define GC_FD(c) (c)->fd
#define GC_TZ(c) (c)->time_zone
#define GC_LOGGER_ADDR(c) (c)->logger_addr
#define GC_SOFT_ADDR(c) (c)->soft_addr
#define GC_SCODE(c) (c)->security_code
#define GC_DATA(c) (c)->data_dir
#define GC_DEV_NAME(c) (c)->device
#define GC_LOGGER_HOST(c) (c)->device
#define GC_LOGGER_PORT(c) (c)->port
#define GC_IF_SERIAL(c) ((c)->port <= 0 ?1:0)
#define GC_LOGGER_READY(c) (c)->logger_ready
#define GC_LOGGER_PAUSE(c) (c)->logger_pause
#define GC_CHK_LAST_RECORD(c) (c)->chk_last_record_flag
#define GC_UID(c) (c)->user_id
#define GC_GID(c) (c)->group_id
#define GC_TRAN_NBR(c) (c)->tran_nbr
#define GC_TRAN_NBR_NEW(c) (++GC_TRAN_NBR(c) == 0?		\
			    ++GC_TRAN_NBR(c):GC_TRAN_NBR(c))
#define GC_STREAM_OUT(c) (c)->stream_out
#define GC_GET_STREAM_OUT(c) \
	( GC_STREAM_OUT(c) == NULL ?stdout:GC_STREAM_OUT(c)); 
#define GC_STATION_NAME(c) (c)->station_name
#define GC_TSTAMP_FMT(c) (c)->tstamp_fmt
#define GC_HOP_METRIC(c) (c)->hop_metric

typedef struct _down_load_conf {
	int table_number;
	int table_def_signature; /* the storge value is uint16_t */
	char *table_name;
	int dload_flag;
	uint32_t last_record_read;
	struct _down_load_conf *next;
	int index;
} DL_CONF;

#define DL_TBL_NUMBER(d) (d)->table_number
#define DL_TBL_SIGNATURE(d) (d)->table_def_signature
#define DL_TBL_NAME(d) (d)->table_name
#define DL_TBL_FLAG(d) (d)->dload_flag
#define DL_TBL_LAST_RECORD(d) (d)->last_record_read
#define DL_TBL_NEXT(d) (d)->next
#define DL_TBL_INDEX(d) (d)->index

GC_CONF *gcampda_conf_parse(char *filename);
int gcampda_conf_free(GC_CONF *gc);
int gcampda_open(GC_CONF *gc);
int gcampda_init_connection(GC_CONF *gc);
int gcampda_finished_connection(GC_CONF *gc);
void gcampda_close(GC_CONF *gc);
char *gcampda_get_station_name(GC_CONF *gc);
DL_CONF *gcampda_dl_conf_tables_read(GC_CONF *gc);
int gcampda_dl_conf_update_last_record(GC_CONF *gc, DL_CONF *d);

DL_CONF *gcampda_dl_conf_parse(struct configuration *conf);
int gcampda_dl_conf_write(GC_CONF *gc, DL_CONF *d);
int gcampda_dl_conf_free(DL_CONF *gc);
int gcampda_data_download(GC_CONF *gc);
int gcampda_data_lock(GC_CONF *gc);
int gcampda_data_unlock(GC_CONF *gc);


#endif  /* ifndef _GCAMPDA_H */
