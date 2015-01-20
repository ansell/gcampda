/** 
 *   iar_bus_client: IPC multi-platform on virtual BUS, based on
 *   socket. Client side implementation.
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
 *
 *   27/12/2004: fix return val in ibus_read().
 *               add select() to write in ibus_send().
 *   01/07/2005: fix return val in: ibus_bcast_on(), ibus_bcast_off(), ibus_rw(),
 *               ibus_remove(), ibus_set_addr(), ibus_shutdown().
 **/

#define _GNU_SOURCE


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "iar_bus.h"
#include "ib_dev.h"
#include "socket_client.h"



/******************************************************************************
 * Functions for de divece side 
 */

static int ibus_send( IBFD *dc, int addr_to, int action, void *data, int len)
{
	struct ib_packet_head ph;
	fd_set wfds;
	struct timeval tv;
	int fd_max;
	int retval;

	ph.addr = addr_to;
	ph.cmd = action;
	ph.size = len;
	fd_max = IBFD_GET(dc) + 1;
	do {
	        FD_ZERO(&wfds);
		FD_SET(IBFD_GET(dc), &wfds);
		tv.tv_sec = 5; /* FIXME: hay que hacerlo configurable */ 
		tv.tv_usec = 0;
		retval = select(fd_max, NULL, &wfds, NULL, &tv);
	}while (retval == 0 && !FD_ISSET(IBFD_GET(dc), &wfds));

	if (retval == 0) {
		return -1;
	}
	if ( sock_write(IBFD_GET(dc), &ph, sizeof(struct ib_packet_head)) < 0 ) {
		return -1;
	}
	if ( len > 0 ){
		if (sock_write(IBFD_GET(dc), data, len) < 0 )
			return -1;
	}

	return 0;
}


static int ibus_exec_cmd(IBFD *dc, int cmd, void *data_in_out, 
			 int len_in, int len_out)
{
	IB_PK d;
	int ret = ibus_send( dc, IB_BUS_ADDR, cmd, data_in_out, len_in);
	
	if ( ret < 0 )
		return ret;

	do {
		if (ibus_read(dc, &d) == 0) {
			return -1;
		}
	} while(IB_PK_CMD(&d) != cmd || IB_PK_ADDR(&d) != IB_BUS_ADDR);

	if ( IB_PK_SIZE(&d) && len_out ) {
		ret = (IB_PK_SIZE(&d) > len_out ? len_out : IB_PK_SIZE(&d));
		memcpy(data_in_out, IB_PK_DATA(&d), ret);
	}

	return ret;
}


int ibus_send_data( IBFD *dc, void *data, int len)
{
	return ibus_send( dc, IB_BROADCAST_ADDR, IB_CM_MESSAGE, data, len);
}


int ibus_send_data_to_wflag( IBFD *dc, int addr, void *data, int len, 
			     int wait_ret_flag)
{
	IB_PK d;
	int ret = ibus_send( dc, addr, IB_CM_MESSAGE, data, len);

	if ( ret < 0 )
		return ret;

	if ( addr == IB_BROADCAST_ADDR )
		return ret;

	/* if wait_ret_flag is 0: response is send anyway not
	   synchronously to the client */
	if ( wait_ret_flag && IBFD_IF_CON_MSG(dc)) { 
		do {
			if (ibus_read(dc, &d) == 0) {
				return -1;
			}
		} while(IB_PK_CMD(&d) != IB_CM_MESSAGE || 
			IB_PK_ADDR(&d) != IB_BUS_ADDR);
		ret = -1;
		if ( IB_PK_SIZE(&d) ) {
			uint32_t _val;
			__builtin_memcpy(&_val, IB_PK_DATA(&d), 
					 sizeof(uint32_t));
			//if ( *((uint32_t *) IB_PK_DATA(&d)) == 0)
			if ( _val == 0)
				ret = 0;
		}
	}

	return ret;
}


int ibus_read(IBFD *dc, IB_PK *d)
{
	int ret;

	ret = sock_read(IBFD_GET(dc), d, sizeof(struct ib_packet_head) );
	if( IB_PK_SIZE(d) ){
		ret += sock_read(IBFD_GET(dc), IB_PK_DATA(d), IB_PK_SIZE(d));
	}

	return ret;
}


int ibus_read_data( IBFD *dc, uint32_t *addr_from, void *data, int *len)
{
	struct ib_packet_head d;
	int ret = -1;

	if ( sock_read(IBFD_GET(dc), &d, sizeof(struct ib_packet_head)) > 0) {
		*len = d.size;
		if( d.size ){
			ret = sock_read(IBFD_GET(dc), data, d.size );
		}
		if( addr_from != NULL ){
			*addr_from = d.addr;
		}
	}

	return ret;
}


int ibus_set_addr(IBFD *dc, int addr)
{

	ibus_exec_cmd(dc, IB_CM_SET_ADDR, &addr, sizeof(int), sizeof(int));

	return addr;
}


int ibus_get_addr(IBFD *dc, uint32_t *addr)
{
	ibus_exec_cmd(dc, IB_CM_GET_ADDR, addr, 0, sizeof(uint32_t));

	return *addr;
}


#if 0
int ibus_bcast_on( IBFD *dc )
{
	return ibus_exec_cmd(dc, IB_CM_BCAST_ON, 0, 0, 0);
}


int ibus_bcast_off( IBFD *dc )
{
	return ibus_exec_cmd(dc, IB_CM_BCAST_OFF, 0, 0, 0);
}
#endif


int ibus_bcast_on( IBFD *dc )
{
	uint32_t current;

	ibus_exec_cmd(dc, IB_CM_GET_STATUS, &current, 0, sizeof(uint32_t));
	current &= IB_DEV_MASK;
	current |= IB_DEV_BCAST_ON;
	return ibus_exec_cmd(dc, IB_CM_SET_STATUS, &current, 
			     sizeof(uint32_t), sizeof(uint32_t));
}


int ibus_bcast_off( IBFD *dc )
{
	uint32_t current;

	ibus_exec_cmd(dc, IB_CM_GET_STATUS, &current, 0, sizeof(uint32_t));
	current &= IB_DEV_MASK;
	current &= ~IB_DEV_BCAST_ON;
	return ibus_exec_cmd(dc, IB_CM_SET_STATUS, &current, 
			     sizeof(uint32_t), sizeof(uint32_t));
}




int ibus_remove( IBFD *dc )
{
	return ibus_exec_cmd(dc, IB_CM_REMOVE, 0, 0, 0);
}

#if 0
int ibus_rw(IBFD *dc, int new_state)
{
	return ibus_exec_cmd(dc, IB_CM_RW, &new_state, 
			     sizeof(int), sizeof(int));
}
#endif

int ibus_rw(IBFD *dc, int new_state)
{
	uint32_t current;

	ibus_exec_cmd(dc, IB_CM_GET_STATUS, &current, 0, sizeof(uint32_t));
	current &= IB_DEV_MASK;
	if (new_state & IB_DEV_READ) current |= IB_DEV_READ;
	if (new_state & IB_DEV_WRITE) current |= IB_DEV_WRITE;
	new_state = current;

	return ibus_exec_cmd(dc, IB_CM_SET_STATUS, &new_state, 
			     sizeof(uint32_t), sizeof(uint32_t));
}


int ibus_set_msg_confirmation(IBFD *dc, int on_off)
{
	uint32_t current;

	ibus_exec_cmd(dc, IB_CM_GET_STATUS, &current, 0, sizeof(uint32_t));
	current &= IB_DEV_MASK;

	if (on_off) {
		current |= IB_DEV_CON_MSG;
		IBFD_STATUS(dc) |= IB_DEV_CON_MSG;
	} else {
		current &= ~IB_DEV_CON_MSG;
		IBFD_STATUS(dc) &= ~IB_DEV_CON_MSG;
	}

	return ibus_exec_cmd(dc, IB_CM_SET_STATUS, &current, 
			     sizeof(uint32_t), sizeof(uint32_t));
}


int ibus_who(IBFD *dc, DEVS **dev_list)
{
	int size;

	if ((*dev_list = (DEVS *) malloc(IB_PACKET_DATA_LEN)) == NULL) {	
		perror("ibus_who:malloc");
		*dev_list = NULL;
		return -1;
	}
	size = ibus_exec_cmd(dc, IB_CM_WHO, *dev_list, 0, IB_PACKET_DATA_LEN);
	if ( size > 0 ) {
		if (size < IB_PACKET_DATA_LEN)
			DEVS_ADDR((*dev_list+size)) = 0;	
	} else {
		free(*dev_list);
		*dev_list = NULL;
	}

	return size;
}

int ibus_shutdown( IBFD *dc )
{ 
	uint32_t res;
	ibus_exec_cmd(dc, IB_CM_SHUTDOWN, &res, 0, sizeof(uint32_t));
	return res == 0 ? 0 : -1;
}


int ibus_wait_packet(IBFD *dc, long time_out_sec, long time_out_usec)
{
	fd_set rfds;
	struct timeval tv;
	
	FD_ZERO(&rfds);
	FD_SET(IBFD_GET(dc), &rfds);
	tv.tv_sec = time_out_sec;
	tv.tv_usec = time_out_usec;

	return select(IBFD_GET(dc) + 1, &rfds, NULL, NULL, &tv);
}

 
int ibus_set_mcast_list(IBFD *dc, ADDR *list)
{
	int size;

	for(size = 0; ADDR_GET(list+size) != IB_NULL_ADDR; size++);
	
	return ibus_exec_cmd(dc, IB_CM_MCAST_LIST, (void *) list,
			     (size + 1) * sizeof(ADDR), 0);
}


int ibus_remove_mcast_list(IBFD *dc)
{
	return ibus_exec_cmd(dc, IB_CM_MCAST_LIST, NULL, 0, 0);
}


#if 0
static IBFD *ibdc_make(int fd)
{
	IBFD *dc = (IBFD *) malloc(sizeof(IBFD));
	IBFD_GET(dc) = fd;
	IBFD_STATUS(dc) = 0;
	IBFD_GET_USER_DATA(dc) = NULL;
	
	return dc;
}
#endif

#ifndef WIN32
IBFD *ibus_client_connect_local(char *name )
{
	IBFD *dc = NULL;
	int fd = call_socket_local(name);
	
	if (fd > 0) {
		dc = ib_fd_new();
		IBFD_GET(dc) = fd;
	}
	return dc;
}
#endif


IBFD *ibus_client_connect(char *hostname, int port)
{
	IBFD *dc = NULL;
	int fd = call_socket(hostname, port);
	
	if (fd > 0) {
		dc = ib_fd_new();
		IBFD_GET(dc) = fd;
	}
	return dc;
}


int ibus_client_close(IBFD *dc)
{
	if ( IBFD_GET(dc) > 0)
		close(IBFD_GET(dc));
	ib_fd_destroy(dc);

	return 0;
}


/* End */
