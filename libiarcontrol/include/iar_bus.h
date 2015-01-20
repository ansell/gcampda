/** 
 *   iar_bus: IPC multi-platform on virtual BUS, based on socket.  
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

#ifndef _IB_H
#define _IB_H

#include <stdint.h> /* for uintXX_t definition */

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

#include "ib_fd.h"

#define IB_PACKET_LEN 1496

/**
 * Address for struct ib_packet_head (BIG_ENDIAN):
 *
 *     |           +           |           +           |
 *     | 8  4  2  1| 8  4  2  1| 8  4  2  1| 8  4  2  1|
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *...  |15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |-----------  size  -------------|--- unused ---|
 *
 *  |           +           |           +           |
 *  | 8  4  2  1| 8  4  2  1| 8  4  2  1| 8  4  2  1|
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16| ...
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |--+------------ addr --------------|--- cmd ---|
 *
 *
 * About fixed or static address:
 *  +           |           +           |
 *  | 8  4  2  1| 8  4  2  1| 8  4  2  1|
 *  +--+--+--+--+--+--+--+--+--+--+--+--+
 *  |11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
 *  +--+--+--+--+--+--+--+--+--+--+--+--+
 *  |------------- Address -------------|
 *  |--|--|-- Base ---|------ Sub ------|
 *   |  |
 *   |  |
 *   |  +-> Dynamic address flag (always 0).
 *   +-> Multi Cast Flag.
 * 
 * If Multi Cast Flag is on (and Dynamic address flag off) the message
 * is sent to all "Sub Address" which "Base Address"
 *
 * If Multi and Dynamic flag is on, the message is send to all
 * address matching which destination address (address_to):
 * (address & address_to) ==  address ?
 *
 * If Multi Cast list is set, and destination address is equal to
 * IB_MULTICAST_FLAG,the message is send to Multi Cast List; Address
 * and Sub-address are ignored in this case.
 *
 */ 

/* Address definition: */
#define IB_NULL_ADDR      0x000 
#define IB_BUS_ADDR       0x400 /* 0x000 to 0x3ff: fixed address */
#define IB_FIX_MASK_ADDR  0x3c0 /* Fix Address range: 0 - 15 */
#define IB_FIX_MASK_SADDR 0x03f /* Sub address range: 0 - 63 */
#define IB_DYN_MASK_ADDR  0x7ff /* 0x401 to 0x7ff: dynamic range address */
#define IB_DYN_ADDR_FLAG  0x400
#define IB_MULTICAST_FLAG 0x800
#define IB_BROADCAST_ADDR 0xfff /* special case of multicast */

#define IB_IS_VALID_FIX_ADDR(a) (					\
		((a & (IB_FIX_MASK_ADDR|IB_FIX_MASK_SADDR)) == a) &&	\
		(a != IB_BUS_ADDR) &&					\
		(a != IB_NULL_ADDR) ? 1 : 0) 

#define IB_GET_BASE_ADDR(a) (a & IB_DYN_ADDR_FLAG ? a : (a & IB_FIX_MASK_ADDR))
#define IB_GET_SUB_ADDR(a) (a & IB_FIX_MASK_SADDR)


/* Commands definitions for struct ib_packet_head {.cmd} and struct
 * ib_command {.cm_id} in ib_dev.h */
/* Range (4 bits): 0x0 - 0xf */
#define IB_CM_NULL       0x0 /* Not real command */
#define IB_CM_REMOVE     0x1
#define IB_CM_MESSAGE    0x2
#define IB_CM_SET_ADDR   0x3
#define IB_CM_GET_ADDR   0x4
#define IB_CM_SET_STATUS 0x5
#define IB_CM_GET_STATUS 0x6
#define IB_CM_MCAST_LIST 0x7
#define IB_CM_WHO        0x8
#define IB_CM_SHUTDOWN   0xf
/* Fosiles: */
//#define IB_CM_MESSAGE_TO 0x
//#define IB_CM_RW         0x
//#define IB_CM_BCAST_ON   0x
//#define IB_CM_BCAST_OFF  0x

struct ib_packet_head {
#if BYTE_ORDER == BIG_ENDIAN
	/* FIXME: Warning, BIG_ENDIAN not tested. */
	uint32_t unused:5;
	uint32_t size:11;
	uint32_t cmd:4;
	uint32_t addr:12; 
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
	uint32_t addr:12;  /* From the bus side: address To From the
			     device side: address from */ 
	uint32_t cmd:4;
	uint32_t size:11;
	uint32_t unused:5;
#endif
};


#define IB_PACKET_DATA_LEN ((int) IB_PACKET_LEN - \
			    (int) sizeof(struct ib_packet_head))


typedef struct ib_packet {
	struct ib_packet_head h;
	char data[IB_PACKET_DATA_LEN]; 
} IB_PK;

#define IB_PK_SIZE(d) (d)->h.size
#define IB_PK_CMD(d) (d)->h.cmd
#define IB_PK_ADDR(d) (d)->h.addr
#define IB_PK_DATA(d) (d)->data
#define IB_PK_SIZE_IF_VALID(d) (IB_PK_SIZE(d) < IB_PACKET_DATA_LEN?1:0)

typedef struct _devs {
	uint32_t addr;
	uint32_t status;
} DEVS;

#define DEVS_ADDR(d) (d)->addr
#define DEVS_STATUS(d) (d)->status

typedef struct _addr { /* Null terminated list of multicast target */
	uint16_t a;
} ADDR;

#define ADDR_GET(d) (d)->a
#define ADDR_LIMIT (IB_PACKET_DATA_LEN/sizeof(ADDR))

int iar_bus_start(char *sock_name);
int iar_bus_inet_start( int port );
int iar_bus_set_shutdown(int state); /* 0: disable, 1: enable */

#ifndef WIN32
IBFD *ibus_client_connect_local(char *name);
#endif
IBFD *ibus_client_connect(char *hostname, int port);
int ibus_client_close(IBFD *dc);

int ibus_read(IBFD *dc, IB_PK *d);
int ibus_read_data( IBFD *dc, uint32_t *addr_from, void *data, int *len);
int ibus_bcast_on( IBFD *dc );
int ibus_bcast_off( IBFD *dc );
/* ibus_rw(): new_state is defined in ib_dev.h */
int ibus_rw( IBFD *dc, int new_state );
int ibus_remove( IBFD *dc );
int ibus_set_addr(IBFD *dc, int addr);
int ibus_get_addr(IBFD *dc, uint32_t *addr);
/* ibus_send_data: never fail, the message is send to all clients,
 * exept the sender.*/
int ibus_send_data( IBFD *dc, void *data, int len);
/* ibus_send_data_to: Fail if addr not exist. If addr if broadcast,
 * the message is sent to all clients excluding sender, and never
 * fail. */ 
int ibus_send_data_to_wflag( IBFD *dc, int addr, void *data, int len, 
		       int wait_ret_flag);
#define ibus_send_data_to(dc, addr, data, len) \
	ibus_send_data_to_wflag(dc, addr, data, len, 1)
#define ibus_send_data_to_sinc(dc, addr, data, len) \
	ibus_send_data_to_wflag(dc, addr, data, len, 1)
#define ibus_send_data_to_asinc(dc, addr, data, len) \
	ibus_send_data_to_wflag(dc, addr, data, len, 0)

int ibus_shutdown( IBFD *dc );
int ibus_wait_packet(IBFD *dc, long time_out_sec, long time_out_usec);
#define IBUS_IF_PACKET(dc) ibus_wait_packet(dc, 0, 0)
int ibus_who(IBFD *dc, DEVS **dev_list);
int ibus_set_mcast_list(IBFD *dc, ADDR *list);
int ibus_remove_mcast_list(IBFD *dc);
int ibus_set_msg_confirmation(IBFD *dc, int on_off);


#define f_ib_pk_print(stream, d) {					\
		fprintf(stream,						\
			"cmd: 0x%x arrived from: 0x%03x, size: %d",	\
			IB_PK_CMD(&d), IB_PK_ADDR(&d), IB_PK_SIZE(&d));	\
		if (IB_PK_SIZE(&d) ) {					\
			int _i;						\
			fprintf(stream,	" - data:");			\
			for(_i = 0; _i < IB_PK_SIZE(&d); _i++) {	\
				if ( (_i % 16) == 0 )			\
					fprintf(stream, "\n%02x: ",	\
						_i % 16);		\
				if ( _i && (_i % 8) == 0 )		\
					fprintf(stream, "| ");		\
				fprintf(stream,"%02x ",			\
					*(IB_PK_DATA(&d)+_i));		\
			}						\
		}							\
		fprintf(stream, "\n");					\
		}

#endif  /* ifndef _IB_H */




