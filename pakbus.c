/** 
 *   pakbus: 
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
 * References:
 *
 * [1] BMP5 Transparent Commands Manual, Rev. 9/08, Campbell
 * Scientific Inc., 2008 
 * 
 * [2] PakBus Networking Guide for the CR10X, CR510, CR23X, and CR200
 * Series and LoggerNet 2.1C, Rev. 3/05, Campbell Scientific Inc.,
 * 2004-2005
 *
 **/
#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <my_stdio.h>

#include "pakbus.h"
#include "rs232.h"
#include "my_io.h"
#include "gcampda.h"
#include "version.h"

#ifdef DEBUG_MODE
#define TOCHAR(c) ((c)>=0x20&&(c)<=0x7e?c:'.')
void hexdump(char prefij, unsigned char *buff, int size)
{
	int i, j, k;
	int mod16 = size % 16;
	FILE *fp = stderr;

	fprintf(fp, "\n");
	for (i = 0; i < size; i+=16) {
		if (prefij != 0 ) 
			fprintf(fp, "%c %08x  ",prefij, i);
		else
			fprintf(fp, "%08x  ", i);
		for (j = i; j < i+8 && j < size;j++) fprintf(fp, "%02x ", *(buff+j));
		fprintf(fp, " ");
		for (k = j; k < j+8 && k < size;k++) fprintf(fp, "%02x ", *(buff+k));
		if(k == size && mod16) 
			for(j=16;j > mod16;j--)
				fprintf(fp, "   ");
		fprintf(fp, " |");
		for (j = i; j < i+16 && j < size;j++) 
			fprintf(fp, "%c", TOCHAR(*(buff+j)));
		if(j == size && mod16) 
			for(k=16;k > mod16;k--)
				fprintf(fp, " ");
		fprintf(fp, "|\n");
	}

	return;
}
#endif


static int pk_test_sig_nullifier(unsigned char *a, uint32_t len)
{
	int sed = calc_sig_nullifier(calc_sig_for(a, len - 2, 0xaaaa));

	return ((*(a+len-2) == ((sed >> 8) & 0xff)) &&
		*(a+len-1) == (sed & 0xff) ) ? 1 : 0;
}


#define pak_send(d, p) cr_send(d, PAKBUS_DATA(p), PAKBUS_STO_LEN(p))


static int cr_send(int dev, unsigned char *buff, uint32_t len)
{
	unsigned char *buff_out;
	int len_out = 0;
	int ret;
	int i;
	
	if (len > 1010) {
		SYSLOG(LOG_WARNING, "Bad len: %d", len);
		return -1;
	}
	buff_out = (unsigned char *) malloc(1024);

	*(buff_out+len_out++) = SERSYNCBYTE;
	for(i = 0; i < len; i++) {
		if(*(buff+i) == QUOTEBYTE || *(buff+i) == SERSYNCBYTE) {
			*(buff_out+len_out++) = QUOTEBYTE;
			*(buff_out+len_out++) = *(buff+i) + 0x20;
		} else
			*(buff_out+len_out++) = *(buff+i);
	}
	*(buff_out+len_out++) = SERSYNCBYTE;
	ret = hw_write(dev, buff_out, len_out);
#ifdef DEBUG_MODE
	if (debug_flag && ret > 0)
		hexdump('s', buff_out+1, len_out-2);
#endif			
	free(buff_out);

	return ret > 0 ? 0:-1;
}


static int cr_rcv(int dev, uint16_t soft_addr, unsigned char **pk, uint32_t *len)
{
	unsigned char *buff = (unsigned char *) malloc(1024);
	int ret;
	int quote = 0;
	int syncbyte = 0;
	
	*len = 0;
	*pk = NULL;
	do {
		ret = hw_read(dev, buff, 1);
	} while (*buff != SERSYNCBYTE && ret > 0);
	*(buff+*len) = 0;
	while (ret > 0  && ! syncbyte) {
		ret = hw_read(dev, buff+*len, 1);
		if(ret > 0) {
			if ( *(buff+*len) == QUOTEBYTE) {
				quote = 1;
			} else if ( *(buff+*len) == SERSYNCBYTE ) {
				syncbyte = 1;
			} else {
				if (quote) {
					quote = 0;
					if ( *(buff+*len) == 0xdd )
						*(buff+*len) = SERSYNCBYTE;
					else if ( *(buff+*len) == 0xdc )
						*(buff+*len) = QUOTEBYTE;
					else { /* an error condition */
						SYSLOG(LOG_ERR,
						      "unspected byte: 0x%02x",
						      *(buff+*len) );
						ret = -1;
					}
				}
				(*len)++;
				if (*len > 1010) {
					SYSLOG(LOG_CRIT,"Bad len: %d", *len);
					exit(2);
				}
			}
		}
	}
#ifdef DEBUG_MODE
	if (debug_flag)
		hexdump('r', buff, *len);
#endif			
	if ( ret > 0) {
		if (PB_GET_DST_ADDR(buff) == soft_addr ||
		    PB_GET_DST_ADDR(buff) == 0xfff  	  ) {
			if (pk_test_sig_nullifier(buff, *len)) {
				*pk = (unsigned char *) malloc(*len);
				memcpy(*pk, buff, *len);
			} else {
				SYSLOG(LOG_WARNING,"Invalid sig nullifier");
				ret = -2;
			}
		} else {
			SYSLOG(LOG_WARNING,"Invalid DstPhyAddr: 0x%03x", 
			      PB_GET_DST_ADDR(buff) );
			ret = -3;
		}
	}
#ifdef DEBUG_MODE
	else {
		PVERB(PINFO,"ret = %d", ret);
	}
#endif	     
	free(buff);

	return ret;
}


int pak_rcv(int dev, uint16_t soft_addr, PAKBUS **pak) 
{
	PAKBUS *pk = (PAKBUS *) malloc(sizeof(PAKBUS));	
	int ret = cr_rcv(dev, soft_addr, 
			 &(PAKBUS_DATA(pk)), (uint32_t *) &PAKBUS_LEN(pk));
	if (ret > 0) {
		PAKBUS_STO_LEN(pk) = PAKBUS_H_LEN; 
		PAKBUS_READ_INDEX(pk) = PAKBUS_H_LEN;
		*pak = pk;
	} else {
		pak_free(pk);
		*pak = NULL;
	}

	return ret;
}


static void cr_make_pakbus_header(GC_CONF *gc, PAKBUS *pak, int priority)
{
	unsigned char *pk = PAKBUS_DATA(pak);
	
	PB_SET_LINK_STATE (pk, LS_READY);
	PB_SET_DST_ADDR   (pk, GC_LOGGER_ADDR(gc));
	PB_SET_DST_NODE_ID(pk, GC_LOGGER_ADDR(gc));
	PB_SET_SRC_ADDR   (pk, GC_SOFT_ADDR(gc));
	PB_SET_SRC_NODE_ID(pk, GC_SOFT_ADDR(gc));
	PB_SET_EXP_MORE_CODE(pk, EMC_EXPECT_MORE);
	PB_SET_PRIORITY   (pk, priority);
	PB_SET_HI_PROTO_CODE(pk, 1);
	PB_SET_HOP_CNT    (pk, 0);

	return;
}


/**
 [1] section 2.2 PakBus Control Packets (PakCtrl)
**/


int pakctrl(GC_CONF *gc, char linkstate, int addr_to, char expmorecode, 
	    char priority)
{
	unsigned char *pk = (unsigned char *) malloc(sizeof(PAKBUSH) +
						     sizeof(PAKBUSH_HIP) + 2);
	int ret;

	/* PAKBUSH: */
	PB_SET_LINK_STATE(pk, linkstate);
	PB_SET_DST_ADDR(pk, addr_to);
	PB_SET_EXP_MORE_CODE(pk, expmorecode);
	PB_SET_PRIORITY(pk, priority);
	PB_SET_SRC_ADDR(pk, GC_SOFT_ADDR(gc));
	/* PAKBUSH_HIP: */
	PB_SET_HI_PROTO_CODE(pk, 0);
	PB_SET_DST_NODE_ID(pk, addr_to);
	PB_SET_HOP_CNT(pk, 0);
	PB_SET_SRC_NODE_ID(pk, GC_SOFT_ADDR(gc));
	
	PB_SET_SIG_NULLIFIER(pk, sizeof(PAKBUSH) + sizeof(PAKBUSH_HIP) + 2);

	ret = cr_send(GC_FD(gc), pk, sizeof(PAKBUSH) + sizeof(PAKBUSH_HIP) + 2);
	free(pk);

	return ret;
}


/**
 [1] section 2.2.1 Deliverry Failure Message (MsgType 0x81)
**/

/* FIXME: not tested. */
static int delivery_failure_message(GC_CONF *gc, PAKBUS *pki, char err_code)
{
	PAKBUS *pk = pak_make(PKL(7+1));
	unsigned short val;
	int ret;

	SYSLOG(LOG_WARNING,"Packet out of sequence.");
#ifdef DEBUG_MODE
	if ( debug_flag )
		hexdump('F', PAKBUS_DATA(pki), PAKBUS_LEN(pki));
#endif
	return 0; /* Not tested */
	cr_make_pakbus_header(gc, pk, 0);
	pak_add_byte(pk, 0x81); /* MsgType */
	pak_add_byte(pk, 0); /* TranNbr */
	pak_add_byte(pk, err_code); /*  Failure code */
	val = PB_GET_HI_PROTO_CODE(PAKBUS_DATA(pki)) << 12 | 
		PB_GET_DST_NODE_ID(PAKBUS_DATA(pki));
	pak_add_short(pk, val); 
	val = PB_GET_HOP_CNT(PAKBUS_DATA(pki)) << 12 | 
		PB_GET_SRC_NODE_ID(PAKBUS_DATA(pki));
	pak_add_short(pk, val); 
	pak_add_byte(pk, 0);
	PAK_ADD_SIG_NULLIFIER(pk);

	ret = pak_send(GC_FD(gc), pk);
	pak_free(pk);

	return ret;
}

/**
 [1] section 2.2.2 Hello Transaction (MsgType 0x09 & 0x89)

     The Hello transaction is used to verify that two-way
     communication can occur with a specific node. An application does
     not have to send a Hello command to the datalogger but the
     datalogger may send a Hello command to which the application
     should respond.

     It is important that the application copy the exact transaction
     number from the received Hello command meassage into the Hello
     response sent to the datalogger. Since the application is
     connected directly to the datalogger, the hop metric received in
     the command message packet from the datalogger can be copied by
     the application and inserted in the response message packet. In
     addition, the application should not identify itself as a router
     in the IsRouter parameter of the response message packet.
**/

int cr_hello_transaction_request(GC_CONF *gc)
{
	PAKBUS *pk = pak_make(PKL(4+L_UInt2));
	int ret;

	cr_make_pakbus_header(gc, pk, 2); /* ops HI_PROTO_CODE is set */
	PB_SET_HI_PROTO_CODE(PAKBUS_DATA(pk), 0); /* :) */
	pak_add_byte(pk, 0x09); /* MsgType */
	pak_add_byte(pk, GC_TRAN_NBR(gc)); /* TranNbr */
	pak_add_byte(pk, 0); /* Is Router */
	pak_add_byte(pk, GC_HOP_METRIC(gc)); /* Hop Metric */ // 0x02
	pak_add_short(pk, 60); // 0xffff
	PAK_ADD_SIG_NULLIFIER(pk);

	ret = pak_send(GC_FD(gc), pk);
	pak_free(pk);

	return ret;
}


static int hello_transaction_respond(GC_CONF *gc, PAKBUS *pki)
{
	PAKBUS *pk = pak_make(PKL(4+L_UInt2));
	unsigned char t_nbr = pak_read_byte(pki);
	int ret = pak_read_byte(pki); /* dummy read of "Is Route" */
	unsigned char hop_metric = pak_read_byte(pki);
	int verify_intv = (int) ((double) pak_read_short(pki) / 2.5);


	GC_HOP_METRIC(gc) = hop_metric;
	verify_intv = 1800;
	cr_make_pakbus_header(gc, pk, 2);
	pak_add_byte(pk, 0x89); /* MsgType */
	pak_add_byte(pk, t_nbr); /* TranNbr */
	pak_add_byte(pk, 0); /* Is Router */
	pak_add_byte(pk, GC_HOP_METRIC(gc)); /* Hop Metric */
	pak_add_short(pk, verify_intv);
	PAK_ADD_SIG_NULLIFIER(pk);

	ret = pak_send(GC_FD(gc), pk);
	pak_free(pk);

	return ret;
}


PAKBUS *pak_spect(GC_CONF *gc, unsigned char type)
{
	unsigned char type_i, 
		nbr_i;
	unsigned char nbr = GC_TRAN_NBR(gc);
	PAKBUS *pk = NULL;
	PAKBUS *pk_09 = NULL;
	PAKBUS *pk_unknow = NULL;
	int ret;
	int done = 0;
 
	do {
		if ( pk != NULL) {
			pak_free(pk);
			pk = NULL;
		}
		ret = pak_rcv(GC_FD(gc), GC_SOFT_ADDR(gc), &pk);

		if (pk == NULL ) {
			SYSLOG(LOG_ERR, 
			       "Fail specting 0x%02x, trn: 0x%02x, ret: %d",
			       type, nbr, ret);
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
		type_i = 0;
		if ( PAKBUS_LEN(pk) > 6) {
			if (PAKBUS_GET_LINK_STATE(pk) == LS_RING) {
				cr_ready(gc);
			} else {
				type_i = pak_read_byte(pk);
				if ( type_i == type) {
					nbr_i = pak_read_byte(pk);
					if (nbr_i == nbr) {
						done = 1;
					}
				} else {
					switch((int) type_i) {
					case 0x09:
						if ( pk_09 != NULL) {
							pak_free(pk_09);
						}
						pk_09 = pk;
						pk = NULL;
						
						break;
					case 0x0e:
						cr_hello_transaction_request(gc);
						break;
					default:
						if ( pk_unknow != NULL) {
							pak_free(pk_unknow);
						}
						pk_unknow = pk;
						pk = NULL;
						break;
					}
				}
				PVERB(PINFO,"type: 0x%02x (0x%02x), nbr: 0x%x (0x%02x)",
				      type_i,type, nbr_i, nbr);
			}
		}
	} while (!done);
	
	if(pk_09 != NULL) {
		hello_transaction_respond(gc, pk_09);
		pak_free(pk_09);
	}

	if(pk_unknow != NULL) {
		delivery_failure_message(gc, pk_unknow, 0x04);
		pak_free(pk_unknow);
	}

	return pk;
}


PAKBUS *pak_send_and_spect(GC_CONF *gc, PAKBUS *pk, unsigned char type_spect)
{
	int retry = 0;
	PAKBUS *pki;

	do {
		if ( retry ){ /* FIXME 20130326 */
			PVERB(PINFO, "waiting..."); 
			cr_ready(gc);
		}else {if (pak_send(GC_FD(gc), pk) < 0)
			return NULL;
		}
		pki = pak_spect(gc, type_spect);
	} while(pki == NULL && retry++ < 3); //2
	
	if(pki == NULL)
		SYSLOG(LOG_WARNING, "pki: NULL");
	
	return pki;
}


int cr_send_0xbd(GC_CONF *gc, int num)
{
	unsigned char wake[num];

	memset(wake, SERSYNCBYTE, num);

	return hw_write(GC_FD(gc), wake, num);
}


int cr_at(GC_CONF *gc)
{
	unsigned char *pk = (unsigned char *) malloc(sizeof(PAKBUSH) + 2);
	unsigned char *buff = NULL;
	int ret;
	int retry = 0;
	int done = 0;
	uint32_t len;

	PB_SET_LINK_STATE(pk, LS_RING);
	PB_SET_DST_ADDR(pk, GC_LOGGER_ADDR(gc));
	PB_SET_EXP_MORE_CODE(pk, EMC_LAST);
	PB_SET_PRIORITY(pk, 0);
	PB_SET_SRC_ADDR(pk, GC_SOFT_ADDR(gc));
	PB_SET_SIG_NULLIFIER(pk, sizeof(PAKBUSH) + 2);

	ret = cr_send(GC_FD(gc), pk, sizeof(PAKBUSH) + 2);		
	do {
		if (ret > 0) {
			if (buff != NULL) free(buff);
			ret = cr_rcv(GC_FD(gc), GC_SOFT_ADDR(gc),&buff, &len);
		}
		if (ret > 0 && buff != NULL) {
			PVERB(PINFO,
			      "len(%d) = %d, PB_GET_SRC_ADDR(0x%x) = 0x%x, PB_GET_DST_ADDR(0x%x) = 0x%x, PB_GET_LINK_STATE(0x%x) = 0x%x",
			      (sizeof(PAKBUSH) + 2),len,
			      GC_LOGGER_ADDR(gc), PB_GET_SRC_ADDR(buff),
			      GC_SOFT_ADDR(gc), PB_GET_DST_ADDR(buff),
			      LS_READY, PB_GET_LINK_STATE(buff));
			      

			if (len == (sizeof(PAKBUSH) + 2) &&
			    PB_GET_SRC_ADDR(buff) == GC_LOGGER_ADDR(gc)  && 
			    PB_GET_DST_ADDR(buff) == GC_SOFT_ADDR(gc) )     {
				if (PB_GET_LINK_STATE(buff) == LS_READY) {
					done = 1;
				}
			}
			#if 0
			/* FEXME: */
			if (PB_GET_LINK_STATE(buff) == 0xe /*LS_RING*/) {
				cr_ready(gc);
			}				
			#endif

		}
		ret = 1;
	} while (retry++ < 5 && !done);

	free(pk);
	if (buff != NULL) free(buff);
	
	return (done == 1?0:-1);
}


int cr_ready(GC_CONF *gc)
{
	unsigned char *pk = (unsigned char *) malloc(sizeof(PAKBUSH) + 2);
	int ret;

	PB_SET_LINK_STATE(pk, LS_READY);
	PB_SET_DST_ADDR(pk, GC_LOGGER_ADDR(gc));
	PB_SET_EXP_MORE_CODE(pk, EMC_LAST);
	PB_SET_PRIORITY(pk, 0);
	PB_SET_SRC_ADDR(pk, GC_SOFT_ADDR(gc));
	PB_SET_SIG_NULLIFIER(pk, sizeof(PAKBUSH) + 2);

	ret = cr_send(GC_FD(gc), pk, sizeof(PAKBUSH) + 2);		
	free(pk);

	return ret;
}


/** 
    [1] 2.3.2 Clock Transaction (0x17 & 0x97)

    The clock transaction can be used to check the current time or to
    adjust the datalogger clock. Note the inherent danger of retrying
    a clock set command. If the client were to simply retry a failed
    clock set attempt without knowing whether the first try reached
    the station, there is a danger the clock will be changed more than
    wanted. If a clock set attempt fails, the client should read the
    clock again to determine whether additional adjustments are
    needed.

**/

static int cr_clock_get_set(GC_CONF *gc, int32_t offset_sec,
		     int32_t offset_usec,
		     int32_t *logger_offset_sec,
		     int32_t *logger_offset_usec)
{
	PAKBUS *pk = pak_make(PKL(4+L_NSec));
	PAKBUS *pki = NULL;
	int ret = -1;
	unsigned char resp_code;

	GC_TRAN_NBR_NEW(gc);
	cr_make_pakbus_header(gc, pk, 2);
	pak_add_byte(pk, 0x17); /* MsgType */
	pak_add_byte(pk,GC_TRAN_NBR(gc)); /* TranNbr */
	pak_add_short(pk, GC_SCODE(gc));
	pak_add_int(pk, offset_sec);
	pak_add_int(pk, offset_usec); /* FIXME ??? */
	PAK_ADD_SIG_NULLIFIER(pk);

	pki = pak_send_and_spect(gc, pk, 0x97);
	pak_free(pk);

	if (pki == NULL) {
		SYSLOG(LOG_WARNING, "pki null (?)");
		return -1;
	}

	resp_code = pak_read_byte(pki);
	if ( resp_code == 0 ) {
		if (logger_offset_sec != NULL) 
			*logger_offset_sec = pak_read_int(pki);
		if (logger_offset_usec != NULL)
			*logger_offset_usec = pak_read_int(pki);
		ret = 0;
	}
	pak_free(pki);

	return ret;
}


struct timeval *cr_clock_get(GC_CONF *gc)
{
	struct timeval *t = NULL;
	int32_t sec, usec;

	if (cr_clock_get_set(gc, 0, 0, &sec, &usec) == 0) {
		t = (struct timeval *) malloc(sizeof(struct timeval));
		t->tv_sec = sec + TIME_CR2UNIX_TZ(GC_TZ(gc));
		t->tv_usec = usec;
	}

	return t;
}


int cr_clock_set(GC_CONF *gc, struct timeval *t)
{
	int32_t sec, usec;
	int ret;

	if ((ret = cr_clock_get_set(gc, 0, 0, &sec, &usec)) == 0) {
		sec = t->tv_sec - TIME_CR2UNIX_TZ(GC_TZ(gc)) - sec;
		usec = t->tv_usec - usec;
		SYSLOG(LOG_INFO, "clk correction: %d sec, %d usec", sec, usec);
		ret = cr_clock_get_set(gc, sec, usec, NULL, NULL);
	}

	return ret;
}


/** 
    [1] 2.3.3.1 File Download Transaction (MsgType 0x1c & 0x9c)
**/

// TODO...
#if 0
int cr_send_file(int dev, char *filename)
{
	PAKBUS *pk = pak_make(PAKBUS_H_LEN+/*FIXME*/);
	int ret;

	GC_TRAN_NBR_NEW(gc);
	cr_make_pakbus_header(gc, pk, 2);
	pak_add_byte(pk, 0x17); /* MsgType */
	pak_add_byte(pk, GC_TRAN_NBR(gc)); /* TranNbr */
	pak_add_short(pk,GC_SCODE(gc));
	pak_add_int(pk, offset);
	pak_add_int(pk, 0); /* FIXME ??? */
	PAK_ADD_SIG_NULLIFIER(pk);

	ret = pak_send(dev, pk);
	if ( ret ) ret = (int) GC_TRAN_NBR(gc);
	pak_free(pk);

	return ret;
}
#endif


/** 
    [1] 2.3.3.2 File Upload Transaction (MsgType 0x1d & 0x9d)
**/

int cr_get_file_size(GC_CONF *gc, char *fname, 
		     char **file_buff, size_t *buff_size_out,
		     size_t size)
{
	/* if size < 0, size is unknow */
	PAKBUS *pk, *pki;
	int ret = -1;
	unsigned char close_flag = 0;
	uint32_t foffset = 0;
	//int swath = 512;//128; /* The number of bytes to read (FIXME:997)*/
	int swath = (size < 0?512:(size < 997?size:512));
	int swath_read = 0;
	int complet = 0;
	unsigned char resp_code;
	int len;

	*file_buff = NULL;
	*buff_size_out = -1;
	pk = pak_make(PKL(4 + strlen(fname) + 8));
	GC_TRAN_NBR_NEW(gc);
	cr_make_pakbus_header(gc, pk, 2);


	while (!complet) {
		pak_add_byte(pk, 0x1d); /* MsgType */
		pak_add_byte(pk, GC_TRAN_NBR(gc)); /* TranNbr */
		pak_add_short(pk, GC_SCODE(gc));
		pak_add_string(pk, fname);
		pak_add_byte(pk, close_flag);
		pak_add_int(pk, foffset);
		len = (size < 0?swath:((foffset+swath)>size?size-foffset:swath));
		pak_add_short(pk, len); /* The number of bytes to read */
		PAK_ADD_SIG_NULLIFIER(pk);
		pki = pak_send_and_spect(gc, pk, 0x9d);
		PAKBUS_STO_LEN(pk) = PAKBUS_H_LEN;

		if (pki == NULL) {
			SYSLOG(LOG_WARNING, "pki null (?)");
			pak_free(pk);
			if (*file_buff != NULL) free(*file_buff);
			*buff_size_out = 0;
			return -1;
		}

		resp_code = pak_read_byte(pki);
		if ( resp_code == 0 ) {
			foffset = pak_read_int(pki);
			swath_read = PAKBUS_LEN(pki) - PAKBUS_H_LEN - 9;
			*file_buff = realloc(*file_buff, foffset + swath_read);
			*buff_size_out = foffset + swath_read;
			memcpy((*file_buff)+foffset, PAKBUS_DATA(pki)+ PAKBUS_READ_INDEX(pki), swath_read);
			foffset += swath_read;
			PVERB(PINFO, "read: %d, spect: %d, total: %d", 
			      swath_read, len, foffset);
			if ( (int) size < 0 && (swath_read < len)) {
				complet = 1;
				ret = 0;
			} else if ( foffset == (uint32_t) size ) {
				complet = 1;
				ret = 0;
			}
		} else {
			if (*file_buff != NULL) free(*file_buff);
			*file_buff = NULL;
			complet = 1;
			SYSLOG(LOG_ERR, "error reading %s: 0x%02x", fname, resp_code);
			ret = - resp_code;
			*buff_size_out = 0;
		}
		pak_free(pki);
	}
	pak_free(pk);

	return ret;
}


int cr_get_file(GC_CONF *gc, char *fname, char **file_buff, 
		size_t *buff_size_out)
{
	return cr_get_file_size(gc, fname, file_buff, buff_size_out, -1);
}


int cr_get_dir(GC_CONF *gc, CR_FD **fd)
{
	char *buff = NULL;
	size_t size;

	if (*fd != NULL) cr_fd_free(fd);
	*fd = NULL;
	cr_get_file(gc, ".dir", &buff, &size);
	if ((int) size > 0) {
		if (*buff == 0x01) {
			cr_fd_store(fd, (void *) (buff+1), size - 1);
		} else {
			SYSLOG(LOG_CRIT,
			       "Invalid fdirectory format, or unknow: 0x%02x",
			       *buff);
		}
		free(buff);
	}
		
	return size;


}


int cr_ls(GC_CONF *gc)
{
	int ret;
	CR_FD *fd = NULL;
	CR_FD *fdp;
	
	if ((ret = cr_get_dir(gc, &fd)) > 0) {
		for(fdp = fd;CR_FD_NEXT(fdp)!=NULL;fdp = CR_FD_NEXT(fdp)) {
			printf("%8d %s %s\n", CR_FD_SIZE(fdp), 
			       CR_FD_DATE(fdp),  CR_FD_NAME(fdp));
		}
		cr_fd_free(&fd);
	}
		
	return ret;
}



/**
   [1] 2.3.4.2 Getting Table Definitions and Table Signatures
**/


int cr_get_tdf(GC_CONF *gc, CR_TDF **tdf)
{
	
	char *buff = NULL;
	size_t size;
	int ret;

	if (*tdf != NULL) cr_tdf_free(tdf);
	*tdf = NULL;
	ret = cr_get_file(gc, ".TDF", &buff, &size);
	if ((int) size > 0) {
		if (*buff == 0x01) {
			cr_tdf_store(tdf, (void *) (buff+1), size - 1);
		} else {
			SYSLOG(LOG_CRIT,
			       "Invalid TDF format, or unknow: 0x%02x", *buff);
			ret = -1;
		}
		free(buff);
	} else {
		ret = -1;
	}
		
	return ret;


}

/**
   [1] 2.3.4.3 Collect Data Transaction (MsgType 0x09 & 0x89)
**/

static PAKBUS *cr_collect_data(GC_CONF *gc, int tblnum, int sig, int mode, 
			       uint32_t p1, uint32_t p2) 
{

	PAKBUS *pk = pak_make(PKL(4+17+8));
	PAKBUS *pki = NULL;
	unsigned char resp_code;
	GC_TRAN_NBR_NEW(gc);
	cr_make_pakbus_header(gc, pk, 2);
	pak_add_byte(pk, 0x09); /* MsgType */
	pak_add_byte(pk, GC_TRAN_NBR(gc)); /* TranNbr */
	pak_add_short(pk, GC_SCODE(gc));
	pak_add_byte(pk, mode);
	pak_add_short(pk, tblnum);
	pak_add_short(pk, sig);
	pak_add_int(pk, p1);
	if(p2 != 0) pak_add_int(pk, p2);
	//pak_add_short(pk, 0);
	pak_add_short(pk, 0);
	PAK_ADD_SIG_NULLIFIER(pk);
	pki = pak_send_and_spect(gc, pk, 0x89);
	pak_free(pk);

	if (pki != NULL) {
		resp_code = pak_read_byte(pki);
		if ( resp_code != 0 ) {
			SYSLOG(LOG_WARNING, 
			       "Invalid resp code: 0x%02x", resp_code);
			pak_free(pki);
			pki = NULL;
		} 
	} else {
		SYSLOG(LOG_WARNING, "pki null (?)");
	}

	return pki;
}


static int pak_fprintf_table(GC_CONF *gc, CR_TDF *tdf, PAKBUS *pki, 
			     time_t *toffset, unsigned char *lastbyte)
{	
	CR_TDF_FIELD *field = NULL;
	int current_index = PAKBUS_READ_INDEX(pki);
	int dummy, i;
	int tblnbr,nor;
	uint32_t frn; 
	int fragment, offset;
	time_t tor, tstamp;
	struct tm *tm_tstamp = NULL;
	char str_tstamp[256];
	uint32_t record;
	FILE *stream = GC_GET_STREAM_OUT(gc);
	
	do {
		tblnbr = pak_read_short(pki);
		record = frn = pak_read_int(pki);
		nor = pak_read_short(pki);
		fragment = nor & 0x8000;
		if(fragment) 
			offset = ((nor & 0x7ff) << 16)| pak_read_short(pki);
		tor = pak_read_int(pki) + TIME_CR2UNIX_TZ(GC_TZ(gc));
		*toffset = tor;
		dummy = pak_read_int(pki); /* read usecs */
		for(i=0; i<nor  ;i++) { 
			/* Time stamp: */
			tstamp = *toffset + i * CR_TDF_INTERVAL(tdf);
			if (GC_TSTAMP_FMT(gc) != NULL) {
				tm_tstamp = gmtime( &tstamp);
				strftime(str_tstamp, sizeof(str_tstamp),
					 GC_TSTAMP_FMT(gc), tm_tstamp);
				fprintf(stream, "%s ", str_tstamp);
			} else {
				fprintf(stream, "%ld ", tstamp);
			}
			/* Record: */
			/* 12/06/2013: Fortmat change from %5d to %10d */
			fprintf(stream, "%10d ", record++);

			for(field=CR_TDF_FIELD_LIST(tdf);
			    field != NULL;field = CR_TDF_FIELD_NEXT(field)) {
				if (CR_TDF_FIELD_DIM_ARRAY(field)[0] == 1 ) {
					switch(CR_TDF_FIELD_TYPE(field)) {
					case type_FP2:
						fprintf(stream, "%8.2f ",
							pak_read_fp2(pki));
						break;
					case type_IEEE4:
						fprintf(stream, "%8.3f ",
							pak_read_float(pki));
						break;
					case type_Int4:
						fprintf(stream, "%8d ",
							pak_read_int(pki));
						break;
					case type_NSec:
						dummy = pak_read_int(pki);
						fprintf(stream, "%8d.%09d ",
							dummy, 
							pak_read_int(pki));
						break;
					case type_Bool4:
						dummy = pak_read_int(pki);
						if(dummy > 0) 
							fprintf(stream, "true ");
						else
							fprintf(stream, "false ");
						break;
					default:
						fprintf(stream, "unknow_%02d ", 
							CR_TDF_FIELD_TYPE(field));
					}
				}
			}
			fprintf(stream, "\n");
		}
	} while (PAKBUS_READ_INDEX(pki) < PAKBUS_LEN(pki) - 3);	
	
	*lastbyte = pak_read_byte(pki);
	CR_TDF_LAST_RECORD_READ(tdf) = record;
	PAKBUS_READ_INDEX(pki) = current_index;

	return 0;
}


void pak_fprintf_table_header(GC_CONF *gc, CR_TDF *tdf)
{
	CR_TDF_FIELD *f = CR_TDF_FIELD_LIST(tdf);
	FILE *fp = GC_GET_STREAM_OUT(gc);

	if (GC_STATION_NAME(gc) != NULL)
		fprintf(fp, "# Station_name: %s\n", GC_STATION_NAME(gc));
	fprintf(fp, "# Table %s, #%d\n# Signull: 0x%04x\n",
		CR_TDF_NAME(tdf), CR_TDF_NUMBER(tdf), CR_TDF_SIGNATURE(tdf));
	fprintf(fp, "# Time_into: %d\n# Time_interval: %d\n", 
		CR_TDF_TIME_INTO(tdf), CR_TDF_INTERVAL(tdf));
	fprintf(fp, "# gcampda_version: %s\n", VERSION);
	fprintf(fp, "#\n# Field definitions:\n");
	if (GC_TSTAMP_FMT(gc) != NULL) {
		fprintf(fp, "# Time_stamp [%s], Record", GC_TSTAMP_FMT(gc));
	} else {
		fprintf(fp, "# Time_stamp [Sec], Record");
	}
	while(f != NULL){
		if ( CR_TDF_FIELD_DIM_ARRAY(f)[0] == 1) {
			fprintf(fp, ", %s [%s]", 
				CR_TDF_FIELD_NAME(f),
				CR_TDF_FIELD_UNITS(f));
		}
		f = CR_TDF_FIELD_NEXT(f);
	}
	fprintf(fp, "\n#\n");		

	return;
}


int cr_get_table(GC_CONF *gc, CR_TDF *tdf, int mode, uint32_t p1, uint32_t p2) 
{

	PAKBUS *pki = NULL;
	int ret = -1;
	int tblnum = CR_TDF_NUMBER(tdf);
	int sig = CR_TDF_SIGNATURE(tdf);
	time_t toffset = 0;
	unsigned char lastbyte;

	pki = cr_collect_data(gc, tblnum, sig, mode, p1, p2); 

	if ( pki != NULL ) {
		pak_fprintf_table(gc, tdf, pki, &toffset, &lastbyte);
		printf("last byte: 0x%02x and record: %d\n", 
		       lastbyte, CR_TDF_LAST_RECORD_READ(tdf));
		pak_free(pki);
		ret = 0;
	}

	return ret;
}


int cr_get_table_from_to(GC_CONF *gc, CR_TDF *tdf, uint32_t from, uint32_t to) 
{

	PAKBUS *pki = NULL;
	int ret = -1;
	int tblnum = CR_TDF_NUMBER(tdf);
	int sig = CR_TDF_SIGNATURE(tdf);
	time_t toffset = 0;
	unsigned char lastbyte;
	
	if(from > to)
		return 0;

	CR_TDF_LAST_RECORD_READ(tdf) = from;
	do {
		if (pki != NULL) {
			pak_free(pki);
			pki = NULL;
		}
		if (CR_TDF_LAST_RECORD_READ(tdf) < to)
			pki = cr_collect_data(gc, tblnum, sig, 0x06, 
					      CR_TDF_LAST_RECORD_READ(tdf), to);
		if ( pki != NULL ) {
			pak_fprintf_table(gc, tdf, pki, &toffset, &lastbyte);
			ret = 0;
		}
	} while(lastbyte && pki != NULL);
	
	if (pki != NULL) {
			pak_free(pki);
	} else {
		ret = -1;
	}

	return ret;
}


int cr_get_table_from(GC_CONF *gc, CR_TDF *tdf, uint32_t from) 
{

	PAKBUS *pki = NULL;
	int ret = -1;
	int tblnbr;
	uint32_t frn;
	int tblnum = CR_TDF_NUMBER(tdf);
	int sig = CR_TDF_SIGNATURE(tdf);

	/* get the most recent record */
	pki = cr_collect_data(gc, tblnum, sig, 0x05, 1, 0); 

	if ( pki != NULL ) {
		tblnbr = pak_read_short(pki);
		frn = pak_read_int(pki) + 1; /* FIXME: 2011/07/27 */
		pak_free(pki);	
		if ( from <= frn ) {
			ret = cr_get_table_from_to(gc, tdf, from, frn); 
		} else {
			ret = 0;
		}
	}

	return ret;
}


/**

   [1] 2.3.5 Get/Set Values Transaction (MsgType 0x1a, 0x9a, 0x1b, &
   0x9b)

**/

void *cr_get_value(GC_CONF *gc, char *tablename, char *fieldname, 
		 unsigned char typecode, int len)
{
	PAKBUS *pk = NULL;

	PAKBUS *pki = NULL;
	unsigned char resp_code;
	void *value = NULL;
	float dummy_float;
	int dummy_int;
	struct timeval *t;
	
	PDBG("Getting table: %s, field: %s, type_code: 0x%x, len: %d\n",
	     tablename, fieldname, typecode, len);
	pk = pak_make(PKL(4+strlen(tablename)+2+strlen(fieldname)+3));
	GC_TRAN_NBR_NEW(gc);
	cr_make_pakbus_header(gc, pk, 2);
	pak_add_byte(pk, 0x1a); /* MsgType */
	pak_add_byte(pk,GC_TRAN_NBR(gc)); /* TranNbr */
	pak_add_short(pk, GC_SCODE(gc));
	pak_add_string(pk, tablename);
	pak_add_byte(pk, typecode);
	pak_add_string(pk, fieldname);
	pak_add_short(pk, len); /* Number of values to get starting with
				 the one specified by fieldname */
	PAK_ADD_SIG_NULLIFIER(pk);
	
	pki = pak_send_and_spect(gc, pk, 0x9a);
	pak_free(pk);
	if (pki == NULL) {
		SYSLOG(LOG_WARNING, "pki null (?)");
		return value;
	}
	resp_code = pak_read_byte(pki);
	if (resp_code == 0 ) {
		switch(typecode) {
		case type_ASCII:
			value = pak_read_string(pki);
			break;
		case type_IEEE4:
			value = (void *) malloc(sizeof(float));
			dummy_float = pak_read_float(pki);
			memcpy(value, &dummy_float, sizeof(float));
			break;
		case type_NSec:
			t = (struct timeval *) malloc(sizeof(struct timeval));
			t->tv_sec = pak_read_int(pki) + 
				TIME_CR2UNIX_TZ(GC_TZ(gc));
			t->tv_usec = pak_read_int(pki);
			value = (void *) t;
			break;
		case type_Int4:
			value = (int32_t *) malloc(sizeof(int32_t));
			dummy_int = pak_read_int(pki);
			memcpy(value, &dummy_int, sizeof(int32_t));
			break;
		default:
			SYSLOG(LOG_CRIT, "Type code 0x%02x not implement", typecode);
			break;
		}
	}
	pak_free(pki);

	return value;
}


static int cr_get_value_int(GC_CONF *gc, char *tablename, char *fieldname)
{
	int *i = (int *) cr_get_value(gc, tablename, fieldname, type_Int4, 0x01);
	int val = -1;

	if (i != NULL) {
		val = *i;
		free(i);
	} else {
		SYSLOG(LOG_ERR,"Error reading \"%s\" value.", fieldname);
	}

	return val;	
}






char *cr_get_station_name(GC_CONF *gc)
{

	return (char *) 
		cr_get_value(gc, "Status","StationName(1)", type_ASCII, 0x40);
}


float cr_get_lithium_battery(GC_CONF *gc)
{
	float *f = (float *) 
		cr_get_value(gc, "Status","LithiumBattery", type_IEEE4, 0x01);
	float val = NAN;

	if (f != NULL) {
		val =  *f;
		free(f);
	}
	return val;	
}


struct timeval *cr_get_start_time(GC_CONF *gc)
{
	return (struct timeval *) 
		cr_get_value(gc, "Status","StartTime", type_NSec, 0x01);
}


int cr_get_low_12v_count(GC_CONF *gc)
{
	return cr_get_value_int(gc, "Status","Low12VCount");
}


int cr_get_low_5v_count(GC_CONF *gc)
{
	return cr_get_value_int(gc, "Status","Low5VCount");
}


int cr_get_skipped_scan(GC_CONF *gc)
{
	return cr_get_value_int(gc, "Status","SkippedScan");
}


int cr_get_skipped_system_scan(GC_CONF *gc)
{
	return cr_get_value_int(gc, "Status","SkippedSystemScan");
}


int cr_get_memory_free(GC_CONF *gc)
{
	return cr_get_value_int(gc, "Status","MemoryFree");
}


int cr_get_memory_size(GC_CONF *gc)
{
	return cr_get_value_int(gc, "Status","MemorySize");
}


int cr_get_watchdog_err(GC_CONF *gc)
{
	return cr_get_value_int(gc, "Status","WatchdogErrors");
}



/**************************************************************************/


void pak_reset(PAKBUS *pk)
{
	memset(PAKBUS_DATA(pk), 0, PAKBUS_H_LEN);
	PAKBUS_STO_LEN(pk) = PAKBUS_H_LEN; 
	PAKBUS_READ_INDEX(pk) = PAKBUS_H_LEN;
	return;
}


int pak_reserve(PAKBUS *pk, int len)
{
	unsigned char *tmp = NULL;
	int factor;
	int new_len = 0;

	if (len > (PAKBUS_LEN(pk))) {
		for(factor = 2; PAKBUS_LEN(pk) * factor < len; factor++);
		new_len = PAKBUS_LEN(pk) * factor;
		tmp = (unsigned char *) malloc(new_len);
		memcpy(tmp, PAKBUS_DATA(pk), PAKBUS_STO_LEN(pk));
		free(PAKBUS_DATA(pk));
		PAKBUS_DATA(pk) = tmp;
		PAKBUS_LEN(pk) = new_len;
	}

	return 0;
}


PAKBUS *pak_make(int len)
{
	
	PAKBUS *pk = (PAKBUS *) malloc(sizeof(PAKBUS));

	if ( pk != NULL) {
		if (len == 0) len = 1024;
		PAKBUS_DATA(pk) = (unsigned char *) malloc(len);
		PAKBUS_LEN(pk) = len;
		pak_reset(pk);
	}
	
	return pk;
}


void pak_free(PAKBUS *pk)
{
	free(PAKBUS_DATA(pk));
	free(pk);

	return;
}
	

void pak_add_bytes(PAKBUS *pk, unsigned char *buff, int buff_len)
{
	int i;

	pak_reserve(pk, PAKBUS_STO_LEN(pk) + buff_len);
	for(i = 0; i < buff_len; i++)
		*(PAKBUS_DATA(pk)+PAKBUS_STO_LEN(pk)++) = buff[i];

	return;
}


void pak_add_byte(PAKBUS *pk, unsigned char byte)
{
	pak_reserve(pk, PAKBUS_STO_LEN(pk) + 1);
	*(PAKBUS_DATA(pk)+PAKBUS_STO_LEN(pk)++) = byte;

	return;
}


void pak_add_short(PAKBUS *pk, uint16_t val)
{
	unsigned char temp[2];
	*(temp)   = (unsigned char)((val & 0xff00) >> 8);
	*(temp+1) = (unsigned char)(val & 0x00ff);
	pak_add_bytes(pk, temp, 2);

	return;
}


void pak_add_int(PAKBUS *pk, uint32_t val)
{
	unsigned char temp[4];
	*(temp)   = (unsigned char)((val & 0xff000000) >> 24);
	*(temp+1) = (unsigned char)((val & 0x00ff0000) >> 16);
	*(temp+2) = (unsigned char)((val & 0x0000ff00) >> 8);
	*(temp+3) = (unsigned char)(val & 0x000000ff);
	pak_add_bytes(pk, temp, 4);

	return;
}


void pak_add_string(PAKBUS *pk, char *str)
{
	int len = strlen((char *) str);
	pak_add_bytes(pk, (unsigned char *) str,len);
	if( *(str+len-1) != '\0')
		pak_add_byte(pk, 0);

	return;
}


void pak_add_float(PAKBUS *pk, float f)
{	
	FLOAT_UINT32 d;
	
	d.f = f;	
	pak_add_int(pk, d.i); 

	return;
}


unsigned char *pak_read_bytes(PAKBUS *pk, int len) /* return asigned data */
{
	unsigned char *buff = NULL;

	if(PAKBUS_READ_INDEX(pk) + len > PAKBUS_LEN(pk)) {
		SYSLOG(LOG_WARNING,"Attempt to read past end");
		SYSLOG(LOG_WARNING, "index: %d, len: %d, pk_len: %d",
		       PAKBUS_READ_INDEX(pk), len, PAKBUS_LEN(pk));
		return NULL;
	}
	buff = (unsigned char *) malloc(len);
	memcpy(buff, PAKBUS_DATA(pk)+ PAKBUS_READ_INDEX(pk), len);
	PAKBUS_READ_INDEX(pk) += len;

	return buff;
}


#if 0 /* Now is a macro definition */
unsigned char pak_read_byte(PAKBUS *pk)
{
	return *(PAKBUS_DATA(pk)+ PAKBUS_READ_INDEX(pk)++);
}
#endif 


uint16_t pak_read_short(PAKBUS *pk)
{
	uint16_t val = ((uint16_t) pak_read_byte(pk) & 0xff) << 8;

	val |= (uint16_t) pak_read_byte(pk) & 0xff;

	return val;
}


int32_t pak_read_int(PAKBUS *pk)
{
	int32_t val = -1;
	unsigned char *tmp = pak_read_bytes(pk, 4);

	if ( tmp != NULL) {
		val = (((int32_t) *(tmp)   & 0xff) << 24) |
			(((int32_t) *(tmp+1) & 0xff) << 16) |
			(((int32_t) *(tmp+2) & 0xff) << 8) |
			((int32_t) *(tmp+3) & 0xff);
		free(tmp);
	} else {
		SYSLOG(LOG_CRIT, "Read int32 fail.");
	}
	
	return val;
}


float pak_read_float(PAKBUS *pk)
{
	FLOAT_UINT32 d;

	d.i =  pak_read_int(pk);
	
	return d.f;
}


unsigned char *pak_read_string(PAKBUS *pk) /* return asigned data */
{
	int i, len;
	unsigned char *buff = NULL;

	for(i=PAKBUS_READ_INDEX(pk); 
	    *(PAKBUS_DATA(pk)+i) != 0 && i < PAKBUS_LEN(pk); i++);
	len = i - PAKBUS_READ_INDEX(pk) + 1;
	buff = (unsigned char *) malloc(len);
	memcpy(buff, PAKBUS_DATA(pk)+PAKBUS_READ_INDEX(pk), len);
	PAKBUS_READ_INDEX(pk) += len;

	return buff;
}


/*                         0     1      2       3    */
static float _powf_10[] = {1.0, 10.0, 100.0, 1000.0};
/*
0x9ffe => nan
 */
float fp2_to_float( uint16_t fp2)
{
	if ( fp2 == 0x9ffe)
		return NAN;
	return ((fp2 >> 15)?-(float) (fp2 & 0x1fff):(float) (fp2 & 0x1fff)) / 
		_powf_10[(fp2 >> 13) & 0x03];
}


float pak_read_fp2(PAKBUS *pk)
{
	return fp2_to_float( (uint16_t) pak_read_short(pk));
}

