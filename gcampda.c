/** 
 *   gcampda: 
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
#include <unistd.h>
#include <my_stdio.h>
#include <socket_client.h>
#include <time.h>
#include <string.h>

#include "rs232.h"
#include "my_io.h"
#include "locks.h"
#include "gcampda.h"
#include "pakbus.h"



int gcampda_open(GC_CONF *gc)
{
	if ( GC_IF_SERIAL(gc) ) {
		hw_set_timeout(2);
		GC_FD(gc) = rs232_open(GC_DEV_NAME(gc));	
	} else {
		hw_set_timeout(30);// 30
		GC_FD(gc) = call_socket(GC_LOGGER_HOST(gc), GC_LOGGER_PORT(gc));
	}
	
	return GC_FD(gc);
}


void gcampda_close(GC_CONF *gc)
{
	if ( GC_IF_SERIAL(gc) ) {
		hw_close(GC_FD(gc));
	} else {
		close_socket(GC_FD(gc));
	}
}



static PAKBUS *gcampdapak_spect(GC_CONF *gc, 
				unsigned char type, unsigned char nbr)
{
	unsigned char type_i, 
		nbr_i;
	PAKBUS *pk = NULL;
	int ret;
	int done = 0;
 
	do {
		if ( pk != NULL) {
			pak_free(pk);
			pk = NULL;
		}
		ret = pak_rcv(GC_FD(gc), GC_SOFT_ADDR(gc), &pk);
		if (pk == NULL ) {
			return NULL;
		}
		GC_LOGGER_READY(gc) = 
			(PAKBUS_GET_LINK_STATE(pk) & LS_READY) == LS_READY?1:0;
		GC_LOGGER_PAUSE(gc) = 
			(PAKBUS_GET_LINK_STATE(pk) & LS_PAUSE) == LS_PAUSE?1:0;
		if (GC_LOGGER_PAUSE(gc)) {
			SYSLOG(LOG_INFO, "Pause request from logger, ready: %d",
			      GC_LOGGER_READY(gc));
		}
		nbr_i = 0;
		type_i = pak_read_byte(pk);
		if ( type_i == type) {
			nbr_i = pak_read_byte(pk);
			if (nbr_i == nbr)
				done = 1;
		}
		/* if !done -> tengo un paquete que no se para que es... FIXME. */
#ifdef DEBUG_MODE
		if ( !done) {
			PDBG("Unknown arrived packet, no action for this.\n");
		}
#endif			
	} while (!done);

	return pk;
}


static int gcampda_hello_request(GC_CONF *gc)
{
	PAKBUS *pki = NULL;
	int ret = cr_hello_transaction_request(gc);
	
	if ( ret == 0 ) {
		pki = gcampdapak_spect(gc, 0x89, GC_TRAN_NBR(gc));
		
		if (pki != NULL)
		pak_free(pki);
		else
			ret = -1;
	}

	return ret;
}


int gcampda_init_connection(GC_CONF *gc)
{
	PAKBUS *pki = NULL;
	int ret = -1;
	int retry = 0;

	do {
		cr_send_0xbd(gc, 6);
		pakctrl(gc, LS_OFF_LINE, 0xfff, EMC_NEUTRAL, 0);
		/* wait hello request */
		if (pki != NULL)
			pak_free(pki);
		pki = gcampdapak_spect(gc, 0x0e, 0);
	} while (pki == NULL && retry++ < 3); 
	/* Ring: */
	if (pki != NULL  ) {
		pak_free(pki);
		cr_send_0xbd(gc, 1);
		cr_wake_up(gc);
		if (cr_at(gc) != 0) {
			SYSLOG(LOG_ERR, 
			      "Unable to communicate with the data logger");
			ret = -2;
		} else {
			ret = gcampda_hello_request(gc);
		}
	}

	return ret >= 0 ? 0: ret;
}


int gcampda_finished_connection(GC_CONF *gc)
{
	PAKBUS *pki = NULL;
	int ret;
	int done = 0;

	ret = pakctrl(gc, LS_FINISHED, GC_LOGGER_ADDR(gc), EMC_LAST, 3);
	if ( ret == 0 ) {
		do {
			ret = pak_rcv(GC_FD(gc), GC_SOFT_ADDR(gc), &pki);
			if (pki != NULL) {
				if (PAKBUS_GET_LINK_STATE(pki) == LS_OFF_LINE)
					done = 1;
				pak_free(pki);
				pki = NULL;
			}
		} while (!done); 
	}

	return ret;
}


char *gcampda_get_station_name(GC_CONF *gc)
{
	return (GC_STATION_NAME(gc) = cr_get_station_name(gc));
}


DL_CONF *gcampda_dl_conf_tables_read(GC_CONF *gc)
{
	char *conf_name;
	DL_CONF *d = NULL;
	struct configuration *conf = NULL;

	if (GC_STATION_NAME(gc) == NULL)
		gcampda_get_station_name(gc);

	if (GC_STATION_NAME(gc) == NULL) {
		SYSLOG(LOG_ERR,"Can't get station name.\n");
		return NULL;
	}

	asprintf(&conf_name,"%s/%s.conf", GC_DATA(gc), GC_STATION_NAME(gc));
	
	if (read_conf_file(&conf, conf_name) != 0 ) {
		return d;
	}

	if( expand_conf_braket_array(conf) != 0 ) {
		SYSLOG(LOG_ERR,"%s: bracket not match.\n", conf_name);
	}

	d = gcampda_dl_conf_parse(conf);

	free_conf(conf);
	free(conf_name);

	return d;

}


static int gcampda_check_last_record(GC_CONF *gc, DL_CONF *d, CR_TDF *tdf)
{
	long int bflen = 512;
	char *line = (char *) malloc(sizeof(char) * bflen);
	size_t len = bflen;
	int ret = -1;
	int reg_len;
	int tstamp_len;
	uint32_t record;
	
	fseek(GC_STREAM_OUT(gc), 0L, SEEK_SET);
	do {
		ret = getline(&line, &len, GC_STREAM_OUT(gc));
	} while (ret > 0 && *(line) == '#');
	if ( ret > 0 ) {
		reg_len = ret;
		fseek(GC_STREAM_OUT(gc), - (((long int) reg_len) * 2), SEEK_END);
		ret = getline(&line, &len, GC_STREAM_OUT(gc)); /* dummy read */
		ret = getline(&line, &len, GC_STREAM_OUT(gc));
		if ( reg_len > ret ) {
			SYSLOG(LOG_CRIT, 
		       "Last register in %s/%s-%s-%04x.dat is bad, fix it.",
			       GC_DATA(gc), GC_STATION_NAME(gc), DL_TBL_NAME(d),
			       DL_TBL_SIGNATURE(d));
			ret = -2;
		}
	} else {
		ret = 0; /* Only header exist */
	}
	if ( ret > 0 ) {
		time_t t;
		struct tm *tmp;
		char str_tstamp[80];
		if (GC_TSTAMP_FMT(gc) != NULL) {
			t = time(NULL); /* dummy time calculation */
			tmp = localtime(&t);
			tstamp_len = strftime(str_tstamp, 
					      80 /*sizeof(str_tstamp)*/,
					      GC_TSTAMP_FMT(gc), tmp);
		} else { /* Unix time: */
			tstamp_len = 10;
		}
		//tstamp_len++;
		/* Read last register number: */
		sscanf(line+tstamp_len,"%u", &record);
		record++;
		if ( record != DL_TBL_LAST_RECORD(d) ) {
			SYSLOG(LOG_WARNING, "Last record not match (%d != %d)",
			       DL_TBL_LAST_RECORD(d), record);
			DL_TBL_LAST_RECORD(d) = record;	
		} 
		ret = 0; /* FIXME = 0 */
	}

	free(line);

	return ret;
}


static int gcampda_close_data_file(GC_CONF *gc)
{
	int ret = fclose(GC_STREAM_OUT(gc));

	GC_STREAM_OUT(gc) = NULL;

	return ret;
}


static int gcampda_open_data_file(GC_CONF *gc, DL_CONF *d, CR_TDF *tdf)
{
	int header_flag = 0;
	char *datafile;
	int ret;

	asprintf(&datafile,"%s/%s-%s-%04x.dat", GC_DATA(gc), 
		 GC_STATION_NAME(gc), DL_TBL_NAME(d), DL_TBL_SIGNATURE(d));
	if ( access(datafile, F_OK) != 0) header_flag = 1;
	GC_STREAM_OUT(gc) = fopen(datafile, "a+");
	if ( GC_STREAM_OUT(gc) == NULL ){
                SYSLOG(LOG_ERR, "Error opening file: %s", datafile);
		free(datafile);
                return -1;
        }
	if ( !header_flag && GC_CHK_LAST_RECORD(gc) ) { /* check integrity: */
		if ( (ret = gcampda_check_last_record(gc,d,tdf)) < 0 ) {
			gcampda_close_data_file(gc);
			free(datafile);
			return ret;
		}
	}
	if (header_flag) 
		pak_fprintf_table_header(gc, tdf);

	free(datafile);

	return 0;
}


static int gcampda_update_telemetry_table(GC_CONF *gc)
{
	int header_flag = 0;
	char *datafile;
	FILE *fp;
	struct timeval *t;
	time_t current;

	asprintf(&datafile,"%s/%s-telemetry.dat", GC_DATA(gc), 
		 GC_STATION_NAME(gc));
	if ( access(datafile, F_OK) != 0) header_flag = 1;
	fp = fopen(datafile, "a");
	if ( fp == NULL ){
                SYSLOG(LOG_ERR, "Error opening file: %s", datafile);
		free(datafile);
                return -1;
        }
	if (header_flag) { 
		fprintf(fp, "# Field definitions:\n");
		fprintf(fp, "# Date [sec], Start time [sec], Uptime [Days], Lithium Battery [Volts], Low 12V count, Low 5V count, Skipped Scan, Skipped System Scan, Watchdog Errors, Memory Free [bytes]\n#\n");
	}
	t = cr_get_start_time(gc);
	current = time(NULL);
/*
  if (t == NULL) { Removed: 06/08/2011 
  t = (struct timeval *) malloc(sizeof(struct timeval));
  memset(t, 0, sizeof(struct timeval));
  }
*/
	if (t != NULL) { /* Added 06/08/2011 */ 
		fprintf(fp, "%10ld %10ld %4d %4.2f %2d %2d %2d %2d %2d %6d\n",
			current, t->tv_sec, (int) ((current - t->tv_sec)/86400),
			cr_get_lithium_battery(gc),
			cr_get_low_12v_count(gc), 
			cr_get_low_5v_count(gc),
			cr_get_skipped_scan(gc),
			cr_get_skipped_system_scan(gc),
			cr_get_watchdog_err(gc),
			cr_get_memory_free(gc));
		free(t);
	}
	fclose(fp);
/*  Removed: 06/08/2011 
  if (t != NULL)
  free(t);
*/
	free(datafile);

	return 0;
}


int gcampda_data_download_table(GC_CONF *gc, DL_CONF *d, CR_TDF *tdf)
{
	CR_TDF *table;
	int ret = -1;

	if ( DL_TBL_FLAG(d) == 1){
		table = cr_tdf_find_table(tdf, DL_TBL_NAME(d));
		if (table != NULL) {
			if (DL_TBL_SIGNATURE(d) == CR_TDF_SIGNATURE(table) ) {
				if ( (ret =
				      gcampda_open_data_file(gc, d, table))==0) {
					ret = cr_get_table_from(
						gc, table, 
						DL_TBL_LAST_RECORD(d));
					DL_TBL_LAST_RECORD(d) = 
						CR_TDF_LAST_RECORD_READ(table);
					gcampda_close_data_file(gc);
				}
			}
		}
	}

	return ret;
}


int gcampda_data_lock(GC_CONF *gc)
{
	char *lock_name;
	int ret = 1;

	asprintf(&lock_name,"%s/%s", GC_DATA(gc), GC_STATION_NAME(gc));
	if ( if_lock_file(lock_name) != 1 )
		ret = lock_file(lock_name);

	free(lock_name);
	
	return ret;
}


int gcampda_data_unlock(GC_CONF *gc)
{
	char *lock_name;
	int ret;

	asprintf(&lock_name,"%s/%s", GC_DATA(gc), GC_STATION_NAME(gc));
	ret = unlock_file(lock_name);
	free(lock_name);

	return ret;
}


int gcampda_data_download(GC_CONF *gc)
{
	DL_CONF *d = NULL, 
		*dp;
	CR_TDF *tdf = NULL;
	int ret = -1;

	
	if (cr_get_tdf(gc, &tdf) != 0) {
		return -1;
	}
	dp = d = gcampda_dl_conf_tables_read(gc);

	while(dp != NULL) {
		ret = gcampda_data_download_table(gc, dp, tdf);
		dp = DL_TBL_NEXT(dp);
			
	}
	gcampda_dl_conf_write(gc, d);
	gcampda_update_telemetry_table(gc);

	cr_tdf_free(&tdf);
	ret = gcampda_dl_conf_free(d);
	
	return ret;
}
