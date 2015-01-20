/** 
 *   ib_dev: IPC multi-platform on virtual BUS, based on socket. 
 *
 *   (c) Copyright 2004-2010 Federico Bareilles <fede@iar.unlp.edu.ar>,
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
 **/

#ifndef _IB_DEV_H
#define _IB_DEV_H


struct ib_dev {
	uint32_t fd;     /* socket descriptor */
	uint32_t addr;   /* device address */
	uint32_t status; /* device on/off, etc ... */
	struct ib_packet pk; /* last packet send from device */
	ADDR *mcast_list;
	struct ib_dev *prev;
	struct ib_dev *next;
};


/* for status in struct ib_dev */
#define IB_DEV_BCAST_ON 0x0001 /* Broadcas enable */ 
//#define IB_DEV_OFF      ( ~ IB_DEV_ON ) /* Broadcast desable */
#define IB_DEV_BUSY     0x0002 
#define IB_DEV_READ     0x0004 /* from the device side */
#define IB_DEV_WRITE    0x0008 /*  "                   */
#define IB_DEV_RW       ( IB_DEV_READ | IB_DEV_WRITE )
#define IB_DEV_NEWPK    0x0010 /* new packet for dispatching */
#define IB_DEV_ALIVE    0x0020
#define IB_DEV_CON_MSG  0x0040 /* Return mesage (to) confirmation */
#define IB_DEV_MASK (IB_DEV_BCAST_ON|IB_DEV_RW|IB_DEV_CON_MSG)

#define IB_DEV_IF_CON_MSG(ib) (ib->status & IB_DEV_CON_MSG)

struct ib_command {
	int cm_id;
	int (*func)(struct ib_dev *dev, struct ib_dev *curren);
};

int ib_cm_remove(struct ib_dev *dev, struct ib_dev *current);
/* Broadcast enable: */ 
//int ib_cm_bcast_on(struct ib_dev *dev, struct ib_dev *current); 
/* Broadcast desable (dafault): */
//int ib_cm_bcast_off(struct ib_dev *dev, struct ib_dev *current);
//int ib_cm_rw(struct ib_dev *dev, struct ib_dev *current);
int ib_cm_set_status(struct ib_dev *dev, struct ib_dev *current);
int ib_cm_get_status(struct ib_dev *dev, struct ib_dev *current);
/* Send broadcast: */
int ib_cm_message(struct ib_dev *dev, struct ib_dev *current);
int ib_cm_message_to(struct ib_dev *dev, struct ib_dev *current);
int ib_cm_set_addr(struct ib_dev *dev, struct ib_dev *current);
int ib_cm_get_addr(struct ib_dev *dev, struct ib_dev *current);
int ib_cm_shutdown(struct ib_dev *dev, struct ib_dev *current);
int ib_cm_who(struct ib_dev *dev, struct ib_dev *current);
int ib_cm_mcast_list(struct ib_dev *dev, struct ib_dev *current);

int init_iar_bus_local(struct ib_command *cm, char *sock_name );
int init_iar_bus_inet(struct ib_command *cm, int port );




#endif  /* ifndef _IB_DEV_H */
