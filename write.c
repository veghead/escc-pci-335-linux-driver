/******************************************************
 *
 * Copyright (C) 2000 Commtech, Inc. Wichita KS
 *
 * write.c -- write function for escc-pci module
 *
 * Tested with Linux version 2.2 12
 ******************************************************/

/* $Id: write.c,v 1.5 2004/10/18 17:21:16 carl Exp $ */

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
#include <asm/io.h>
#include <linux/uaccess.h>
#include "esccdrv.h" /* local definitions */

ssize_t escc_write(struct file *filp,
                   const char __user *buf, size_t count, loff_t *off)
{
  struct buf *tbuf;
  unsigned i;
  unsigned port;
  unsigned mode;
  unsigned j;
  unsigned long flags;
#ifdef USE_2_6
  DEFINE_WAIT(wait);
#endif

  Escc_Dev *dev = filp->private_data;
  port = dev->base; // needed for register defines to work with portio functions calls

  PDEBUG("%u write, count:%lu, available:%u\n", dev->dev_no, (unsigned long)count, atomic_read(&dev->transmit_frames_available));

  if (dev->port_initialized)
  {
    // check to make sure it will fit in the txbuffer
    if (count > dev->settings.n_tfsize_max)
      return -EFBIG;

    while (atomic_read(&dev->transmit_frames_available) == 0)
    {
      if (filp->f_flags & O_NONBLOCK)
        return -EAGAIN; // if nonblocking then dump
//  PDEBUG("write sleeping \n");//otherwise block on a free buffer
#ifdef USE_2_6
      prepare_to_wait(&dev->wq, &wait, TASK_INTERRUPTIBLE);
      if (atomic_read(&dev->transmit_frames_available) == 0)
        schedule();
      finish_wait(&dev->wq, &wait);
#else

      save_flags(flags);
      cli();
      if (atomic_read(&dev->transmit_frames_available) == 0)
        interruptible_sleep_on(&dev->wq);
      restore_flags(flags);
#endif

      if (signal_pending(current))
        return -ERESTARTSYS;
    } // end of while not transmit_buffers_available

// here we have at least 1 transmit buffer, so fill it up and possibly start
// the interrupt tx chain

// find the free txbuffer
// note it must be done while isr is disabled
#ifdef USE_2_6
    spin_lock_irqsave(&dev->irqlock, flags);
#else

    save_flags(flags);
    cli(); // XDU occurs above this, isr handles it and all is well, we will go through the is_transmitting =0 case below and start up a new transmit
    // XDU occurs between the cli() and the restore_flags() then it won't get to the
    // isr until after the restore flags, and is_transmitting =1, the command is lost
    // along with the 32 bytes of data, but the new xdu/exe handler will restart the send

#endif
    i = dev->current_txbuf;
    do
    {
      tbuf = &dev->escc_tbuf[i];
      if (tbuf->valid == 0)
      {
#ifdef USE_2_6
        spin_unlock_irqrestore(&dev->irqlock, flags);
#else
        restore_flags(flags);
#endif

        atomic_dec(&dev->transmit_frames_available); // ISR increments this as it finishes sending frames, we decrement it as we queue them
        if (copy_from_user(tbuf->frame, buf, count))
          return -EFAULT;
        tbuf->max = count;
        tbuf->no_bytes = 0;
        tbuf->valid = 1;
        // check to see if we need to start the send.
        if (dev->is_transmitting == 0)
        {
          PDEBUG("is transmitting is 0, (tx is idle)\n");
          // is_transmitting flag is 0 so we are not currently transmitting, and there
          // are not going to be any more scheduled interrupts of the transmit varity
          // until we issue a transmit command, so lets start-er up.

#ifdef RS485_RX_ECHO_CANCEL
          outb(dev->settings.mode & 0xF7, MODE);
#endif
#ifdef FORCE_485_RTS_CONTROL
          outb(dev->settings.mode | 0x04, MODE);
#endif

          if (tbuf->max <= 32)
          {
            tbuf->valid = 0; // we will be done with this frame here
            atomic_inc(&dev->transmit_frames_available);
            outsb(FIFO, tbuf->frame, tbuf->max);
            tbuf->no_bytes = tbuf->max; // a formality, not needed
            // figure out the tx command
            mode = dev->settings.ccr0 & 0x03;
            if (mode <= 2) // hdlc or bisync (or SDLC Loop)
            {
              if ((dev->settings.mode & 0xc0) == 0xc0)
                j = dev->tx_type; // extended transparent hdlc mode
              else
                j = dev->tx_type + XME; // hdlc/sdlc/bisync need closing xme to finish the frame
            }
            else
              j = 0x08; // async transmit command is allways 0x08
          } // end of <=32 block
          else
          {
            // number of bytes >32 so send first 32 to get XPR interrupt chain going
            outsw(FIFO, tbuf->frame, 16); // write 32 bytes
            tbuf->no_bytes += 32;
            // figure out the tx command
            mode = dev->settings.ccr0 & 0x03;
            if (mode <= 2) // hdlc or bisync (or SDLC Loop)
            {
              j = dev->tx_type; // extended transparent hdlc mode
            }
            else
              j = 0x08; // async transmit command is allways 0x08
          } // end of >32 block

          WAIT_WITH_TIMEOUT; // wait for CEC clear
          dev->is_transmitting = 1;
          PDEBUG("CMDR: %d", j);
          outb(j, CMDR); // start the send, will immed cause XPR
        } // end of is_transmitting check
        return count;
      } // end of if valid==0;
      i++;
      if (i == dev->settings.n_tbufs)
        i = 0; // rollover
    } while (i != dev->current_txbuf);
#ifdef USE_2_6
    spin_unlock_irqrestore(&dev->irqlock, flags);
#else
    restore_flags(flags);
#endif
    // should not get here as we had indication of a free tbuf
    PDEBUG("past while in write with transmit_frames_available >0\n");
    return -EAGAIN;
  } // end of if port_initialized
  else
    return -EBUSY;
}
