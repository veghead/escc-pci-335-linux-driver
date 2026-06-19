/******************************************************
 *
 * Copyright (C) 2000 Commtech, Inc. Wichita KS
 *
 * open.c -- open function for escc-pci module
 *
 * Tested with Linux version 2.2 12
 ******************************************************/

/* $Id: open.c,v 1.7 2005/03/17 15:32:46 carl Exp $ */

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
#include <linux/cred.h>
#include "esccdrv.h" /* local definitions */

int escc_open(struct inode *inode, struct file *filp)
{
    // int type = TYPE(inode->i_rdev);
    int num = NUM(inode->i_rdev);
    Escc_Dev *dev; /* device information */

    /* type 0, check the device number */
    if (num >= escc_nr_devs)
        return -ENODEV;

    dev = &escc_devices[num];
    if (dev->base == 0)
        return -ENODEV; // on PCI board if the base isn't allready there
                        // then the port will never exist (don't allow open)

    PDEBUG("%u OPEN -- opencount:%lu\n", num, dev->escc_u_count);

#ifdef ONLY_ONE_OPEN_AT_A_TIME
    if (dev->escc_u_count > 0)
        return -EBUSY;
#endif
    // multiple process opening avoidance as per 'Linux Device Drivers'
    // seems like a good idea
    // when a device is opened the uid is saved into our dev as the owner
    // if someone other than the owner tries to open the device they get EBUSY
    // when the user closes the device then the next user to open the device becomes the
    // owner, etc.

    if (dev->escc_u_count && !uid_eq(dev->escc_u_owner, current_uid()) &&
        !uid_eq(dev->escc_u_owner, current_euid()) &&
        (!capable(CAP_SYS_ADMIN)))
        return -EBUSY; // if allready open and not the same
    // user then fail with busy
    if (dev->escc_u_count == 0)
    {
        dev->escc_u_owner = current_uid();
    }
    dev->escc_u_count++;

    /* and use filp->private_data to point to the device data */
    filp->private_data = dev;

    dev->dev_no = num; // dev_no is the number that was opened (minor)
#ifdef USE_2_6
#else
    MOD_INC_USE_COUNT; // to keep the module loaded
#endif

    return 0; /* success */
}
