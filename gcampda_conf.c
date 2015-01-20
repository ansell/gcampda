/** 
 *   gcampda_conf: Confiuration routines. 
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
#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <my_stdio.h>
#include <parse_tools.h>
#include <locks.h>

#include "gcampda.h"
#include "pakbus.h"


static GC_CONF *gcampda_conf_make(void)
{
        GC_CONF *gc= NULL;

        gc = (GC_CONF *) malloc(sizeof(GC_CONF));
	GC_STREAM_OUT(gc) = NULL;
	GC_STATION_NAME(gc) = NULL;
	GC_TSTAMP_FMT(gc) = NULL;
	GC_LOGGER_PORT(gc) = -1;
	GC_LOGGER_READY(gc) = 0;
	GC_LOGGER_PAUSE(gc) = 0;
	GC_HOP_METRIC(gc) = 0x02; /* A code used to indicate the worst
				     case interval for the speed of
				     the link required to complete a
				     transaction (default value of
				     0x02):

				     0x00: 200 msec or less
				     0x01: 1 sec or less
				     0x02: 5 sec or less (default for RS232 or
				     TCP/IP)
				     0x03: 10 sec or less
				     0x04: 20 sec or less
				     0x05: 1 min or less
				     0x06: 5 min or less
				     0x07: 30 min or less
				  */
        return gc;
}


GC_CONF *gcampda_conf_parse(char *filename)
{
	GC_CONF *gc = NULL;
	struct configuration *conf = NULL;
	struct configuration *tag = NULL;
	

	if (read_conf_file(&conf, filename) != 0 ) {
		return gc;
	}
	
	gc = gcampda_conf_make();
	GC_FD(gc) = -1;
	srand(getpid());
	GC_TRAN_NBR(gc) = rand() & 0xff; /* :) */

	if ((tag = find_conf_tag(conf, "port")) != NULL) {
		GC_LOGGER_PORT(gc) = atoi(tag->data);
		if ((tag = find_conf_tag(conf, "host")) != NULL) {
			snprintf(GC_LOGGER_HOST(gc), 
				 _POSIX_PATH_MAX, "%s", tag->data);
		} else {
			GC_LOGGER_PORT(gc) = -1;
		}
	} 
	if (GC_IF_SERIAL(gc) ) {
		if ((tag = find_conf_tag(conf, "device")) != NULL) {
			snprintf(GC_DEV_NAME(gc), 
				 _POSIX_PATH_MAX, "%s", tag->data);
		} else {
			sprintf(GC_DEV_NAME(gc),"/dev/ttyS0");
		}
	}
	if ((tag = find_conf_tag(conf, "time_zone")) != NULL) {
		GC_TZ(gc) = atoi(tag->data);
	} else {
		GC_TZ(gc) = 0;
	}
	if ((tag = find_conf_tag(conf, "security_code")) != NULL) {
		GC_SCODE(gc) = strtol(tag->data, NULL, 0);
	} else {
		GC_SCODE(gc) = 0x0000;
	}
	if ((tag = find_conf_tag(conf, "logger_addr")) != NULL) {
		GC_LOGGER_ADDR(gc) = strtol(tag->data, NULL, 0);;
	} else {
		GC_LOGGER_ADDR(gc) = 0x001;
	}
	if ((tag = find_conf_tag(conf, "soft_addr")) != NULL) {
		GC_SOFT_ADDR(gc) = strtol(tag->data, NULL, 0);
	} else {
		GC_SOFT_ADDR(gc) = 0xffc;
	}
	if ((tag = find_conf_tag(conf, "data")) != NULL) {
		snprintf(GC_DATA(gc), _POSIX_PATH_MAX,
			 "%s", tag->data);
	} else {
		sprintf(GC_DATA(gc),".");
	}
	if ((tag = find_conf_tag(conf, "time_fmt")) != NULL) {
		asprintf(&GC_TSTAMP_FMT(gc), "%s", tag->data); 
	} else {
		GC_TSTAMP_FMT(gc) = NULL;
	}
	/*
	if ((tag = find_conf_tag(conf, "user_id")) != NULL) {
		GC_UID(gc) = atoi(tag->data);
	} else {
		GC_UID(gc) = 0;
	}
	if ((tag = find_conf_tag(conf, "group_id")) != NULL) { 
		GC_GID(gc) = atoi(tag->data);
	} else {
		GC_GID(gc) = 0;
	}
	*/
	GC_UID(gc) = GC_GID(gc) = 0; /* not implement yet */
	if ((tag = find_conf_tag(conf, "chk_last_record")) != NULL) {
		GC_CHK_LAST_RECORD(gc) = atoi(tag->data);
	} else {
		GC_CHK_LAST_RECORD(gc) = 1;
	}
	free_conf(conf);

        return gc;
}


int gcampda_conf_free(GC_CONF *gc)
{
	
	if (GC_STREAM_OUT(gc) != NULL)
		fclose(GC_STREAM_OUT(gc));
	if (GC_STATION_NAME(gc) != NULL)
		free(GC_STATION_NAME(gc));
	if (GC_TSTAMP_FMT(gc) != NULL)
		free(GC_TSTAMP_FMT(gc));

        free(gc);
	
        return 0;
}



DL_CONF *gcampda_dl_conf_make(void)
{
	DL_CONF *d = (DL_CONF *) malloc(sizeof(DL_CONF));

	DL_TBL_NUMBER(d) = -1;
	DL_TBL_SIGNATURE(d) = -1;
	DL_TBL_NAME(d) = NULL;
	DL_TBL_FLAG(d) = 0;
	DL_TBL_LAST_RECORD(d) = 0;
	DL_TBL_NEXT(d) = NULL;

	return d;
}


static struct configuration *_find_dl_tag(struct configuration *conf, 
					  DL_CONF *d, char *name)
{
	char tag_name[80];
	struct configuration *tag;

	snprintf(tag_name, 80, "table.%d.%s", DL_TBL_INDEX(d), name);
	if ((tag = find_conf_tag(conf,tag_name)) == NULL) {
		SYSLOG(LOG_CRIT, "Record %s, not found.", tag_name);
	}
	return tag;
}


static int _dl_conf_parse(struct configuration *conf, DL_CONF *d)
{
	struct configuration *tag = NULL;

	if ((tag = _find_dl_tag(conf, d, "name")) != NULL) {
		asprintf(&DL_TBL_NAME(d), "%s", tag->data);
	} else {
		return -1;
	}
	if ((tag = _find_dl_tag(conf, d, "number")) != NULL) {
		DL_TBL_NUMBER(d) = atoi(tag->data);
	} else {
		return -1;
	}
	if ((tag = _find_dl_tag(conf, d, "signature")) != NULL) {
		DL_TBL_SIGNATURE(d) = strtol(tag->data, NULL, 0);
	} else {
		return -1;
	}
	if ((tag = _find_dl_tag(conf, d, "download")) != NULL) {
		DL_TBL_FLAG(d) = atoi(tag->data);
	} else {
		return -1;
	}
	if ((tag = _find_dl_tag(conf, d, "next_record")) != NULL) {
		DL_TBL_LAST_RECORD(d) = atoi(tag->data);
	} else {
		return -1;
	}

	return 0;
}


DL_CONF *gcampda_dl_conf_parse(struct configuration *conf)
{
	DL_CONF *d = NULL;
	DL_CONF *dp = NULL;
	struct configuration *tag = NULL;
	int num_tables = -1;
	int i;


	if ((tag = find_conf_tag(conf, "table.n")) != NULL) {
		d = gcampda_dl_conf_make();
		dp = d;
		num_tables = atoi(tag->data);
		for(i = 0; i < num_tables; i++) {
			DL_TBL_INDEX(dp) = i;
			if (_dl_conf_parse(conf, dp) != 0) {
				DL_TBL_FLAG(dp) = 0;
			}
			if ( i < num_tables -1) { 
				DL_TBL_NEXT(dp) = gcampda_dl_conf_make();
				dp = DL_TBL_NEXT(dp);
			}
		}
	} else {
		SYSLOG(LOG_CRIT, "No \"table\" definition in configuration file\n");
	}

	return d;
}



int gcampda_dl_conf_write(GC_CONF *gc, DL_CONF *d)
{
	char *filename = NULL;
	FILE *fp;
	int lock;

	asprintf(&filename,"%s/%s.conf", GC_DATA(gc), GC_STATION_NAME(gc));
	if (access(GC_DATA(gc),  F_OK) != 0)
		mkdir(GC_DATA(gc), 0755);

	if ( (lock = if_lock_file(filename)) ) {
                SYSLOG(LOG_ERR,"File %s locked by pid: %d.", filename, lock);
                return -1;
        }

	fp = fopen(filename, "w");
        if( fp == NULL ){
                SYSLOG(LOG_ERR, "Error opening file: %s", filename);
                return -1;
        }
        while (d != NULL) {
		fprintf(fp, "table = {\n\tname = %s,\n\tsignature = 0x%04x,\n\tnumber = %d,\n\tdownload = %d,\n\tnext_record = %d\n}\n\n",
			DL_TBL_NAME(d), DL_TBL_SIGNATURE(d), DL_TBL_NUMBER(d),
			DL_TBL_FLAG(d),	DL_TBL_LAST_RECORD(d));
		d = DL_TBL_NEXT(d);
	}
	fclose(fp);
        unlock_file(filename);

	free(filename);
	return 0;
}


int gcampda_dl_conf_free(DL_CONF *d)
{
	DL_CONF *dp;

	while(d != NULL){
		dp = d;
		d = DL_TBL_NEXT(d);
		free(DL_TBL_NAME(dp));
		free(dp);
	}

	return 0;
}


int gcampda_tdf2dl_conf(CR_TDF *tdf, DL_CONF **dlc)
{
	DL_CONF *dp = NULL;



	while(tdf != NULL) {
		if ( strncmp(CR_TDF_NAME(tdf),"Status", 6) != 0 &&
		     strncmp(CR_TDF_NAME(tdf),"Public", 6) != 0   ){
			
		}
		if ( *dlc == NULL) 
			dp = *dlc = gcampda_dl_conf_make();
		else {
			DL_TBL_NEXT(dp) = gcampda_dl_conf_make();
			dp = DL_TBL_NEXT(dp);
		}
		cr_tdf2dl_conf(tdf, dp);
		if ( strncmp(CR_TDF_NAME(tdf),"Status", 6) != 0 &&
		     strncmp(CR_TDF_NAME(tdf),"Public", 6) != 0   ){
			DL_TBL_FLAG(dp) = 1;
		}
		tdf = CR_TDF_NEXT(tdf); 
	}

	return 0;
}
