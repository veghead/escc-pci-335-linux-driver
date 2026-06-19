/******************************************************
 *
 * Copyright (C) 2000 Commtech, Inc. Wichita KS
 *
 * poll.c -- poll (select) function for escc-pci module
 *
 * not currently tested
 ******************************************************/

/* $Id: poll.c,v 1.3 2004/07/07 14:33:56 carl Exp $ */

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

If you encounter problems or have suggestions and/or bug fixes please email them to:

custserv@commtech-fastcom.com

or mailed to:

Commtech, Inc.
9011 E. 37th N.
Wichita, KS 67226
ATTN: Linux BugFix Dept

*/

#ifndef __KERNEL__
#define __KERNEL__
#endif
#ifndef MODULE
#define MODULE
#endif

#define __NO_VERSION__
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h>  /* get MOD_DEC_USE_COUNT, not the version string */
#include <linux/version.h> /* need it for conditionals in scull.h */

#include <linux/kernel.h> /* printk() */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
#include <linux/slab.h>
#else
#include <linux/malloc.h> /* kmalloc() */
#endif
#include <linux/vmalloc.h>
#include <linux/fs.h>    /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/fcntl.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <asm/irq.h>
#include "esccdrv.h" /* local definitions */

unsigned int escc_poll(struct file *filp, poll_table *pt)
{
    Escc_Dev *dev;
    unsigned int mask = 0;

    dev = filp->private_data;

    poll_wait(filp, &dev->rq, pt);
    poll_wait(filp, &dev->wq, pt);

    if (atomic_read(&dev->received_frames_pending) != 0)
        mask |= POLLIN | POLLRDNORM;
    if (atomic_read(&dev->transmit_frames_available) != 0)
        mask |= POLLOUT | POLLWRNORM;

    return mask;
}
