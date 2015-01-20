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
 *
 *   27/12/2004: fix IB_PACKET_DATA_LEN
 *               add select() to write in send_data_to_dev();
 *   15/07/2005: add IB_DEV_BUSY and modify type in send_data_to_dev();
 *
 *   17/07/2005: check flag IB_DEV_BCAST_ON in .status on ib_cm_message();
 *               add broadcast addr in ib_cm_message_to();
 *
 **/

#define _GNU_SOURCE

#define DEBUG_PK 1

#include <errno.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "socket_srv.h"
#include "iar_bus.h"
#include "ib_dev.h"
#include "my_stdio.h"

static int done = 0;
static int ib_cm_shutdown_enable_flag = 0;
static struct ib_command _ib_cm[] = {
	{IB_CM_MESSAGE,    ib_cm_message},    /* tested */
//f	{IB_CM_MESSAGE_TO, ib_cm_message_to}, /* tested */
//f	{IB_CM_BCAST_ON,   ib_cm_bcast_on},   /* tested */
	{IB_CM_MCAST_LIST, ib_cm_mcast_list}, /* tested */
	{IB_CM_REMOVE,     ib_cm_remove},     /* tested */
//f	{IB_CM_BCAST_OFF,  ib_cm_bcast_off},  /* untested */
//f	{IB_CM_RW,         ib_cm_rw},         /* tested */
	{IB_CM_GET_STATUS, ib_cm_get_status}, /* tested */
	{IB_CM_SET_STATUS, ib_cm_set_status}, /* tested */
	{IB_CM_GET_ADDR,   ib_cm_get_addr},   /* tested */
	{IB_CM_SET_ADDR,   ib_cm_set_addr},   /* tested */
	{IB_CM_WHO,        ib_cm_who},        /* tested */
	{IB_CM_SHUTDOWN,   ib_cm_shutdown},   /* tested */
	{-1, NULL}
};


static struct ib_dev *init_ib_dev(void)
{
	struct ib_dev *dev;

	if ((dev = (struct ib_dev *) malloc(sizeof(struct ib_dev))) == NULL) {
		perror("init_ib_dev:malloc");
		exit(1);
	}
       	dev->fd = -1;
	dev->addr = IB_NULL_ADDR;
	dev->status = IB_DEV_RW;
	dev->status &= ~ IB_DEV_ALIVE;
	//dev->status |= IB_DEV_CON_MSG; /* def. off */
	dev->mcast_list = NULL;
	dev->next = dev->prev = NULL;
	
	return dev;
}


static int free_ib_dev(struct ib_dev *dev)
{
	if( dev->fd >= 0 )
		close(dev->fd);
	if ( dev->mcast_list )
		free(dev->mcast_list);
	free(dev);

	return 0;
}


static int set_ib_dev_addr(struct ib_dev *dev, int addr)
{
	if (addr == 0)	
		dev->addr = (IB_DYN_ADDR_FLAG | dev->fd) & IB_DYN_MASK_ADDR ;
	else 
		dev->addr = addr;

	return 0;
}


static struct ib_dev *find_ib_dev (struct ib_dev *dev, int addr)
{
	struct ib_dev *devp = dev;
	
	while (devp != NULL ){
		if (addr & IB_MULTICAST_FLAG) {
			if ( !(devp->addr & IB_DYN_ADDR_FLAG) ) {
				if ((devp->addr & (addr & IB_FIX_MASK_ADDR)) ==  
				    (devp->addr & IB_FIX_MASK_ADDR)) {
					return devp;
				}
			} else { /* IB_MULTICAST_FLAG and Dynamic */
				if ((devp->addr & addr) == devp->addr ) {
					return devp;
				}
			}
		} else {
			if (devp->addr == addr) {
				return devp;
			}
		}
		devp = devp->next;
	}

	return NULL;
}


static int die_ib_dev(struct ib_dev *devp)
{
	PVERB(PWARN, "Core bus: read/write device fail");
	close(devp->fd);
	devp->fd = -1;
	devp->status &= ~ IB_DEV_ALIVE;

	return 0;
}


static int send_data_to_dev(struct ib_packet *d, struct ib_dev *devp)
{
        fd_set wfds;
	struct timeval tv;
	int fd_max;
	int retval;
	int to_fd = devp->fd;

	if ( devp->status & IB_DEV_BUSY) {
		tv.tv_sec = 0;
		tv.tv_usec = 1;
	} else {
		tv.tv_sec = 5;
		tv.tv_usec = 0;
	}
	fd_max = to_fd + 1;
	FD_ZERO(&wfds);
	FD_SET(to_fd, &wfds);
	retval = select(fd_max, NULL, &wfds, NULL, &tv);
	if (retval) {
#if DEBUG_PK
		PVERB(PINFO,
		      "Core bus: send 0x%03x to 0x%03x, cm 0x%x, size %3d",
		      d->h.addr, devp->addr, d->h.cmd, d->h.size);
#endif

		if (write(to_fd, d, sizeof(struct ib_packet_head)) <= 0) {
			return -1;
		}
		if( d->h.size > 0 ) {
			FD_ZERO(&wfds);
			FD_SET(to_fd, &wfds);
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			retval = select(fd_max, NULL, &wfds, NULL, &tv);
			if (retval) {
				if (write(to_fd, d->data, d->h.size) < 0) {
					return -1;
				}
			}
		}
	}
	if (retval) {
		if (devp->status & IB_DEV_BUSY ) {
			SYSLOG(LOG_WARNING, "dev: %d, addr: 0x%03x: ok", 
			       to_fd, devp->addr);
			devp->status &= ~IB_DEV_BUSY;
		}
	} else {
		if ((devp->status & IB_DEV_BUSY) == 0) {
			SYSLOG(LOG_WARNING, "dev: %d, addr: 0x%03x: busy",
			       to_fd, devp->addr);
			devp->status |= IB_DEV_BUSY;
		}
	}

	return 0;
}


static int send_to_dev(struct ib_dev *from, struct ib_dev *to)
{
	if( to->status & (IB_DEV_ALIVE | IB_DEV_READ) ) {
		from->pk.h.addr = from->addr;
		if( send_data_to_dev( &from->pk, to) < 0 ){
			die_ib_dev( to );
			return -1;
		}
	}

	return 0;
}


static int message_broadcast(struct ib_dev *dev, struct ib_dev *current)
{
	struct ib_dev *devp = dev;

	while (devp != NULL){
		if( (devp->status & 
		     (IB_DEV_BCAST_ON|IB_DEV_ALIVE|IB_DEV_READ)) && 
		    (devp != current) ){
			//send_to_dev(current, devp);
			if( send_data_to_dev( &current->pk, devp) < 0 ){
				die_ib_dev( devp );
			}
		}
		devp = devp->next;
	}
	
	return 0;
}


static struct ib_command *find_ib_command(struct ib_command *cm, int cm_id)
{
	int i;

	for (i = 0; cm[i].func; i++) {
		if (cm[i].cm_id == cm_id ) {
			return (&cm[i]);
		}
	}
	
	return ((struct ib_command *)NULL);
}


/* Execute a command. */
static int execute_command(struct ib_command *cm, struct ib_dev *dev,
			   struct ib_dev *devp)
{
	struct ib_command *command;
	
	command = find_ib_command (cm, devp->pk.h.cmd);

	if (!command) {
		return -1;
	}

	/* Call the function. */
	return ((*(command->func)) (dev, devp));
}


static int read_cm_from_dev(struct ib_dev *devp)
{
	if ( read(devp->fd, &devp->pk.h, sizeof(struct ib_packet_head)) <= 0) {
		/* Ops... die */
		die_ib_dev( devp );
		return -1;
	}
	if (  devp->pk.h.size > 0 && devp->pk.h.size <= IB_PACKET_DATA_LEN ){
		if ( read(devp->fd, devp->pk.data, devp->pk.h.size) <= 0 ) {
			die_ib_dev( devp );
			return -1;
		}
	}
	devp->status |= IB_DEV_NEWPK;
#if DEBUG_PK 
	  PVERB(PINFO, "Core bus: rcv  0x%03x to 0x%03x, cm 0x%x, size %3d",
		devp->addr, devp->pk.h.addr, devp->pk.h.cmd, devp->pk.h.size);
#endif
	return 0;
}


static int process_cm_queue(struct ib_command *cm, struct ib_dev *dev)
{
	struct ib_dev *devp = dev;

	while(devp != NULL) {
		if ( devp->status & IB_DEV_NEWPK ){
			execute_command(cm, dev, devp);
			devp->status &= ~IB_DEV_NEWPK;
		}
		devp = devp->next;
	}

	return 0;
}


static void intHandler (int signo) 
{
	SYSLOG(LOG_ERR, "Core bus: write error, SIGPIPE trapped");
	return;
}


static int iar_bus_core(struct ib_command *cm, int fd_socket )
{
	struct ib_dev *dev, *devp, *devd;
	fd_set rfds;
	struct timeval tv;
	int retval;
	struct sockaddr isa;
	socklen_t addrlen; /* originally int */
	int queue;
	int fd_max;

	addrlen = sizeof(isa); /* find socket's address for accept() */
	getsockname(fd_socket, (struct sockaddr *)&isa, &addrlen);
	if (signal(SIGPIPE, intHandler) == SIG_ERR) {
		perror("iar_bus_core:Signal Failed (SIGPIPE).");
		exit(1);
	}
	dev = init_ib_dev();
	/* bus dev */
	dev->fd = fd_socket;
	dev->addr = IB_BUS_ADDR;
	dev->status = IB_DEV_BCAST_ON | IB_DEV_ALIVE | IB_DEV_READ;

	done = 0;
	while (!done){
		devp = dev;
		FD_ZERO(&rfds);
		fd_max = fd_socket + 1;
		while(devp != NULL) {
			if (devp->fd && (devp->status & IB_DEV_ALIVE) ){ 
				FD_SET(devp->fd, &rfds);
				if( fd_max < devp->fd + 1)
					fd_max = devp->fd + 1;
			}
			else if ( !(devp->status & IB_DEV_ALIVE) && 
				  (devp != dev) ) { 
                                /* OK: device dead, cleaning */ 
				devd = devp;
				devp = devd->prev;
				devp->next = devd->next;
				if( devd->next )
					devd->next->prev = devp;
				free_ib_dev(devd);	
			}
			devp = devp->next;
		}
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		retval = select(fd_max, &rfds, NULL, NULL, &tv);
		if (retval) {
			devp = dev->next ; /* skip device bus */
			queue = 0;
			while(devp != NULL) {
				if(devp->status & IB_DEV_ALIVE)
					if( FD_ISSET(devp->fd, &rfds) ) {
						if ( read_cm_from_dev(devp)==0 ) 
							queue = 1;
					}
				devp = devp->next;
			}
			if( queue )
				process_cm_queue(cm, dev->next);
			if (FD_ISSET(dev->fd, &rfds)) {
				/* New device is connected */
				PVERB(PINFO, "Core bus: new call");
				devp = dev ;
				while(devp->next != NULL) devp = devp->next;
				devp->next = init_ib_dev();
				devp->next->prev = devp;
				devp = devp->next;
				devp->fd = accept(dev->fd, 
						  (struct sockaddr *)&isa, 
						  &addrlen); 
				if( devp->fd ){
					devp->status |= IB_DEV_ALIVE;
					PVERB(PINFO, "Core bus: connected");
					set_ib_dev_addr(devp, 0);
				}
			}
		}
	}

	devp = dev;
	while (devp != NULL) {
		if (devp->fd) {
			close(devp->fd);
		}
		devp = devp->next;
	}

	return 0;
}


/****************************************************************************
 * Start functions:
 **/

int init_iar_bus_local(struct ib_command *cm, char *sock_name )
{
	int fd_socket;

	if ( (fd_socket = socket_init_local( sock_name )) < 0) {
		return -1;
	}

	return iar_bus_core(cm, fd_socket );
}


int init_iar_bus_inet(struct ib_command *cm, int port )
{
	int fd_socket;

	if ( (fd_socket = socket_init( port )) < 0) {
		return -1;
	}

	return iar_bus_core(cm, fd_socket);
}


int iar_bus_start( char *sock_name )
{
	return init_iar_bus_local( _ib_cm, sock_name );
}


int iar_bus_inet_start( int port )
{
	return init_iar_bus_inet( _ib_cm, port );
}


int iar_bus_set_shutdown(int state)
{
	return ib_cm_shutdown_enable_flag = (state == 0 ? 0 : 1);
}

/**
 * End Start functions.
 ****************************************************************************/


/****************************************************************************
 * Command functions:
 **/


int ib_cm_remove(struct ib_dev *dev, struct ib_dev *current)
{
	struct ib_packet d = {
		.h.addr = IB_BUS_ADDR,
		.h.size = 0,
		.h.cmd = IB_CM_REMOVE,
	};

	send_data_to_dev(&d, current);
	current->status &= ~ (IB_DEV_BCAST_ON |IB_DEV_ALIVE) ;

	return close( current->fd ); /* free current en core */
}

#if 0
int ib_cm_bcast_on(struct ib_dev *dev, struct ib_dev *current)
{
	struct ib_packet d = {
		.h.addr = IB_BUS_ADDR,
		.h.size = 0,
		.h.cmd = IB_CM_BCAST_ON,
	};

	current->status |= IB_DEV_BCAST_ON;
	send_data_to_dev(&d, current);

	return 0;
}


int ib_cm_bcast_off(struct ib_dev *dev, struct ib_dev *current)
{
	struct ib_packet d = {
		.h.addr = IB_BUS_ADDR,
		.h.size = 0,
		.h.cmd = IB_CM_BCAST_OFF,
	};

	send_data_to_dev(&d, current);
	current->status &= ~ IB_DEV_BCAST_ON;

	return 0;
}


int ib_cm_rw(struct ib_dev *dev, struct ib_dev *current)
{
	uint32_t new_state = *((uint32_t*) current->pk.data);
	struct ib_packet d = {
		.h.addr = IB_BUS_ADDR,
		.h.size = sizeof(uint32_t),
		.h.cmd = IB_CM_RW,
	};

	new_state &= IB_DEV_RW;
	*((uint32_t *) d.data) = new_state;
	current->status &= ~IB_DEV_RW;
	current->status |= new_state;
	send_data_to_dev(&d, current);

	return 0;
}
#endif


int ib_cm_set_status(struct ib_dev *dev, struct ib_dev *current)
{
	//uint32_t new_state = *((uint32_t*) current->pk.data);
	uint32_t new_state;
	struct ib_packet d = {
		.h.addr = IB_BUS_ADDR,
		.h.size = sizeof(uint32_t),
		.h.cmd = IB_CM_SET_STATUS,
	};

	__builtin_memcpy(&new_state, current->pk.data, sizeof(uint32_t));
	new_state &= IB_DEV_MASK;
	//*((uint32_t *) d.data) = new_state;
	__builtin_memcpy(d.data, &new_state, sizeof(uint32_t));
	current->status &= ~IB_DEV_MASK;
	current->status |= new_state;
	send_data_to_dev(&d, current);

	return 0;
}


int ib_cm_get_status(struct ib_dev *dev, struct ib_dev *current)
{
	struct ib_packet d = {
		.h.addr = IB_BUS_ADDR,
		.h.size = sizeof(uint32_t),
		.h.cmd = IB_CM_GET_STATUS,
	};

	//*((uint32_t *) d.data) = current->status;
	__builtin_memcpy(d.data, &current->status, sizeof(uint32_t));
	send_data_to_dev(&d, current);

	return 0;
}


#if 1

/* ib_cm_message and ib_cm_message_to is now this function; */
int ib_cm_message(struct ib_dev *dev, struct ib_dev *current)
{
	int ret = -1;
	struct ib_dev *to;
	struct ib_packet d = {
		.h.addr = IB_BUS_ADDR,
		.h.size = sizeof(uint32_t),
		.h.cmd = current->pk.h.cmd, /* IB_CM_MESSAGE */
	};
	int addr_to = current->pk.h.addr;

	if ( addr_to == IB_BROADCAST_ADDR) {
		ret = message_broadcast(dev, current); /* never fail */
	} else {
		if ((addr_to == IB_MULTICAST_FLAG) &&
		    (current->mcast_list != NULL) ) {
			ADDR *mca = current->mcast_list;
			while ( ADDR_GET(mca) != IB_NULL_ADDR) {
				if ( (to = find_ib_dev(dev, ADDR_GET(mca))) ) {
					if (send_to_dev(current, to ) == 0 )
						ret = 0;
				}
				mca++;
			}	
		} else {
			to = dev;
			do {
				if ( (to = find_ib_dev(to, addr_to)) ) {
					if (to->addr != current->addr)
						ret = send_to_dev(current, to );
					to = to->next;
				}
			} while (to != NULL);
		} 
		if (IB_DEV_IF_CON_MSG(current) ) {
			//*((uint32_t *) d.data) = (ret < 0 ? 1 : 0);
			__builtin_memset(d.data, (ret < 0 ? 1 : 0), 
					 sizeof(uint32_t));
			send_data_to_dev(&d, current);
		}
	}
	
	return ret;
}

#else

int ib_cm_message_to(struct ib_dev *dev, struct ib_dev *current)
{
	struct ib_dev *to;

	if ( current->pk.h.addr == IB_BROADCAST_ADDR) {
		return ib_cm_message(dev, current);
	}
	if ( (to = find_ib_dev(dev, current->pk.h.addr)) ) {
		return send_to_dev(current, to );
	}
	
	return -1;
}

#endif




int ib_cm_set_addr(struct ib_dev *dev, struct ib_dev *current)
{
	//int new_addr = *((uint32_t*) current->pk.data);
	uint32_t new_addr;
	int ret;
	struct ib_packet d = {
		.h.addr = IB_BUS_ADDR,
		.h.size = sizeof(uint32_t),
		.h.cmd = IB_CM_SET_ADDR,
	};
	__builtin_memcpy(&new_addr, current->pk.data, sizeof(uint32_t));
	if ( ! IB_IS_VALID_FIX_ADDR(new_addr) || 
	     ( find_ib_dev(dev, new_addr) != NULL) ) {
		new_addr = current->addr;
		ret = -1;
	} else {
		ret = set_ib_dev_addr(current, new_addr);
	}
	//*((uint32_t*) d.data) = new_addr;
	__builtin_memcpy(d.data, &new_addr, sizeof(uint32_t));
	send_data_to_dev(&d, current);

	return ret;
}


int ib_cm_get_addr(struct ib_dev *dev, struct ib_dev *current)
{
	struct ib_packet d = {
		.h.addr = IB_BUS_ADDR,
		.h.size = sizeof(uint32_t),
		.h.cmd = IB_CM_GET_ADDR,
	};

	//*((uint32_t*) d.data) =  current->addr;
	__builtin_memcpy(d.data, &current->addr, sizeof(uint32_t));

	return send_data_to_dev(&d, current);
}


int ib_cm_who(struct ib_dev *dev, struct ib_dev *current)
{
	int count = 0;
	struct ib_dev *devp = dev;
	struct ib_packet d = {
		.h.addr = IB_BUS_ADDR,
		.h.size = 0,
		.h.cmd = IB_CM_WHO,
	};
	DEVS *dvs = (DEVS *) d.data;

	while(devp != NULL && count < (IB_PACKET_DATA_LEN / sizeof(DEVS))) {
		DEVS_ADDR(dvs) = devp->addr;
		DEVS_STATUS(dvs) = devp->status;
		dvs++;
		count++;
		devp = devp->next;
	}
	d.h.size = count * sizeof(DEVS);

	return send_data_to_dev(&d, current);
}



int ib_cm_shutdown(struct ib_dev *dev, struct ib_dev *current)
{
	struct ib_packet d = {
		.h.addr = IB_BUS_ADDR,
		.h.size = sizeof(uint32_t),
		.h.cmd = IB_CM_SHUTDOWN,
	};
	if (ib_cm_shutdown_enable_flag) {
		done = 1;
		//*((uint32_t*) d.data) = 0;
		__builtin_memset(d.data, 0, sizeof(uint32_t));
		message_broadcast(dev, current);
	} else {
		//*((uint32_t*) d.data) = 1;
		__builtin_memset(d.data, 1, sizeof(uint32_t));
	}
	send_data_to_dev(&d, current);

	return 0;
}


int ib_cm_mcast_list(struct ib_dev *dev, struct ib_dev *current)
{
	struct ib_packet d = {
		.h.addr = IB_BUS_ADDR,
		.h.size = 0,
		.h.cmd = IB_CM_MCAST_LIST,
	};

	if ( current->mcast_list != NULL) {
		free(current->mcast_list);
		current->mcast_list = NULL;
	}
	if (current->pk.h.size > 0) {
		current->mcast_list = (ADDR *) malloc(current->pk.h.size);
		memcpy(current->mcast_list, current->pk.data, 
		       current->pk.h.size);
	}
	send_data_to_dev(&d, current);	

	return 0;
}

/**
 * End command functions.
 ****************************************************************************/

/* End */


