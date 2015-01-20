/** 
 *
 *   Library for PAKBUS communication
 *
 *   (c) Copyright 2010 Federico Bareilles <bareilles@gmail.com>,
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 3 of
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
 *
 **/

#ifndef _PAKBUS_H
#define _PAKBUS_H


#include <stdint.h> /* for uintXX_t definition */
#include <stdlib.h>
#include <sys/time.h>

#if !defined(__APPLE__) && !defined(WIN32)
# include <endian.h>
#elif defined(__APPLE__)
# include <machine/endian.h>
#else
# include <windows.h>
# ifndef BIG_ENDIAN
#  define BIG_ENDIAN 4321
# endif
# ifndef LITTLE_ENDIAN
#  define LITTLE_ENDIAN 1234
# endif
# ifndef BYTE_ORDER
/* if intel x86 or alpha proc, little endian */
#  if defined(_M_IX86) || defined(_M_ALPHA)
#   define BYTE_ORDER LITTLE_ENDIAN
#  endif
/* if power pc or MIPS RX000, big endian */
#  if defined(_MPPC) || defined(_M_MX000)
#   define BYTE_ORDER BIG_ENDIAN
#  endif
# endif
#endif /* WIN32 */

#include "gcampda.h"

#define SERSYNCBYTE 0xbd
#define QUOTEBYTE   0xbc
#define INITSED     0xaaaa


#define SWAP_BYTE(b) b = (((b << 8) & 0xff00) | ((b >> 8) & 0x00ff))
#define CHAR2INT32(c) (((int32_t) *(c) & 0xff) << 24) |	\
	(((int32_t) *(c+1) & 0xff) << 16) |			\
	(((int32_t) *(c+2) & 0xff) << 8) |			\
	((int32_t) *(c+3) & 0xff)
#define VOID2INT32(b) CHAR2INT32((unsigned char *) b)

/* date --date='1990-01-01' +%s -u */ /* - (TZ * 3600) */
#define TIME_CR2UNIX_UTC 631152000
#define TIME_CR2UNIX_TZ(t) (TIME_CR2UNIX_UTC - (t * 3600))

/* [1] 2.1: SerPkt Link Sub Protocol */

typedef struct pakbus_head {
#if BYTE_ORDER == BIG_ENDIAN
        /* FIXME: Warning, BIG_ENDIAN not implement. */
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
        uint32_t dst_phy_addr_h:4;
        uint32_t link_state:4;
/* --- */   
        uint32_t dst_phy_addr_l:8;
/* --- */   
	uint32_t src_phy_addr_h:4;
        uint32_t priority:2;
        uint32_t exp_more_code:2;
/* --- */   
	uint32_t src_phy_addr_l:8;
#endif
} PAKBUSH;

#define PB_OBJECT(a) ((PAKBUSH *) a)
#define PB_GET_DST_ADDR(a) (PB_OBJECT(a)->dst_phy_addr_h << 8|	\
			    PB_OBJECT(a)->dst_phy_addr_l)
#define PB_SET_DST_ADDR(a, b) PB_OBJECT(a)->dst_phy_addr_h = (b >> 8); \
	PB_OBJECT(a)->dst_phy_addr_l = b & 0xff 
#define PB_GET_LINK_STATE(a) (PB_OBJECT(a)->link_state)
#define PB_SET_LINK_STATE(a, b) (PB_GET_LINK_STATE(a) = b)
#define PB_GET_PRIORITY(a) (PB_OBJECT(a)->priority)
#define PB_SET_PRIORITY(a, b) (PB_GET_PRIORITY(a) = b)
#define PB_GET_EXP_MORE_CODE(a) (PB_OBJECT(a)->exp_more_code)
#define PB_SET_EXP_MORE_CODE(a, b) (PB_GET_EXP_MORE_CODE(a) = b)
#define PB_GET_SRC_ADDR(a) (PB_OBJECT(a)->src_phy_addr_h << 8|	\
			    PB_OBJECT(a)->src_phy_addr_l)
#define PB_SET_SRC_ADDR(a, b) PB_OBJECT(a)->src_phy_addr_h = (b >> 8); \
	PB_OBJECT(a)->src_phy_addr_l = b & 0xff 


typedef struct pakbus_head_hip {
#if BYTE_ORDER == BIG_ENDIAN
        /* FIXME: Warning, BIG_ENDIAN not implement. */
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
        uint32_t dst_node_id_h:4;
        uint32_t hi_proto_code:4;
        uint32_t dst_node_id_l:8;

	uint32_t src_node_id_h:4;
        uint32_t hop_cnt:4;
        uint32_t src_node_id_l:8;
#endif
} PAKBUSH_HIP;

#define PAKBUSH_HIP_OFFSET sizeof(PAKBUSH)
#define PB_HIP_OBJECT(a) ((PAKBUSH_HIP *) (a + PAKBUSH_HIP_OFFSET))

#define PB_GET_DST_NODE_ID(a) (PB_HIP_OBJECT(a)->dst_node_id_h << 8|	\
				   PB_HIP_OBJECT(a)->dst_node_id_l)
#define PB_SET_DST_NODE_ID(a, b) PB_HIP_OBJECT(a)->dst_node_id_h = (b >> 8); \
				     PB_HIP_OBJECT(a)->dst_node_id_l = b & 0xff 
#define PB_GET_SRC_NODE_ID(a) (PB_HIP_OBJECT(a)->src_node_id_h << 8|PB_HIP_OBJECT(a)->src_node_id_l)
#define PB_SET_SRC_NODE_ID(a, b) PB_HIP_OBJECT(a)->src_node_id_h = (b >> 8);	\
	PB_HIP_OBJECT(a)->src_node_id_l = b & 0xff 
#define PB_GET_HI_PROTO_CODE(a) (PB_HIP_OBJECT(a)->hi_proto_code)
#define PB_SET_HI_PROTO_CODE(a, b) (PB_GET_HI_PROTO_CODE(a) = b)
#define PB_GET_HOP_CNT(a) (PB_HIP_OBJECT(a)->hop_cnt)
#define PB_SET_HOP_CNT(a, b) (PB_GET_HOP_CNT(a) = b)

#define PB_SET_SIG_NULLIFIER(a, len) {		\
		int __sed =						\
			calc_sig_nullifier(calc_sig_for(a, len - 2, 0xaaaa)); \
		*(a+len-2) = (__sed >> 8) & 0xff;			\
		*(a+len-1) = __sed & 0xff;				\
	}

#define LS_OFF_LINE 0x08
#define LS_RING     0x09
#define LS_READY    0x0a
#define LS_FINISHED 0x0b
#define LS_PAUSE    0x0c

#define EMC_LAST        0x0
#define EMC_EXPECT_MORE 0x1
#define EMC_NEUTRAL     0x2
#define EMC_REVERSE     0x3

typedef struct _pakbus_in_out {
	unsigned char *data;
	int len; /* data buffer length */
	int storage_len; /* storage length of data seting */
	int read_index; /* index for input packets */
} PAKBUS;

#define PAKBUS_H_LEN (sizeof(PAKBUSH)+sizeof(PAKBUSH_HIP))
#define PAKBUS_LEN(p) (p)->len 
#define PAKBUS_STO_LEN(p) (p)->storage_len 
#define PAKBUS_DATA(p) (p)->data 
#define PAKBUS_READ_INDEX(p) (p)->read_index 

#define PAK_ADD_SIG_NULLIFIER(p) {					\
		int __sed =						\
			calc_sig_nullifier(				\
				calc_sig_for(PAKBUS_DATA(p),		\
					     PAKBUS_STO_LEN(p), 0xaaaa)); \
		*(PAKBUS_DATA(p)+PAKBUS_STO_LEN(p)++) = (__sed >> 8) & 0xff; \
		*(PAKBUS_DATA(p)+PAKBUS_STO_LEN(p)++) = __sed & 0xff;	\
	}
#define PAKBUS_GET_LINK_STATE(p) PB_GET_LINK_STATE(PAKBUS_DATA(p)) 

int pakctrl(GC_CONF *gc, char linkstate, int addr_to, char expmorecode, 
	    char priority);
PAKBUS *pak_make(int len);
void pak_reset(PAKBUS *pk);
int pak_reserve(PAKBUS *pk, int len);
void pak_free(PAKBUS *pk);
void pak_add_bytes(PAKBUS *pk, unsigned char *buff, int buff_len);
void pak_add_byte(PAKBUS *pk, unsigned char byte);
void pak_add_short(PAKBUS *pk, uint16_t val);
void pak_add_int(PAKBUS *pk, uint32_t val);
void pak_add_string(PAKBUS *pk, char *str);
void pak_add_float(PAKBUS *pk, float f);

unsigned char *pak_read_bytes(PAKBUS *pk, int len); /* return asigned data */
//unsigned char pak_read_byte(PAKBUS *pk);
#define pak_read_byte(p) ((unsigned char)				\
			  *(PAKBUS_DATA(p)+ PAKBUS_READ_INDEX(p)++))
			  
uint16_t pak_read_short(PAKBUS *pk);
int32_t pak_read_int(PAKBUS *pk);
float pak_read_float(PAKBUS *pk);
unsigned char *pak_read_string(PAKBUS *pk); /* return asigned data */
float pak_read_fp2(PAKBUS *pk);


#define L_UByte     1  /* One-byte unsigned integer */
#define L_UInt2     2  /* Two-byte unsigned integer (MSB first) */
#define L_UInt4     4  /* Four-byte unsigned integer (MSB first) */
#define L_Int1      1  /* One-byte signed integer */
#define L_Int2      2  /* Two-byte signed Integer (MSB first) */
#define L_In4       4  /* Four-byte signed Integer (MSB first) */
#define L_FP2       2  /* Two-byte final storage floating point */
#define L_FP3       3  /* Three-byte final storage floating point */
#define L_FP4       4  /* Four-byte final storage floating point (CSI Format) */
#define L_IEEE4B    4  /* Four-byte floating point (IEEE standard, MSB first) */
#define L_IEEE8B    8  /* Eight-byte floating point(IEEE standard, MSB first) */
#define L_Bool8     1  /* Byte of flags */
#define L_Bool      1  /* Boolean value */
#define L_Bool2     2  /* Boolean value */
#define L_Bool4     4  /* Boolean value */
#define L_Sec       4  /* Four-byte integer used for one second resolution time (MSB First) */
#define L_USec      6  /* Six-byte unsigned integer, 10's of milliseconds resolution (MSB first) */
#define L_NSec      8  /* Two four-byte integers, nanosecond time resolution (MSB first) */
//#define L_ASCII     n /* A fixed length string formed by using an array of ASCII characters. The unused portion of the fixed length should be padded with NULL characters or spaces. */
//#define L_ASCIIZ   n+1 /* A variable length string formed by an array of ASCII characters and terminated with a NULL character. */
#define L_Short     2  /* Two-byte integer (LSB first) */
#define L_Long      4  /* Four-byte integer (LSB first) */
#define L_UShort    2  /* Two-byte unsigned integer (LSB first) */
#define L_ULong     4  /* Four-byte unsigned integer (LSB first) */
#define L_IEEE4L    4  /* Four-byte floating point (IEEE format, LSB first) */
#define L_IEEE8L    8  /* Eight-byte floating point (IEEE format, LSB first) */
#define L_SecNano   8  /* Two Longs (LSB first), seconds, then nanoseconds */

#define PKL_17 (BMP5H_OFFSET + sizeof(BMP5H) + L_Nsec + 2)
#define PKL(t) (PAKBUS_H_LEN + t + 2)

#define type_UByte 1
#define type_UInt2 2
#define type_UInt4 3
#define type_Byte 4
#define type_Int2 5
#define type_Int4 6
#define type_FP2 7
#define type_IEEE4 9
#define type_Sec 12
#define type_USec 13
#define type_NSec 14
#define type_ASCII 11
#define type_Int2_lsf 19
#define type_Int4_lsf 20
#define type_UInt2_lsf 21
#define type_UInt4_lsf 22
#define type_NSec_lsf 23
#define type_IEEE4_lsf 24
#define type_Bool4 28


/**
 * [1] 2.3.3.3 File Directory Format
 **/
typedef struct _file_directory {
	char *fname;
	char *date;
	uint32_t size;
	unsigned char attr[12];
	struct _file_directory *next;
} CR_FD;


#define CR_FD_NEXT(d) (d)->next
#define CR_FD_ATTR(d) (d)->attr
#define CR_FD_SIZE(d) (d)->size
#define CR_FD_DATE(d) (d)->date
#define CR_FD_NAME(d) (d)->fname
#define CR_FD_ATTR(d) (d)->attr

int cr_fd_store(CR_FD **d, void *buff, size_t buff_size);
void cr_fd_free(CR_FD **cr_fd);


/**
 * [1] 2.3.4.2 Getting Table Definitions and Table Signatures
 **/

typedef struct _cr_tdf_field {
	int field_number;
	unsigned char read_only;
	unsigned char field_type;
	char *field_name;
	char *alias_name;
	char *procesing;
	char *units;
	char *description;
	uint32_t begin_index;
	uint32_t *dimension;
	uint32_t dim_num;
	struct _cr_tdf_field *next;
} CR_TDF_FIELD;

#define CR_TDF_FIELD_NUMBER(f) (f)->field_number
#define CR_TDF_FIELD_RO(f) (f)->read_only
#define CR_TDF_FIELD_TYPE(f) (f)->field_type
#define CR_TDF_FIELD_NAME(f) (f)->field_name
#define CR_TDF_FIELD_ALIAS(f) (f)->alias_name /* unused */
#define CR_TDF_FIELD_PROCESING(f) (f)->procesing
#define CR_TDF_FIELD_UNITS(f) (f)->units
#define CR_TDF_FIELD_DESCRIP(f) (f)->description
#define CR_TDF_FIELD_BEGIN_INDEX(f) (f)->begin_index
#define CR_TDF_FIELD_DIM_ARRAY(f) (f)->dimension
#define CR_TDF_FIELD_DIM_NUM(f) (f)->dim_num
#define CR_TDF_FIELD_NEXT(f) (f)->next

typedef struct _cr_tdf_table {
	int table_number;
	int table_def_signature;
	char *tname;
	uint32_t table_size;
	unsigned char time_type;
	uint32_t time_into;
	uint32_t interval;
	CR_TDF_FIELD *field;

	uint32_t last_record_read;
	struct _cr_tdf_table *next;
} CR_TDF;

#define CR_TDF_NUMBER(t) (t)->table_number
#define CR_TDF_SIGNATURE(t) (t)->table_def_signature
#define CR_TDF_NAME(t) (t)->tname
#define CR_TDF_NR(t) (t)->table_size
#define CR_TDF_TIME_TYPE(t) (t)->time_type
#define CR_TDF_TIME_INTO(t) (t)->time_into
#define CR_TDF_INTERVAL(t) (t)->interval
#define CR_TDF_FIELD_LIST(t) (t)->field
#define CR_TDF_LAST_RECORD_READ(t) (t)->last_record_read
#define CR_TDF_NEXT(t) (t)->next

int cr_tdf_store(CR_TDF **d, void *buff, size_t buff_size);
void cr_tdf_free(CR_TDF **cr_tdf);
void cr_tdf_list(CR_TDF *t, int field_flag);
CR_TDF *cr_tdf_find_table(CR_TDF *t, char *table_name);
void cr_tdf2dl_conf(CR_TDF *t, DL_CONF *d);
int gcampda_tdf2dl_conf(CR_TDF *tdf, DL_CONF **dlc);

uint16_t calc_sig_for(void const *buff, uint32_t len, uint16_t seed);
uint16_t calc_sig_nullifier(uint16_t sig);

int cr_hello_transaction_request(GC_CONF *gc);
int cr_send_0xbd(GC_CONF *gc, int num);
#define cr_wake_up(gc) cr_send_0xbd(gc, 4)
int cr_at(GC_CONF *gc);
int cr_ready(GC_CONF *gc);
int cr_clock_set(GC_CONF *gc, struct timeval *t);
struct timeval *cr_clock_get(GC_CONF *gc);
int cr_get_file(GC_CONF *gc, char *fname, char **file_buff, size_t *buff_size_out);
int cr_get_dir(GC_CONF *gc, CR_FD **fd);
int cr_ls(GC_CONF *gc); /* for testing */
int cr_get_tdf(GC_CONF *gc, CR_TDF **tdf);
int cr_get_table(GC_CONF *gc, CR_TDF *tdf, int mode, uint32_t p1, uint32_t p2);
int cr_get_table_from_to(GC_CONF *gc, CR_TDF *tdf, uint32_t from, uint32_t to);
int cr_get_table_from(GC_CONF *gc, CR_TDF *tdf, uint32_t from);
void *cr_get_value(GC_CONF *gc, char *tablename, char *fieldname, 
	     unsigned char typecode, int len);
char *cr_get_station_name(GC_CONF *gc);
float cr_get_lithium_battery(GC_CONF *gc);
struct timeval *cr_get_start_time(GC_CONF *gc);
int cr_get_low_12v_count(GC_CONF *gc);
int cr_get_low_5v_count(GC_CONF *gc);
int cr_get_skipped_scan(GC_CONF *gc);
int cr_get_skipped_system_scan(GC_CONF *gc);
int cr_get_memory_free(GC_CONF *gc);
int cr_get_memory_size(GC_CONF *gc);
int cr_get_watchdog_err(GC_CONF *gc);

void pak_fprintf_table_header(GC_CONF *gc, CR_TDF *tdf);

PAKBUS *pak_send_and_spect(GC_CONF *gc, PAKBUS *pk, unsigned char type_spect);
PAKBUS *pak_spect(GC_CONF *gc, unsigned char type);
int pak_rcv(int dev,uint16_t soft_addr, PAKBUS **pak);

float fp2_to_float( uint16_t fp2);
#define fp2_to_double(fp2) ((double) fp2_to_float(fp2))
#ifdef DEBUG_MODE
void hexdump(char prefix, unsigned char *buff, int size);
#endif

typedef union _float_uint32_t {
	uint32_t i;
	float f;
} FLOAT_UINT32;

#endif  /* ifndef _PAKBUS_H */
