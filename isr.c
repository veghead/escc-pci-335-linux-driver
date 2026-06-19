/******************************************************
 *
 * Copyright (C) 2000 Commtech, Inc. Wichita KS
 *
 * isr.c -- Interrupt service routine for escc-pci module
 *
 * Tested with Linux version 2.2 12
 ******************************************************/

/* $Id: isr.c,v 1.9 2005/05/19 19:51:17 carl Exp $ */

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
#include <linux/fs.h>	 /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/ioport.h>
#include <linux/sched.h>
#include <asm/io.h>
#include "esccdrv.h" /* local definitions */

irqreturn_t escc_irq(int irq, void *dev_id)
{
	Escc_Dev *dev;
	struct buf *rbuf;
	struct buf *tbuf;
	// unsigned i;
	unsigned j = 0;
	unsigned port;
	unsigned gis;
	unsigned isr0;
	unsigned isr1;
	unsigned pis;
	unsigned opmode;
	unsigned extendedtransparent;
	unsigned isrprt;
	int irqhit;
	unsigned long flags;
	unsigned handled_an_int = 0;

topofisr:
	irqhit = 0;
	for (isrprt = 0; isrprt < escc_nr_devs; isrprt++)
	{
		dev = &escc_devices[isrprt];
		//	dev = (Escc_Dev *)dev_id;
	restart_isr:
		port = dev->base;

		PDEBUG("irq\n");
#ifdef VER_3_0
#else
		// don't care if it is ours or not, check/clear the AMCC
		// clear the AMCC interrupt
		if ((inb(dev->amcc + 0x3a) & 0x80) == 0x80) // get IRQ status
		{
			// only clear if irq pending (set)
			outb(0x02, dev->amcc + 0x3a); // reset Mailbox IRQ
			inb(dev->amcc + 0x1f);		  // read MB4B3
		}
#endif
		gis = inb(GIS);
		if (gis != 0)
		{
			irqhit = 1;
			// ok it is either us or the other channel attached to this base address,
			// check ours
			// get interrupt status(s)
			isr0 = inb(ISR0);
			isr1 = inb(ISR1);
			pis = inb(PIS);

			if ((isr0 + isr1 + pis) == 0)
			{
				dev = dev->complement_dev;
				goto restart_isr;
				// return;//must be other channel, so let them have at it
			}

			// it is ours so here we go.
			rbuf = &dev->escc_rbuf[dev->current_rxbuf];
			tbuf = &dev->escc_tbuf[dev->current_txbuf];

			opmode = dev->settings.ccr0 & 0x3; // 0 = HDLC, 1 = SDLCloop, 2 = BISYNC, 3 = ASYNC

			if ((opmode == OPMODE_HDLC) && ((dev->settings.mode & 0xc0) == 0xc0))
				extendedtransparent = TRUE;
			else
				extendedtransparent = FALSE;

			PDEBUG("%x:%x:%x\n", isr0, isr1, pis);
			handled_an_int = 1;

			// do
			//  {
			if (dev->port_initialized == 1)
			{
#ifdef USE_2_6
				spin_lock_irqsave(&dev->irqlock, flags);
#endif

				// we are inited here, so we have buffer space, service the interrupt
				if ((isr0 & RPF) == RPF)
				{
					if (opmode == OPMODE_HDLC)
					{
						if ((rbuf->no_bytes + 32) > dev->settings.n_rfsize_max)
						{
							// received bytes would overfill buffer
							if (extendedtransparent)
							{
								// could be normal for extended transparent mode so close up the
								// buffer and return it
								rbuf->valid = 1;
								atomic_inc(&dev->received_frames_pending);
								dev->status |= ST_RX_DONE;
								dev->current_rxbuf++;
								if (dev->current_rxbuf == dev->settings.n_rbufs)
									dev->current_rxbuf = 0;
								rbuf = &dev->escc_rbuf[dev->current_rxbuf];
								if (rbuf->valid == 1)
								{
									dev->status |= ST_OVF; //
														   // if we don't do this then reads will start failing
														   //(the pending counter will be larger than the number
														   // of frames buffered, so when all the frames
														   // are read the read routine will break at the end
														   // thinking that there are more frames, but never
														   // decrementing the counter because it cant find them)
														   // this is true for all overflow conditions following
									atomic_dec(&dev->received_frames_pending);

								} // end of rbuf->valid == 1
								rbuf->no_bytes = 0;
								rbuf->valid = 0;
								insw(FIFO, &rbuf->frame[rbuf->no_bytes], 16);
								WAIT_WITH_TIMEOUT;
								outb(RMC, CMDR);
								rbuf->no_bytes += 32;
								wake_up_interruptible(&dev->rq);

							} // end of extended transparent
							else
							{
								// we don't have enough room in the buffer to hold the incomming
								// bytes, so
								// is an overflow condition so
								// we just release the fifo without reading it (data is lost here)
								dev->status |= ST_RFO;
								WAIT_WITH_TIMEOUT;
								outb(RMC, CMDR);
							} // end of else (not extended transparent mode)
						} // end of bytes > max
						else
						{
							// bytes will fit in buffer so go get em
							insw(FIFO, &rbuf->frame[rbuf->no_bytes], 16); // pull the 32 bytes
							WAIT_WITH_TIMEOUT;							  // wait for command executing
							outb(RMC, CMDR);							  // release fifo
							rbuf->no_bytes += 32;						  // inc byte count

						} // end of else block -- (bytes <max)
					} // end of if HDLC
					if ((opmode == OPMODE_ASYNC) || (opmode == OPMODE_BISYNC))
					{
						j = inb(RBCL);
						j = j & 0x1f;
						if (j == 0)
							j = 32;

						if (rbuf->no_bytes + j > dev->settings.n_rfsize_max)
						{
							rbuf->valid = 1;
							atomic_inc(&dev->received_frames_pending);
							dev->status |= ST_RX_DONE;
							dev->current_rxbuf++;
							if (dev->current_rxbuf == dev->settings.n_rbufs)
								dev->current_rxbuf = 0;
							rbuf = &dev->escc_rbuf[dev->current_rxbuf];
							if (rbuf->valid == 1)
							{
								dev->status |= ST_OVF;
								atomic_dec(&dev->received_frames_pending);
							} // end of if rbuf->valid ==1
							rbuf->valid = 0;
							rbuf->no_bytes = 0;
							wake_up_interruptible(&dev->rq);
						}

						insb(FIFO, &rbuf->frame[rbuf->no_bytes], j);
						WAIT_WITH_TIMEOUT;
						outb(RMC, CMDR);
						rbuf->no_bytes += j;

					} // end of if async or bisync
					// handler for SDLC_LOOP noticably missing...possibly the same as HDLC?
				} // end of RPF

				if ((isr0 & RME) == RME)
				{

					// PDEBUG("RME\n");
					if (opmode == OPMODE_HDLC)
					{
						// note substitue the following if you have frames greater than 4k
						//	j = ( (inb(RBCH)&0x1f) <<8 )+(inb(RBCL)&0xff);
						j = ((inb(RBCH) & 0x0f) << 8) + (inb(RBCL) & 0xff);
						// PDEBUG("j=%u\n",j);
					} // end of HDLC BLOCK
					if ((opmode == OPMODE_ASYNC) || (opmode == OPMODE_BISYNC))
					{
						// TCD interrupt
						j = inb(RBCL);
						j = j & 0x1f;
						if (j == 0)
							j = 32;
						j = j + rbuf->no_bytes; //(to make compatible with code below)
					}

					if (j > dev->settings.n_rfsize_max)
					{
						dev->status |= ST_RFO; // data will be lost
						j = dev->settings.n_rfsize_max;
						PDEBUG("j>max\n");
					} // endof if j out of bounds
					if (j < rbuf->no_bytes)
					{
						dev->status |= RFO;
						PDEBUG("j<nobytes\n");
					}
					else
					{
						insb(FIFO, &rbuf->frame[rbuf->no_bytes], j - rbuf->no_bytes);
						rbuf->no_bytes = j;
						WAIT_WITH_TIMEOUT;
						outb(RMC, CMDR);

						rbuf->valid = 1;
						atomic_inc(&dev->received_frames_pending);
						dev->status |= ST_RX_DONE;
						dev->current_rxbuf++;
						if (dev->current_rxbuf == dev->settings.n_rbufs)
							dev->current_rxbuf = 0;
						rbuf = &dev->escc_rbuf[dev->current_rxbuf];
						if (rbuf->valid == 1)
						{
							dev->status |= ST_OVF;
							atomic_dec(&dev->received_frames_pending);
						}
						rbuf->valid = 0;
						rbuf->no_bytes = 0;
						wake_up_interruptible(&dev->rq);
						// PDEBUG("wakeup\n");
					} // end of else block (j in bounds)

					if (opmode == OPMODE_BISYNC)
					{

						WAIT_WITH_TIMEOUT;
						outb(HUNT, CMDR);
					}
				} // end of RME
				if ((isr0 & RFS) == RFS)
				{
					if (opmode == OPMODE_HDLC)
						dev->status |= ST_RFS; // receive frame start interrupt
					if (opmode == OPMODE_ASYNC)
					{
						// TIMEOUT interrupt
						if ((inb(STAR) & 0x20) == 0x20)
						{
							WAIT_WITH_TIMEOUT;
							outb(0x20, CMDR); // release fifo bytes(will force TCD int)
						} // end of if bytes in fifo
						else
						{
							// no bytes in fifo, so just finish frame
							if (rbuf->no_bytes > 0)
							{
								rbuf->valid = 1;
								atomic_inc(&dev->received_frames_pending);
								dev->status |= ST_RX_DONE;
								dev->current_rxbuf++;
								if (dev->current_rxbuf == dev->settings.n_rbufs)
									dev->current_rxbuf = 0;
								rbuf = &dev->escc_rbuf[dev->current_rxbuf];
								if (rbuf->valid == 1)
								{
									dev->status |= ST_OVF;
									atomic_dec(&dev->received_frames_pending);
								}
								rbuf->valid = 0;
								rbuf->no_bytes = 0;
								wake_up_interruptible(&dev->rq);
							} // end of rbuf->nobytes>0
						} // end of else block (no bytes in fifo)
					} // end of opmode ASYNC

					// should not get here in BISYNC mode
				} // end of RFS

				if ((isr0 & RSC) == RSC)
				{ // this could be simplified as the constant is the same for all...
					if (opmode == OPMODE_HDLC)
						dev->status |= ST_RSC;
					if (opmode == OPMODE_ASYNC)
						dev->status |= ST_PERR;
					if (opmode == OPMODE_BISYNC)
						dev->status |= ST_PERR;
				} // end of RSC

				if ((isr0 & PCE) == PCE)
				{ // this could be simplified as the constant is the same for all...
					if (opmode == OPMODE_HDLC)
						dev->status |= ST_PCE;
					if (opmode == OPMODE_ASYNC)
						dev->status |= ST_FERR;
					if (opmode == OPMODE_BISYNC)
					{
						dev->status |= ST_SYN;
					}
				} // end of PCE

				if ((isr0 & PLLA) == PLLA)
				{
					dev->status |= ST_DPLLA;

				} // end of PLLA

				if ((isr0 & CDSC) == CDSC)
				{
					dev->status |= ST_CDSC;

				} // end of CDSC

				if ((isr0 & RFO) == RFO)
				{

					dev->status |= ST_RFO;
					WAIT_WITH_TIMEOUT;
					outb(RHR, CMDR);
				} // end of RFO
				if ((isr1 & EXE) == EXE)
				{
					dev->status |= ST_EXE;
					// handle the case where an XDU/EXE occurs durring the write sequence (sending
					// a new frame) where the EXE has caused the transmitter to reset and lock (making the send command dissapear).
					if (extendedtransparent)
					{
						if (dev->is_transmitting == 1)
						{
							if (tbuf->valid == 1)
							{
								if (tbuf->no_bytes <= 32)
								{
									tbuf->no_bytes = 0; // force resend of this frame (write command was lost)
									WAIT_WITH_TIMEOUT;
									outb(XRES, CMDR); // will cause XPR which will resend data
								} // end of if(tbuf->no_bytes<=32)
								else
								{
									printk("ESCCPDRV:Tx fubar, interrupt latency issue, got XDU in middle of frame write\n");
								} // end of else block of if(tbuf->no_bytes<=32)
							} // end of if tbuf->valid==1
							else
							{
								printk("ESCCPDRV:something funny is going on, is_transmitting=1, tbuf->valid==0\n");
							} // end of else block of if tbuf->valid==1
						} // end of if is_transmitting==1
						else
						{
							// is normal end condition, we are done transmitting and have run out of data
						} // end of else block of if(is_transmitting==1)
					} // end of if(extendedtransparent)

// these are necessary to continue, but let user do it with a flush call
#ifdef FLUSH_ON_ERROR
					if (1)
					{
						int i;
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
						wake_up_interruptible(&dev->wq);
					}
#endif
				} // end of EXE

				if ((isr1 & XPR) == XPR)
				{
					if (tbuf->valid == 1)
					{
						if ((tbuf->max - tbuf->no_bytes) > 32)
						{
							// send 32 bytes
							outsw(FIFO, &tbuf->frame[tbuf->no_bytes], 16);
							WAIT_WITH_TIMEOUT;
							outb(dev->tx_type, CMDR);
							tbuf->no_bytes += 32;
							dev->is_transmitting = 1;

						} // end of send 32
						else
						{
							// send < 32 (close frame)

							outsb(FIFO, &tbuf->frame[tbuf->no_bytes], tbuf->max - tbuf->no_bytes);
							WAIT_WITH_TIMEOUT;
							if (opmode == OPMODE_HDLC)
							{
								if (extendedtransparent)
									outb(dev->tx_type, CMDR);
								else
									outb(dev->tx_type + XME, CMDR);
							}
							if (opmode == OPMODE_ASYNC)
								outb(0x08, CMDR);
							if (opmode == OPMODE_BISYNC)
								outb(dev->tx_type + XME, CMDR);
							tbuf->no_bytes = tbuf->max; // formality, not really necessary
							tbuf->valid = 0;			// free it up for later
							atomic_inc(&dev->transmit_frames_available);
							dev->current_txbuf++;
							if (dev->current_txbuf == dev->settings.n_tbufs)
								dev->current_txbuf = 0;
							dev->status |= ST_TX_DONE;
							wake_up_interruptible(&dev->wq);
						} // end of send <32
					} // end of if tbuf->valid ==1
					else
					{

						dev->is_transmitting = 0; // done transmitting (tbuf not valid at interrupt time)
						dev->status |= ST_TX_DONE;
					} // end of tbuf->valid = 0 (else block)
				} // end of XPR
				if ((isr1 & EOP) == EOP)
				{
					if (opmode == OPMODE_HDLC)
						dev->status |= ST_EOP;
					if (opmode == OPMODE_ASYNC)
						dev->status |= ST_BRKD;
					// no function in bisync

				} // end of EOP

				if ((isr1 & RDO) == RDO)
				{
					if (opmode == OPMODE_HDLC)
					{
						dev->status |= ST_ONLP;
#ifdef FLUSH_ON_ERROR
						// issue a flush here!
						WAIT_WITH_TIMEOUT;
						outb(RHR, CMDR);
						for (i = 0; i < dev->settings.n_rbufs; i++)
						{
							dev->escc_rbuf[i].valid = 0;
							dev->escc_rbuf[i].no_bytes = 0;
						}
						dev->current_rxbuf = 0;
						atomic_set(&dev->received_frames_pending, 0);
						wake_up_interruptible((&dev->rq));
#endif
					}
					if (opmode == OPMODE_ASYNC)
						dev->status |= ST_BRKT;
					// no function in bisync
				} // end of RDO

				if ((isr1 & ALLS) == ALLS)
				{
					dev->status |= ST_ALLS;
// uncomment the next line if you are running in 485 mode and do not wish to
// receive what you send
// turn the receiver back on
#ifdef RS485_RX_ECHO_CANCEL
					outb(dev->settings.mode | 0x08, MODE);
#endif
#ifdef FORCE_485_RTS_CONTROL
					outb(dev->settings.mode & 0xFB, MODE);
#endif

				} // end of ALLS

				if ((isr1 & TIN) == TIN)
				{
					dev->status |= ST_TIN;
					//	outb(inb(TIMR),TIMR);
					// a write to the timer register will stop the timer, so a read write will
					// stop further timer interrupts until the timer is started again.
					// under most timer usages this is what you want, however there are those
					// that use the timer to generate a periodic interrupt as to send a fairly
					// precise timed frame send, in which case you want the interrupts to keep
					// occuring. either way doesn't matter to me.  Uncomment it if you only want
					// one timer interrupt per start timer command.

				} // end of TIN

				if ((isr1 & CSC) == CSC)
				{
					dev->status |= ST_CTSC;
				} // end of CSC

				if ((isr1 & XMR) == XMR)
				{
					if (opmode == OPMODE_HDLC)
						dev->status |= ST_XMR;
					if (opmode == OPMODE_BISYNC)
						dev->status |= ST_XMR;
					// shouldn't happnen in async
				} // end of XMR

				if ((pis & 0x80) == 0x80)
				{
					dev->status |= ST_DMA_TC;
					// really should /could use this for early detection of end of dma transfers if need be
				} // end of DMA TC reached

				if ((pis & 0x40) == 0x40)
				{
					dev->status |= ST_DSR1C;

				} // end of DSR1 changed state

				if ((pis & 0x20) == 0x20)
				{
					dev->status |= ST_DSR0C;
				} // end of DSR0 changed state

#ifdef USE_2_6
				spin_unlock_irqrestore(&dev->irqlock, flags);
#endif

			} // end of if initialized
			else
			{
				// if port is not initialized then it would be a bad idea to let the isr
				// have at it, as there are no buffers allocated yet.
				outb(0xff, IMR0);
				outb(0xff, IMR1);
				outb(0xff, PIM);
			}

			// isr0 = inb(ISR0);
			// isr1 = inb(ISR1);
			// pis =  inb(PIS);

			//  }while((isr0+isr1+pis)!=0);

			if (dev->status != 0)
				wake_up_interruptible(&dev->sq);
			goto restart_isr;
		}
	}
	if (irqhit == 1)
		goto topofisr; // keep doing the loop until there are no more interrupts to service
// cant be us because GIS==0
#ifdef USE_2_6
	if (handled_an_int == 0)
		return (IRQ_NONE);
	else
		return (IRQ_HANDLED);
#endif
}
