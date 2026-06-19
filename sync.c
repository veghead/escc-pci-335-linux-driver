/******************************************************
 *
 * Copyright (C) 2000 Commtech, Inc. Wichita KS
 *
 * sync.c -- sync function for escc-pci module
 *
 * Not currently tested
 ******************************************************/

/* $Id: sync.c,v 1.4 2004/10/18 17:21:16 carl Exp $ */

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

#define __NO_VERSION__ /* don't define kernel_verion in module.h */
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h> /* printk() */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
#include <linux/slab.h>
#else
#include <linux/malloc.h> /* kmalloc() */
#endif
#include <linux/fs.h>    /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/timer.h>
#ifdef USE_2_6
#include <linux/wait.h>
#endif
#include <asm/io.h>
#include "esccdrv.h" /* local definitions */

#if LINUX_VERSION_CODE >= VERSION_CODE(2, 4, 0)
wait_queue_head_t escc_fsync_wait;
#else
struct wait_queue *escc_fsync_wait;
#endif

struct timer_list escc_fsync_t1;

void sync_timeout(unsigned long ptr);
int escc_fsync(struct file *filp, loff_t start, loff_t end, int datasync)
{
    Escc_Dev *dev;

    dev = filp->private_data;

    PDEBUG("fsync -entry\n");

    while (atomic_read(&dev->transmit_frames_available) != dev->settings.n_tbufs)
    {
        // should block here...but on what...
#ifdef USE_2_6
        wait_event_interruptible_timeout(escc_fsync_wait, (atomic_read(&dev->transmit_frames_available) == dev->settings.n_tbufs), HZ);
#else
        init_timer(&escc_fsync_t1);
        escc_fsync_t1.function = sync_timeout;
        escc_fsync_t1.data = 0;
        escc_fsync_t1.expires = jiffies + HZ;
        add_timer(&escc_fsync_t1);
        interruptible_sleep_on(&escc_fsync_wait);
#endif
        PDEBUG("fsync-timeout\n");
    }
    // should check for ST_ALLS also here...
    PDEBUG("fsync -exit\n");
    return 0;
}
void sync_timeout(unsigned long ptr)
{
    wake_up_interruptible(&escc_fsync_wait);
}
