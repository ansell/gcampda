/** 
 *   main: gcampda aplication.
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
 *  v0.1.8: 2013/06/11: Add check last record for each data table
 *          (chk_last_record). It set via gcampda.conf or switch in 
 *          command line.
 *          Fix GC_TRAN_NBR_NEW() definition in gcampda.h (not affect 
 *          the old code).
 *          Fortmat in record number change from %5d to %10d.
 *          Add gcampda version in new created header.
 *
 *  v0.1.7: 2013/03/26: Fix retry in pak_send_and_spect(), the logger
 *          not send Please Wait Message (0xa1)...
 * 
 *  v0.1.6: 2012/10/24: Implement --nosyslog switch.
 *          Add other formats for timestamp; now set it via gcampda.conf
 *          (time_fmt = (man strftime)).
 *          Fix station_name = NULL.
 *
 *  v0.1.5: 2012/08/30 - APPLE port >= 10.7.4.
 *
 *  v0.1.4: 2012/04/24 - Implemente lock in download data directory to
 *          prevent concurrent download.
 *
 *  v0.1.3: 2011/08/06 - Fix spurious telemetry entry on fail data adq. 
 *          (function gcampda_update_telemetry_table on gcampda.c).
 *
 *  v0.1.2: 2011/07/27 - Fix last data missing in cr_get_table_from function 
 *          on pakbus.c.
 *
 *  v0.1.1: Add IP support.
 * 
 *  v0.1.0: Initial version for serial link.
 *
 **/
#define _GNU_SOURCE

//#define VERSION "0.1.8"

#include <stdint.h> 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <parse_tools.h>
#include <my_stdio.h>

#include "rs232.h"
#include "pakbus.h"
#include "gcampda.h"
#include "version.h"

const char *name_definition = {"GNU Campbell data acquisition"};
static const char *help_text[] ={
        "OPTIONS:",
	" --init\t\t - Init configuration file for download tables.",
	" -d [--download] - Download tables from data logger.",
	" --config\t - Specifies global configuration file, default: "DEFAULT_CONF_FILE,
	" --setclk\t - Set logger clock from system clock.",
	" --getclk\t - Get logger clock.",
	" -t [--table] {name} - Get table name.",
	" --from {record} - Number of record for geting table.",
	" --nosyslog\t - Disable syslog notifications.",
	" --nochklastrecord - Disable check last record in each data table.",
	" -l\t\t - List tables name.",
	" -f\t\t - List tables and field names.",
	" -a\t\t - List logger directory .DIR.",
        " -V\t\t - Show program version.",
#       ifdef VERBOSE_MODE
        " -v\t\t - Verbose mode (incremental).",
#         ifdef DEBUG_MODE
	" --debug\t - Debug mode.",
#         endif
#       endif
        " -h [--help]\t - This help !!!.",
        ""
};

void version(char *program_name)
{
        fprintf(stderr,"%s: ", name_definition);
        fprintf(stderr,"%s version %s\n", program_name, VERSION);
}


void usage(char *program_name)
{
        version(program_name);
        fprintf(stderr,
 "Usage: \t%s [--config conf_file] {[--setclk|--getclk]}{[--init|--download]}|{[--table NAME [--from RECORD]][-lfa]}[-vV]\n",
                program_name);
}


void help()
{
        int i=0;

        while((char) *help_text[i] != 0)
                fprintf(stderr,"%s\n",help_text[i++]);
        fprintf(stderr,"\n");
}


/* sed 's/\([0-9a-z]\{2\}\)/0x\1,/g' */
/* 90 01 0f fc 75 d4 */
/* a0 01 6f fc 00 01 0f fc 09 e3 01 01 00 54 88 16 */

int main(int argc, char **argv)
{
	GC_CONF *gc;
	CR_TDF *tdf = NULL,
		*table = NULL;
	DL_CONF *dlc = NULL;
        char *program_name;
	char *table_name = NULL;
	uint32_t table_from = 0;
	char *config_file = NULL;
	int list_tables_flag = 0;
	int list_fields_flag = 0;
	int list_files_flag = 0;
	int set_clk_flag = 0;
	int get_clk_flag = 0;
	int init_flag = 0;
	int download_flag = 0;
	int chk_last_record_flag = 1;
	int sys_log_flag = 1;
        char *cp;
        int op;
        int ret = 0;
	struct timeval *t;
        struct option const longopts[] =
        {
                {"help", 0, 0, 'h'},
#ifdef VERBOSE_MODE
                {"verbose", 0, 0, 'v'},
#endif
#ifdef DEBUG_MODE
                {"debug", 0, 0, 201},
#endif
		{"config", 1, 0, 202},
		{"init", 0, 0, 203},
		{"download", 0, 0, 'd'},
		{"table", 0, 0, 't'},
		{"from", 1, 0, 204},
		{"setclk", 0, 0, 205},
		{"getclk", 0, 0, 206},
		{"nosyslog", 0, 0, 207},
		{"nochklastrecord", 0 ,0, 208},
                {0,  0,  0,  0}
        };

        /* who is my name? */
        if ((cp = strrchr(argv[0], '/')) != NULL) program_name  = cp + 1;
        else program_name  = argv[0];

#ifdef DEBUG_MODE
        debug_flag = 0;
#endif
        SET_VERBOSE_FLAG(PERR);
        while ((op = getopt_long(argc, argv, "vhVt:flad",longopts, 0)) != EOF)
                switch( op ){
#ifdef DEBUG_MODE
                case 201:
                        debug_flag = 1;
                        break;
#endif
		case 202:
			config_file = optarg;
			break;
                case 203:
                        init_flag = 1;
                        break;
		case 204:
			table_from = atoi(optarg);
			break;
		case 205:
			set_clk_flag = 1;
			break;
		case 206:
			get_clk_flag = 1;
			break;
		case 207:
			sys_log_flag = 0;
			break;
		case 208:
			chk_last_record_flag = 0;
			break;
                case 'd':
                        init_flag = 0;
			download_flag = 1;
                        break;
                case 'v':
                        INCREMENT_VERBOSE_FLAG();
                        break;
                case 'V':
                        version(program_name);
                        exit(1);
                        break;
		case 't':
			table_name = optarg;
			break;
		case 'f':
			list_fields_flag = 1;
		case 'l':
			list_tables_flag = 1;
			break;
		case 'a':
			list_files_flag = 1;
			break;
                case 'h':
                        usage(program_name);
                        help();
                        exit(1);
                default:
                        usage(program_name);
                        exit(1);
                }
	if ( sys_log_flag )
		OPENLOG(program_name, LOG_PID|LOG_NDELAY, LOG_SYSLOG);

	if (config_file == NULL )
		asprintf(&config_file,"%s", DEFAULT_CONF_FILE);

	if ((gc = gcampda_conf_parse(config_file)) == NULL) {
		SYSLOG(LOG_CRIT, "Can't read configuration file: %s", 
		       config_file);
		CLOSELOG();
		return 2;
	}
	if ( ! chk_last_record_flag )
		GC_CHK_LAST_RECORD(gc) = chk_last_record_flag; /* or 0 */
	SYSLOG(LOG_NOTICE,"version %s start", VERSION);
	SYSLOG(LOG_NOTICE, "Configuraton file: %s", config_file);
	if (gcampda_open(gc) < 0) {
		SYSLOG(LOG_CRIT, "can't open %s", GC_DEV_NAME(gc));
		CLOSELOG();
		return 1;
	} 

	if ((ret = gcampda_init_connection(gc)) != 0) {
		gcampda_close(gc);
		SYSLOG(LOG_CRIT, 
		       "Unable to communicate with the data logger (%d)", ret);
		CLOSELOG();
		return 2;
	}

	gcampda_get_station_name(gc);
	if ( GC_STATION_NAME(gc) == NULL) {
		gcampda_close(gc);
		SYSLOG(LOG_CRIT, "Can't get station name");
		CLOSELOG();
		return 3;
	}
     	SYSLOG(LOG_INFO, "Station name: %s", GC_STATION_NAME(gc));

	if (set_clk_flag) {
		t = (struct timeval *) malloc(sizeof(struct timeval));
		gettimeofday(t, NULL);
		cr_clock_set(gc, t);
		SYSLOG(LOG_INFO,"Set logger clk: %s", 
		       ctime((const time_t *) &t->tv_sec));
		free(t);
	}

	t = cr_clock_get(gc);
	if ( t != NULL) {
		SYSLOG(LOG_INFO,"%s", ctime((const time_t *) &t->tv_sec));
		if (get_clk_flag) {
			printf("Station: %s, clk: %s",
			       GC_STATION_NAME(gc),
			       ctime((const time_t *) &t->tv_sec));
		}
		free(t);
	}

	if (init_flag) {
		if (cr_get_tdf(gc, &tdf) == 0) {
			gcampda_tdf2dl_conf(tdf, &dlc);
			gcampda_dl_conf_write(gc, dlc);
			gcampda_dl_conf_free(dlc);
			dlc = NULL;
		} else {
			SYSLOG(LOG_ERR,"Can't get TDF");
		}
	} else if (download_flag) {
		if ( gcampda_data_lock(gc) == 0) {
			gcampda_data_download(gc);
			gcampda_data_unlock(gc);
		} else {
			SYSLOG(LOG_ERR,"Cant lock data dir");	
		}
	} else {
		if (list_files_flag)
			cr_ls(gc);
		if(list_tables_flag || table_name != NULL) {
			if (cr_get_tdf(gc, &tdf) == 0) {
				if(list_tables_flag) 
					cr_tdf_list(tdf, list_fields_flag);
				if (table_name != NULL) {
					table = cr_tdf_find_table(tdf, 
								  table_name);
					if (table != NULL) {
						ret = cr_get_table_from(
							gc, table, table_from);
					}
				}
				cr_tdf_free(&tdf);
			} else {
				SYSLOG(LOG_ERR,"Can't get TDF");
			}
		}
	}

	gcampda_finished_connection(gc);
	gcampda_close(gc);
	gcampda_conf_free(gc);
	gcampda_dl_conf_free(dlc);

	SYSLOG(LOG_NOTICE,"Terminate.");
	CLOSELOG();
	return ret;
}

