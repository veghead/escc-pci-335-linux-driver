/******************************************************
 *
 * Copyright (C) 2000 Commtech, Inc. Wichita KS
 *
 * ioctl.c -- ioctl function for escc-pci module
 *
 * Tested with Linux version 2.2 12
 ******************************************************/

/* $Id: ioctl.c,v 1.9 2004/12/15 16:54:53 carl Exp $ */

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
#include <linux/kernel.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
#include <linux/slab.h>
#else
#include <linux/malloc.h> /* kmalloc() */
#endif
#include <linux/vmalloc.h>
#include <linux/fs.h> /* everything... */
#include <linux/proc_fs.h>
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/fcntl.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include "esccdrv.h" /* local definitions */

/*
 * The ioctl() implementation
 */

long escc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long timeoutvalue;
	unsigned long flags;
	unsigned long status;
	unsigned port;
	unsigned i; //,j;
	char *tempbuf;
	Escc_Dev *dev;
	Escc_Dev *d;
	// board_settings board_switches;
	regsingle regs;
	clkset clock;
	unsigned long passval;
	unsigned long txbytesleft;
	struct buf *tbuf;
	unsigned long actual;
	unsigned long icsbits;
	unsigned long features;
#ifdef USE_2_6
	DEFINE_WAIT(wait);
#endif

	//    int err = 0;
	// int tmp;
	// int size = _IOC_SIZE(cmd); /* the size bitfield in cmd */
	dev = filp->private_data;
	port = dev->base;
	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return EINVAL before verify_area()
	 */
	if (_IOC_TYPE(cmd) != ESCC_IOC_MAGIC)
		return -EINVAL;
	if (_IOC_NR(cmd) > ESCC_IOC_MAXNR)
		return -EINVAL;
	PDEBUG("%u ioctl-entry:cmd :%u\n", dev->dev_no, _IOC_NR(cmd));
	// printk("tsc:%Ld\r\n",get__tsc());
	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * verify_area is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	/* //obsoleted by v 2.1 kernel
		if (_IOC_DIR(cmd) & _IOC_READ)
			err = verify_area(VERIFY_WRITE, (void *)arg, size);
		else if (_IOC_DIR(cmd) & _IOC_WRITE)
			err =  verify_area(VERIFY_READ, (void *)arg, size);
		if (err) return err;
	*/
	switch (cmd)
	{

	case ESCC_INIT:
		if (dev->base == 0)
			return -ENODEV; // don't try to init a board that isn't there

		// get user settings
		if (copy_from_user(&dev->settings, (setup *)arg, sizeof(setup)))
			return -EFAULT;

		PDEBUG("INIT:CCR0:%x,CCR1:%x,CCR2:%x,MODE:%x\n", dev->settings.ccr0, dev->settings.ccr1, dev->settings.ccr2, dev->settings.mode);

		dev->tx_type = XTF; // import this from somewhere...default to XTF's

		// mask interrupts at device
		outb(0xff, IMR0);
		outb(0xff, IMR1);
		// figure out the mode and setup the registers
		if ((dev->settings.ccr0 & 0x03) == 0)
		{
			PDEBUG("HDLC SETTINGS\n");
			// init HDLC
			outb(dev->settings.mode, MODE);
			outb(dev->settings.timr, TIMR);
			outb(dev->settings.xad1, XAD1);
			outb(dev->settings.xad2, XAD2);
			outb(dev->settings.rah1, RAH1);
			outb(dev->settings.rah2, RAH2);
			outb(dev->settings.ral1, RAL1);
			outb(dev->settings.ral2, RAL2);
			outb(dev->settings.xbcl, XBCL);
			outb(dev->settings.xbch, XBCH);
			outb(dev->settings.ccr0, CCR0);
			outb(dev->settings.ccr1, CCR1);
			outb(dev->settings.ccr2, CCR2);
			outb(dev->settings.ccr3, CCR3);
			outb(dev->settings.ccr4, CCR4);
			outb(dev->settings.bgr, BGR);
			outb(dev->settings.rlcr, RLCR);
			outb(dev->settings.pre, PRE);
			outb(dev->settings.pvr, PVR);
			outb(dev->settings.tsax, TSAX);
			outb(dev->settings.tsar, TSAR);
			outb(dev->settings.xccr, XCCR);
			outb(dev->settings.rccr, RCCR);
			outb(dev->settings.amh, AMH);
			outb(dev->settings.aml, AML);
		}
		if ((dev->settings.ccr0 & 0x03) == 1)
		{
			// init SDLC loop
		}
		if ((dev->settings.ccr0 & 0x03) == 2)
		{
			PDEBUG("Bisync SETTINGS\n");
			// init Bisync
			outb(dev->settings.mode, MODE);
			outb(dev->settings.timr, TIMR);
			outb(dev->settings.synl, SYNL);
			outb(dev->settings.synh, SYNH);
			outb(dev->settings.tcr, TCR);
			outb(dev->settings.dafo, DAFO);
			outb(dev->settings.rfc, RFC);
			outb(dev->settings.xbcl, XBCL);
			outb(dev->settings.xbch, XBCH);
			outb(dev->settings.ccr0, CCR0);
			outb(dev->settings.ccr1, CCR1);
			outb(dev->settings.ccr2, CCR2);
			outb(dev->settings.ccr3, CCR3);
			outb(dev->settings.ccr4, CCR4);
			outb(dev->settings.bgr, BGR);
			outb(dev->settings.pre, PRE);
			outb(dev->settings.tsax, TSAX);
			outb(dev->settings.tsar, TSAR);
			outb(dev->settings.xccr, XCCR);
			outb(dev->settings.rccr, RCCR);
			outb(dev->settings.pvr, PVR);
		}
		if ((dev->settings.ccr0 & 0x03) == 3)
		{
			PDEBUG("ASYNC SETTINGS\n");
			// init async
			outb(dev->settings.mode, MODE);
			outb(dev->settings.timr, TIMR);
			outb(dev->settings.tcr, TCR);
			outb(dev->settings.dafo, DAFO);
			outb(dev->settings.rfc, RFC);
			outb(dev->settings.xbcl, XBCL);
			outb(dev->settings.xbch, XBCH);
			outb(dev->settings.ccr0, CCR0);
			outb(dev->settings.ccr1, CCR1);
			outb(dev->settings.ccr2, CCR2);
			outb(dev->settings.ccr3, CCR3);
			outb(dev->settings.ccr4, CCR4);
			outb(dev->settings.bgr, BGR);
			outb(dev->settings.xon, XON);
			outb(dev->settings.xoff, XOFF);
			outb(dev->settings.tsax, TSAX);
			outb(dev->settings.tsar, TSAR);
			outb(dev->settings.xccr, XCCR);
			outb(dev->settings.rccr, RCCR);
			outb(dev->settings.mxn, MXN);
			outb(dev->settings.mxf, MXF);
			outb(dev->settings.pvr, PVR);
		}
		// resets here
		// XRES & RHR
		timeoutvalue = 0;
		do
		{
			timeoutvalue++;
			if ((inb(STAR) & CEC) == 0x00)
				break;
		} while (timeoutvalue < MAX_TIMEOUT_VALUE);

		if (timeoutvalue >= MAX_TIMEOUT_VALUE)
		{
			PDEBUG("TIMEOUT in waitforCEC\n");
			outb(dev->settings.imr0, 0xff);
			outb(dev->settings.imr1, 0xff);
			outb(dev->settings.pim, 0xff); // could fubar ch2/ch1 DTR/DSR interrupt settings
			return -EBUSY;
		}

		// PDEBUG("XRES-ISSUED\n");
		outb(XRES, CMDR); // reset transmit

		timeoutvalue = 0;
		do
		{
			timeoutvalue++;
			if ((inb(STAR) & CEC) == 0x00)
				break;
		} while (timeoutvalue < MAX_TIMEOUT_VALUE);
		if (timeoutvalue >= MAX_TIMEOUT_VALUE)
		{
			PDEBUG("TIMEOUT in waitforCEC-xres\n");
			outb(dev->settings.imr0, 0xff);
			outb(dev->settings.imr1, 0xff);
			outb(dev->settings.pim, 0xff); // could fubar ch2/ch1 DTR/DSR interrupt settings
			return -EBUSY;
		}
		// PDEBUG("RHR-ISSUED\n");
		outb(RHR, CMDR);

		timeoutvalue = 0;
		do
		{
			timeoutvalue++;
			if ((inb(STAR) & CEC) == 0x00)
				break;
		} while (timeoutvalue < MAX_TIMEOUT_VALUE);
		if (timeoutvalue >= MAX_TIMEOUT_VALUE)
		{
			PDEBUG("TIMEOUT in waitforCEC-rhr\n");
			outb(dev->settings.imr0, 0xff);
			outb(dev->settings.imr1, 0xff);
			outb(dev->settings.pim, 0xff); // could fubar ch2/ch1 DTR/DSR interrupt settings
			return -EBUSY;
		}

		// check limits on n_rbufs/ntbufs/n_rfsize_max/n_tfsize_max
		// and free if allready allocated
		// these are forced because the queuing routines must have at least 2 buffers to work with or they don't operate correctly
		// the min size is to match the hardware fifo size on the 82532
		// the max size is really the max HDLC received frame size before you cannot
		// tell using the chip how many bytes you have received, it is not currently
		// enforced, but it would be wastefull to use frame buffers bigger than this
		if (dev->settings.n_rbufs < 2)
			dev->settings.n_rbufs = 2; //
		if (dev->settings.n_tbufs < 2)
			dev->settings.n_tbufs = 2; //
		if (dev->settings.n_rfsize_max < 64)
			dev->settings.n_rfsize_max = 64; //
		if (dev->settings.n_tfsize_max < 64)
			dev->settings.n_tfsize_max = 64; //
		// technically you can use the overflow bit to get the counter resolution up to 8194
		// and It has been done before(in the NT driver), allowing HDLC frames > 4k to be received.
		// similar testing has not been done on this driver to date
		// if(dev->settings.n_rfsize_max>8194) dev->settings.n_rfsize_max = 8194;
		// if(dev->settings.n_tfsize_max>8194) dev->settings.n_tfsize_max = 8194;

		if ((dev->escc_rbuf != NULL) && (dev->escc_rbuf[0].frame != NULL))
			vfree(dev->escc_rbuf[0].frame);
		if ((dev->escc_tbuf != NULL) && (dev->escc_tbuf[0].frame != NULL))
			vfree(dev->escc_tbuf[0].frame);
		if (dev->escc_rbuf != NULL)
			vfree(dev->escc_rbuf);
		if (dev->escc_tbuf != NULL)
			vfree(dev->escc_tbuf);
		// allocate memory for buffers

		dev->escc_rbuf = (struct buf *)vmalloc(dev->settings.n_rbufs * sizeof(struct buf));
		if (dev->escc_rbuf == NULL)
			return -ENOMEM;
		dev->escc_tbuf = (struct buf *)vmalloc(dev->settings.n_tbufs * sizeof(struct buf));
		if (dev->escc_tbuf == NULL)
		{
			vfree(dev->escc_rbuf);
			return -ENOMEM;
		}
		tempbuf = (char *)vmalloc(dev->settings.n_rbufs * dev->settings.n_rfsize_max);
		if (tempbuf == NULL)
		{
			vfree(dev->escc_rbuf);
			vfree(dev->escc_tbuf);
			return -ENOMEM;
		}
		for (i = 0; i < dev->settings.n_rbufs; i++)
		{
			dev->escc_rbuf[i].frame = &tempbuf[i * dev->settings.n_rfsize_max];
			dev->escc_rbuf[i].valid = 0;
			dev->escc_rbuf[i].no_bytes = 0;
			dev->escc_rbuf[i].max = 0;
		}
		tempbuf = (char *)vmalloc(dev->settings.n_tbufs * dev->settings.n_tfsize_max);
		if (tempbuf == NULL)
		{
			vfree(dev->escc_rbuf[0].frame);
			vfree(dev->escc_rbuf);
			vfree(dev->escc_tbuf);
			return -ENOMEM;
		}
		for (i = 0; i < dev->settings.n_tbufs; i++)
		{
			dev->escc_tbuf[i].frame = &tempbuf[i * dev->settings.n_tfsize_max];
			dev->escc_tbuf[i].valid = 0;
			dev->escc_tbuf[i].no_bytes = 0;
			dev->escc_tbuf[i].max = 0;
		}
		atomic_set(&dev->transmit_frames_available, dev->settings.n_tbufs);
		atomic_set(&dev->received_frames_pending, 0);
		// make the board live, enable interrupts

		dev->port_initialized = 1;
		outb(dev->settings.imr0, IMR0);
		outb(dev->settings.imr1, IMR1);
		outb(dev->settings.pim, PIM);

		if ((dev->settings.ccr0 & 0x03) == 3)
		{
			// bisync-enter hunt mode
			// wait with timeout;
			timeoutvalue = 0;
			do
			{
				timeoutvalue++;
				if ((inb(STAR) & CEC) == 0x00)
					break;
			} while (timeoutvalue < MAX_TIMEOUT_VALUE);
			if (timeoutvalue >= MAX_TIMEOUT_VALUE)
			{
				PDEBUG("TIMEOUT in waitforCEC-hunt\n");
				outb(dev->settings.imr0, 0xff);
				outb(dev->settings.imr1, 0xff);
				outb(dev->settings.pim, 0xff); // could fubar ch2/ch1 DTR/DSR interrupt settings
				dev->port_initialized = 0;
				return -EBUSY;
			}

			outb(HUNT, CMDR);
		}

		PDEBUG("INIT_COMPLETE\n");
		break;

	case ESCC_READ_REGISTER:
		if (dev->base == 0)
			return -ENODEV;

		if (copy_from_user(&regs, (regsingle *)arg, sizeof(regsingle)))
			return -EFAULT;
		regs.data = inb(dev->base + regs.port);

		PDEBUG("READ_REGISTER in:%x ->%x\n", (unsigned)(regs.port + dev->base), regs.data);

		if (copy_to_user((regsingle *)arg, &regs, sizeof(regsingle)))
			return -EFAULT;
		break;

	case ESCC_WRITE_REGISTER:
		// this function should not be used to change the MODE or CCR0 registers, as they
		// are used in various routines to determine driver functions (from dev->settings).
		// specifically you should not change from ASYNC/HDLC/BISYNC or from transparent HDLC t
		// extended transparent HDLC modes on the fly without calling the ESCC_INIT function.
		if (dev->base == 0)
			return -ENODEV;

		if (copy_from_user(&regs, (regsingle *)arg, sizeof(regsingle)))
			return -EFAULT;

		PDEBUG("WRITE_REGISTER out:%x ->%x\n", regs.data, (unsigned)(regs.port + dev->base));

		outb(regs.data, dev->base + regs.port);
		break;

	case ESCC_STATUS:
		// returns reflected IRQ status messages from the ISR to user space
		// This is the only ioctl that will block.
		// If you call this without _O_NONBLOCK you must force a status event
		// there is not currently a mechanism in place to cancel this operation
		// if you find yourself in this situation, run the flushtx executable,
		// or call the _FLUSH_TX ioctl, as it will generate a status event (transmit complete)

	readstatusagain:
#ifdef USE_2_6
		spin_lock_irqsave(&dev->irqlock, flags);
#else
		save_flags(flags); // stop interrupts
		cli();
#endif
		status = dev->status; // get status from driver info area
		dev->status = 0;	  // clear it
#ifdef USE_2_6
		spin_unlock_irqrestore(&dev->irqlock, flags);
#else
		restore_flags(flags); // enable interrupts
#endif
		if (status == 0)
		{
			if (filp->f_flags & O_NONBLOCK)
			{
				// if given as O_NONBLOCK then don't block here
			}
			else
			{
				// not nonblocking, status is 0, so block until status changes
#ifdef USE_2_6
				prepare_to_wait(&dev->sq, &wait, TASK_INTERRUPTIBLE);
				schedule();
				finish_wait(&dev->sq, &wait); // I believe we hang here until dev->sq is signaled
#else
				save_flags(flags);
				cli();
				interruptible_sleep_on(&dev->sq);
				restore_flags(flags);
#endif
				goto readstatusagain; // get the new status and return it
			}
		}

		if (copy_to_user((unsigned long *)arg, &status, sizeof(unsigned long)))
			return -EFAULT;

		PDEBUG("STATUS:%lx\n", status);

		break;

	case ESCC_REMOVE_BOARD:
		return -ENODEV; // cant remove the PCI boards
		break;

	case ESCC_ADD_BOARD:
		return -ENODEV; // can't add PCI boards after the fact
		break;

	case ESCC_TX_ACTIVE:

		PDEBUG("TX_ACTIVE:%u\n", dev->is_transmitting);

		if (dev->is_transmitting == 0)
			put_user(0, (unsigned long *)arg);
		else
			put_user(1, (unsigned long *)arg);
		break;

	case ESCC_RX_READY:

		PDEBUG("RX_READY:%u\n", atomic_read(&dev->received_frames_pending));

		put_user(atomic_read(&dev->received_frames_pending), (unsigned long *)arg);
		break;

	case ESCC_START_TIMER:
		// function to start the 82532 timer
		// you can obtain the same results using ESCC_WRITE_REGISTER
		if (dev->base == 0)
			return -ENODEV;

		PDEBUG("START_TIMER\n");
		WAIT_WITH_TIMEOUT;
		outb(STIB, CMDR);
		break;

	case ESCC_STOP_TIMER:
		// function to stop the 82532 timer
		// you can obtain the same results using ESCC_WRITE_REGISTER
		if (dev->base == 0)
			return -ENODEV;

		PDEBUG("STOP_TIMER\n");
		outb(dev->settings.timr, TIMR);
		break;

	case ESCC_SET_TX_TYPE:
		// used to change the command used to send frames in HDLC modes,
		// can send I frames or transparent frames,
		// I frames only make sense if in Auto mode.
		// transparent frames are normal for transparent HDLC modes, and will
		// send U frames if in Auto mode.

		get_user(passval, (unsigned long *)arg);
		if ((passval != XTF) && (passval != XIF))
			return -EINVAL;

		PDEBUG("SET_TX_TYPE:%x\n", (unsigned)passval);

		if (passval == XIF)
			dev->tx_type = XIF;
		if (passval == XTF)
			dev->tx_type = XTF;

		break;

	case ESCC_SET_TX_ADDRESS:
		// used to set the transmit address, for modes that append addresses automatically
		// this can be done just as easilly using ESCC_WRITE_REGISTER
		if (dev->base == 0)
			return -ENODEV;

		if (dev->is_transmitting == 1)
			return -EBUSY;
		get_user(passval, (unsigned long *)arg);

		PDEBUG("SET_TX_ADD:%x\n", (unsigned)passval);

		outb((passval >> 8) & 0xfc, XAD1);
		outb((passval & 0xff), XAD2);

		break;

	case ESCC_FLUSH_RX:
		// used to wipe all of the drivers receive frame buffers, also issues a RHR command to
		// the 82532

		if (dev->base == 0)
			return -ENODEV;

		if (dev->port_initialized == 0)
			return -ENODEV; // could use a better error value, needs to convey that the port has not been initialized yet
		PDEBUG("FLUSH_RX\n");
#ifdef USE_2_6
		spin_lock_irqsave(&dev->irqlock, flags);
#else
		save_flags(flags); // stop interrupts
		cli();
#endif
		WAIT_WITH_TIMEOUT;
		outb(RHR, CMDR);
		for (i = 0; i < dev->settings.n_rbufs; i++)
		{
			dev->escc_rbuf[i].valid = 0;
			dev->escc_rbuf[i].no_bytes = 0;
		}
		dev->current_rxbuf = 0;
		atomic_set(&dev->received_frames_pending, 0);
#ifdef USE_2_6
		spin_unlock_irqrestore(&dev->irqlock, flags);
#else
		restore_flags(flags); // enable interrupts
#endif
		wake_up_interruptible((&dev->rq));

		break;

	case ESCC_FLUSH_TX:
		// used to wipe all of the drivers transmit frame buffers, also issues an XRES command
		// to the 82532
		if (dev->base == 0)
			return -ENODEV;

		if (dev->port_initialized == 0)
			return -ENODEV; // could use a better error value, needs to convey that the port has not been initialized yet
		PDEBUG("FLUSH_TX\n");
#ifdef USE_2_6
		spin_lock_irqsave(&dev->irqlock, flags);
#else
		save_flags(flags); // stop interrupts
		cli();
#endif
		for (i = 0; i < dev->settings.n_tbufs; i++)
		{
			dev->escc_tbuf[i].valid = 0;
			dev->escc_tbuf[i].no_bytes = 0;
		}
		dev->current_txbuf = 0;
		dev->is_transmitting = 0;
		atomic_set(&dev->transmit_frames_available, dev->settings.n_tbufs);
		WAIT_WITH_TIMEOUT;
		outb(XRES, CMDR);
#ifdef USE_2_6
		spin_unlock_irqrestore(&dev->irqlock, flags);
#else
		restore_flags(flags); // enable interrupts
#endif
		wake_up_interruptible(&dev->wq);
		break;

	case ESCC_CMDR:
		// Can also be done with ESCC_WRITE_REGISTER
		if (dev->base == 0)
			return -ENODEV;

		get_user(passval, (unsigned long *)arg);
		PDEBUG("ESCC_CMDR:%x\n", (unsigned)passval);
		WAIT_WITH_TIMEOUT;
		outb((char)passval, CMDR);
		break;

	case ESCC_GET_DSR:
		if (dev->base == 0)
			return -ENODEV;

		// no need to lock the board, as the PVR register
		// is a single register mapped to both channels
		// as long as we are not writing it it doesn't need
		// to be protected
		if (dev->channel == 0)
		{
			put_user((inb(PVR) >> 5) & 1, (unsigned long *)arg);
		}
		if (dev->channel == 1)
		{
			put_user((inb(PVR) >> 6) & 1, (unsigned long *)arg);
		}
		PDEBUG("GET_DSR:%x\n", inb(PVR));
		break;

	case ESCC_SET_DTR:
		if (dev->base == 0)
			return -ENODEV;

		get_user(passval, (unsigned long *)arg);
		PDEBUG("SET_DTR:%lu\n", passval);
		down(&boardlock[dev->board]); // must protect the read/write from the other board
									  // as the PVR register is shared between both channels

		if (dev->channel == 0)
		{
			outb((inb(PVR) & 0xF7) | ((passval & 1) << 3), PVR);
		}
		if (dev->channel == 1)
		{
			outb((inb(PVR) & 0xEF) | ((passval & 1) << 4), PVR);
		}
		up(&boardlock[dev->board]);
		break;

	case ESCC_SET_CLOCK:
		if (dev->base == 0)
			return -ENODEV;

		if (copy_from_user(&clock, (clkset *)arg, sizeof(clkset)))
			return -EFAULT;
		PDEBUG("SETCLOCK:hval:%lx , nmbits:%u\n", clock.clockbits, clock.numbits);
		down(&boardlock[dev->board]); // it doesn't matter which channel sets the clock, but
									  // if both try to do it at the same time bad things will
									  // happen.
									  // BTW if you didn't know there is only ONE clock generator
									  // that is shared between the two ESCC channels.
									  // The output of the ICD2053B is connected to the 82532
									  // OSC input, there is only one OSC input on the 82532
									  // so you only get to pick one frequency for both channels
									  // if you can't deal with this then use an external
									  // clock routed in through ST or RT and use a clock mode
									  // that uses rxclk and/or txclk as the clock source instead
									  // of the BGR/DPLL/OSC.
		set_clock_generator(port, clock.clockbits, clock.numbits);
		set_clock_generator(port, clock.clockbits, clock.numbits);
		// BUGBUG--For some reason sometimes the first set clock doesn't take...I'm still working
		// on the cause, doing it twice usually works.
		up(&boardlock[dev->board]);
		break;

	case ESCC_IMMEDIATE_STATUS:
// this one allways returns immediatly with 0
// if there are no status events
//(the same as calling _STATUS opened as _O_NONBLOCK)
#ifdef USE_2_6
		spin_lock_irqsave(&dev->irqlock, flags);
#else
		save_flags(flags); // stop interrupts
		cli();
#endif
		status = dev->status;
		dev->status = 0;
#ifdef USE_2_6
		spin_unlock_irqrestore(&dev->irqlock, flags);
#else
		restore_flags(flags); // enable interrupts
#endif
		if (copy_to_user((unsigned long *)arg, &status, sizeof(unsigned long)))
			return -EFAULT;

		PDEBUG("IMMEDIATE_STATUS:%lx\n", status);

		break;
	case ESCC_CLEAR_STUCK_IRQ:
		// don't need this whole function for version 3 hardware
		for (i = 0; i < escc_nr_devs; i++)
		{
			// fill in usefull info here, displayed in /proc/esccdrv via 'cat /proc/esccdev'
			d = &escc_devices[i];
			port = d->base;
			if (port != 0)
			{
				inb(ISR0);
				inb(ISR1);
				inb(PIS);
#ifdef VER_3_0
#else
				outb(0x02, d->amcc + 0x3a);
				inb(d->amcc + 0x1f);
#endif
			}
		}
		break;
	case ESCC_TX_BYTES_LEFT:
		// could disable interrupts for this, but depending on freq is a bad idea
		txbytesleft = 0;
		for (i = 0; i < dev->settings.n_tbufs; i++)
		{
			tbuf = &dev->escc_tbuf[i];
			if (tbuf->valid == 1)
				txbytesleft += (tbuf->max - tbuf->no_bytes);
		}
		if (copy_to_user((unsigned long *)arg, &txbytesleft, sizeof(unsigned long)))
			return -EFAULT;

		break;

	case ESCC_SET_FREQ:
		if (dev->base == 0)
			return -ENODEV;

		if (copy_from_user(&dev->freq, (unsigned long *)arg, sizeof(unsigned long)))
			return -EFAULT;
		PDEBUG("SETFREQ:freq:%ld \n", dev->freq);
		down(&boardlock[dev->board]); // it doesn't matter which channel sets the clock, but
									  // if both try to do it at the same time bad things will
									  // happen.
									  // BTW if you didn't know there is only ONE clock generator
									  // that is shared between the two ESCC channels.
									  // The output of the ICS307 is connected to the 82532
									  // OSC input, there is only one OSC input on the 82532
									  // so you only get to pick one frequency for both channels
									  // if you can't deal with this then use an external
									  // clock routed in through ST or RT and use a clock mode
									  // that uses rxclk and/or txclk as the clock source instead
									  // of the BGR/DPLL/OSC.

		if (GetICS307Bits(dev->freq, &icsbits, &actual) == 0) // get programming value
		{
			setics307clock(port, icsbits); // pump it to the device
		}
		up(&boardlock[dev->board]);
		break;
	case ESCC_GET_FREQ:
		if (dev->base == 0)
			return -ENODEV;
		if (copy_to_user((unsigned long *)arg, &dev->freq, sizeof(unsigned long)))
			return -EFAULT;
		break;
	case ESCC_GET_FEATURES:
		if (dev->base == 0)
			return -ENODEV;
		features = inl(dev->amcc);
		if (dev->channel == 0)
			features = (features & 0xf) | ((features >> 4) & 0x30);
		else
			features = ((features & 0xf0) >> 4) | ((features >> 6) & 0x30);

		if (copy_to_user((unsigned long *)arg, &features, sizeof(unsigned long)))
			return -EFAULT;
		break;
	case ESCC_SET_FEATURES:
		if (dev->base == 0)
			return -ENODEV;
		if (copy_from_user(&dev->features, (clkset *)arg, sizeof(unsigned long)))
			return -EFAULT;
		features = inl(dev->amcc);

		if (dev->channel == 0)
		{
			features = features & (0xFFFFFcF0);			 // mask out channel 0 bits
			features = features | (dev->features & 0xf); // set incomming bits for channel 0,
			features = features | ((dev->features & 0x30) << 4);
			// assumed incomming data of format:
			// bit0 = rxechodisable
			// bit1 = sd485
			// bit2 = tt485
			// bit3 = ctsdisable
			// bit4 = txclk=st
			// bit5 = txclk=tt
		}
		else
		{
			features = features & (0xFFFFF30F);					// mask out channel 1 bits
			features = features | ((dev->features & 0xf) << 4); // set new values
			features = features | ((dev->features & 0x30) << 6);
		}
		outl(features, dev->amcc);
		break;

	default: /* redundant, as cmd was checked against MAXNR */
		return -EINVAL;
	}
	return 0;
}
