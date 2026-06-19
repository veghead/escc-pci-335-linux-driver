/******************************************************
 *
 * Copyright (C) 2000 Commtech, Inc. Wichita KS
 *
 * read.c -- read function for escc-pci module
 *
 * Tested with Linux version 2.2 12
 ******************************************************/

/* $Id: read.c,v 1.4 2004/10/18 17:21:16 carl Exp $ */

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
#include <linux/uaccess.h>
#include "esccdrv.h" /* local definitions */

ssize_t escc_read(struct file *filp,
                  char __user *buf, size_t count, loff_t *off)
{
  count_t retval;
  unsigned i;
  unsigned long flags;
  struct buf *rbuf;
#ifdef USE_2_6
  DEFINE_WAIT(wait);
#endif

  Escc_Dev *dev = filp->private_data; // our device information
  PDEBUG("%u read, count:%lu, frames_pending:%u\n", dev->dev_no, (unsigned long)count, atomic_read(&dev->received_frames_pending));

  if (dev->port_initialized) // if the port isn't inited then skip the read
  {
    while (atomic_read(&dev->received_frames_pending) == 0) // block if there are no frames
    {
      if (filp->f_flags & O_NONBLOCK)
        return -EAGAIN; // don't block if specified nonblocking read
//  PDEBUG("read sleaping\n");
#ifdef USE_2_6
      prepare_to_wait(&dev->rq, &wait, TASK_INTERRUPTIBLE);
      if (atomic_read(&dev->received_frames_pending) == 0)
        schedule();
      finish_wait(&dev->rq, &wait);
#else
      save_flags(flags);
      cli(); // keep ints from happening before sleepon is called
      if (atomic_read(&dev->received_frames_pending) == 0)
        interruptible_sleep_on(&dev->rq);
      restore_flags(flags);
#endif

      // if we got signaled then restart the call so it will complete correctly
      if (signal_pending(current))
        return -ERESTARTSYS;

    } // end of while not pending frames

// if we get here we have at least one frame to return so return it
// we have to do this with interrupts disabled so that our ISR doesn't update
// the dev->current_rxbuf value while we are using it
#ifdef USE_2_6
    spin_lock_irqsave(&dev->irqlock, flags);
#else
    save_flags(flags);
    cli();
#endif
    i = dev->current_rxbuf; // get the current buffer
    do
    {
      i++; // start at current +1
      if (i == dev->settings.n_rbufs)
        i = 0;                   // rollover
      rbuf = &dev->escc_rbuf[i]; // get pointer to this buffer
      if (rbuf->valid == 1)      // it is assumed that 1 means the data in the buffer is valid
      {
#ifdef USE_2_6
        spin_unlock_irqrestore(&dev->irqlock, flags);
#else
        restore_flags(flags); // we are done with dev->current_rbuf here, so we can let IRQ's fly
#endif
        if (rbuf->no_bytes > count)
        {
          return -EFBIG; // we must return a complete frame,
          // I realize that the book says that you can return less than count bytes
          // but it really does not make sense to break up a frame into multiple reads for
          // any of the operating modes execpt async.  And if you want to re-write it
          // to make async return partial frames then so be it.
          //  if their buffer isn't big enough pump back an error...
          // would really like a different one, but haven't figured errno's out quite yet
        }
        if (copy_to_user(buf, rbuf->frame, rbuf->no_bytes))
          return -EFAULT;                          // send frame to user
        rbuf->valid = 0;                           // make buffer free for later
        retval = rbuf->no_bytes;                   // size actually transfered
        rbuf->no_bytes = 0;                        // clear for next use
        atomic_dec(&dev->received_frames_pending); // decrement pending frame count
        return retval;
      } // end of if rbuf->valid

    } while (i != dev->current_rxbuf); // keep looping until we get back to the current one
#ifdef USE_2_6
    spin_unlock_irqrestore(&dev->irqlock, flags);
#else
    restore_flags(flags); // if we break from above we need to re-enable IRQ's
#endif
    // really should not get here
    PDEBUG("past while loop in read after received_frames_pending >0\n");
    return -EAGAIN;
  } // end of port initialized
  else
    return -EBUSY;

} // end of read
