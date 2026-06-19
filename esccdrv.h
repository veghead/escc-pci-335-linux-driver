/******************************************************
 *
 * Copyright (C) 2000 Commtech, Inc. Wichita KS
 *
 * esccdrv.h -- header file for escc-pci module
 *
 * Tested with Linux version 2.2 12
 ******************************************************/

/* $Id: esccdrv.h,v 1.16 2005/05/19 20:33:45 carl Exp $ */

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
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/uidgid.h>

/* version dependencies have been confined to a separate file */

#define VERSION_CODE(vers,rel,seq) ( ((vers)<<16) | ((rel)<<8) | (seq) )
#include "sysdep.h"

//define if using 2.6 kernel series.--possibly rework with VERSION_CODE, is version code still there in 2.6?
#if  LINUX_VERSION_CODE >= VERSION_CODE(2,6,0)
#define USE_2_6
#include <linux/interrupt.h>
#endif

//for debug messages
//#define ESCC_DEBUG
 
//for esccpdrv entry in /proc
#define ESCC_USE_PROC 
//for latest rev of ESCC-PCI card (Fastcom:ESCC-PCI-335)
#define VER_3_0

//defining this will flush the receiver on a RDO error, and will flush the transmitter on a XDU/EXE error.  This is necessary if you are not handling errors (monitoring status) such as with the ppp example code
//#define FLUSH_ON_ERROR

//defining this will only allow one open to succeed at a time on any port
//#define ONLY_ONE_OPEN_AT_A_TIME


//this will turn off the receiver function while transmitting, effectively
//canceling the echo on 2 wire 485 nets
//#define RS845_ECHO_CANCEL

//this will force the rts on before sending a frame
//note it should not be necessary, as the default function of the 82532 is to 
//frame the outgoing data with RTS
//it can be used to keep the rts line on longer (either prior to the frame send
//or after the frame is sent)
//#define FORCE_485_RTS_CONTROL

//if WAIT_FOR_CEC is defined the WAIT_WITH_TIMEOUT macro will check the status
//othewise it does nothing
#define WAIT_FOR_CEC

/*
 * Macros to help debugging
 */

#undef PDEBUG             /* undef it, just in case */
#ifdef ESCC_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_INFO "esccp: " fmt,## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */

#ifndef ESCC_MAJOR
# define ESCC_MAJOR 0    /* dynamic major by default on modern kernels */
#endif


#ifdef VER_3_0
#define ESCCP_VENDOR_ID 0x18f7
#define ESCCP_DEVICE_ID 0x0001
#else
#define ESCCP_VENDOR_ID 0x10E8
#define ESCCP_DEVICE_ID 0x814c
#endif
#ifndef MAX_ESCC_BOARDS
#define MAX_ESCC_BOARDS 3    /* escc0 - escc6 (two per board) */
#endif
#define MAX_TIMEOUT_VALUE 1000000  //max value for WAIT_WITH_TIMEOUT

#define TRUE 1
#define FALSE 0

/*
 */
//structure for buffered frames
struct buf{
unsigned valid;         //indicator 1 = frame[] has data, 0 = frame[] has ???
unsigned no_bytes;      //number of bytes in frame[]
unsigned max;           //maximum number of bytes to send/receive
unsigned char *frame;  //data array for received/transmitted data
};


typedef struct escc_setup{
//used in all
unsigned cmdr;
unsigned mode;
unsigned timr;
unsigned xbcl;
unsigned xbch;
unsigned ccr0;
unsigned ccr1;
unsigned ccr2;
unsigned ccr3;
unsigned ccr4;
unsigned tsax;
unsigned tsar;
unsigned xccr;
unsigned rccr;
unsigned bgr;
unsigned iva;
unsigned ipc;
unsigned imr0;
unsigned imr1;
unsigned pvr;
unsigned pim;
unsigned pcr;
//escc register defines for HDLC/SDLC mode
unsigned xad1;
unsigned xad2;
unsigned rah1;
unsigned rah2;
unsigned ral1;
unsigned ral2;
unsigned rlcr;
unsigned aml;
unsigned amh;
unsigned pre;
//escc async register defines (used in bisync as well)
unsigned xon;
unsigned xoff;
unsigned tcr;
unsigned dafo;
unsigned rfc;
unsigned tic;
unsigned mxn;
unsigned mxf;

//escc bisync register defines
unsigned synl;
unsigned synh;

unsigned n_rbufs;
unsigned n_tbufs;
unsigned n_rfsize_max;
unsigned n_tfsize_max;

}setup;

typedef struct escc_clkset{
	unsigned long clockbits;
	unsigned  numbits;
}clkset;

typedef struct escc_regsingle{
	unsigned long port;		//offset from base address of register to access
	unsigned char data;		//data to write
}regsingle;

typedef struct board_settings{
	unsigned base;
	unsigned irq;
	unsigned channel;
	unsigned dmar;//not used for PCI
	unsigned dmat;//not used for PCI
	unsigned amcc;
} board_settings;

typedef struct Escc_Dev {
//82532 register settings
setup settings;
//icd2053b settings
clkset clock;
//board switches
unsigned amcc;
unsigned base;
unsigned irq;
unsigned channel;
unsigned dmar;
unsigned dmat;
unsigned board;//holds board number (ch0 and  ch1 have same board number)
               //this is used to lock access to the board if 
               //both channels access at the same time
               //in conjunction with the global atomic_t boardlock[] and 
               //the per device wboardlock wait queue 
unsigned dev_no;//only needed to check against previous in addboard
//add other per port info here
struct Escc_Dev *complement_dev;//pointer to dev of other channel on this board

unsigned tx_type;
//state flags
unsigned irq_hooked;
unsigned port_initialized;

//interrupt status flags
unsigned long status;

//wait queues
#if LINUX_VERSION_CODE >= VERSION_CODE(2,4,0)
wait_queue_head_t rq;
wait_queue_head_t wq;
wait_queue_head_t sq;
#else
struct wait_queue *rq;
struct wait_queue *wq;
struct wait_queue *sq;
#endif

//buffering params

struct buf *escc_rbuf;
struct buf *escc_tbuf;
unsigned current_rxbuf;
unsigned current_txbuf;
unsigned is_transmitting;
atomic_t received_frames_pending;
atomic_t transmit_frames_available;

unsigned long escc_u_count;
kuid_t escc_u_owner;
unsigned long freq;
unsigned long features;
spinlock_t irqlock;
} Escc_Dev;

/*
 * Split minors in two parts
 */
#define TYPE(dev)   (MINOR(dev) >> 4)  /* high nibble */
#define NUM(dev)    (MINOR(dev) & 0xf) /* low  nibble */

/*
 * Different minors behave differently, so let's use multiple fops
 */

extern struct file_operations escc_fops;        /* simplest: global */

/*
 * The different configurable parameters
 */
extern int escc_major;     /* main.c */
extern int escc_nr_devs;
extern unsigned long status;
extern clkset clock;
extern Escc_Dev *escc_devices; 
extern int used_board_numbers[MAX_ESCC_BOARDS];
extern struct semaphore boardlock[MAX_ESCC_BOARDS];
/*
 * Prototypes for shared functions
 */



ssize_t escc_read (struct file *filp,
                char *buf, size_t count,loff_t *off);
ssize_t escc_write (struct file *filp,
                const char *buf, size_t count,loff_t *off);
loff_t escc_lseek (struct file *filp,
                 loff_t off, int whence);
long escc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int escc_open (struct inode *inode, struct file *filp);
int escc_release (struct inode *inode, struct file *filp);

int escc_fsync(struct file *filp, loff_t start, loff_t end, int datasync);

unsigned int escc_poll(struct file *filp, poll_table *pt);

irqreturn_t escc_irq(int irq, void *dev_id);

void set_clock_generator(unsigned port, unsigned long hval,unsigned nmbits);
int add_port(board_settings *board_switches, Escc_Dev *dev);
unsigned long GetICD2053Freq(clkset clock);
void convertlsb2msb(unsigned char *data, unsigned size);
uint64_t get__tsc(void);
void setics307clock(unsigned port, unsigned long hval);
int GetICS307Bits(unsigned long desired,unsigned long *bits,unsigned long *actual);
unsigned long GetICD2053Freq(clkset clock);
int  GetICD2053Bits(unsigned long desired,clkset *clk);


#ifndef min
#  define min(a,b) ((a)<(b) ? (a) : (b))
#endif
typedef unsigned long status_t;
 /* Ioctl definitions
 */

/* Use 'k' as magic number */
#define ESCC_IOC_MAGIC  'k'
#define ESCC_INIT           _IOW(ESCC_IOC_MAGIC,  1, setup)
#define ESCC_READ_REGISTER  _IOWR(ESCC_IOC_MAGIC,   2, regsingle)
#define ESCC_WRITE_REGISTER _IOWR(ESCC_IOC_MAGIC,   3, regsingle)
#define ESCC_STATUS         _IOR(ESCC_IOC_MAGIC,  4, status_t)
#define ESCC_ADD_BOARD      _IOR(ESCC_IOC_MAGIC,  5, board_settings)
#define ESCC_TX_ACTIVE      _IOR(ESCC_IOC_MAGIC, 6, status_t)
#define ESCC_RX_READY       _IOR(ESCC_IOC_MAGIC,7, status_t)
#define ESCC_SETUP  ESCC_INIT
#define ESCC_START_TIMER    _IO(ESCC_IOC_MAGIC,8)
#define ESCC_STOP_TIMER     _IO(ESCC_IOC_MAGIC,9)
#define ESCC_SET_TX_TYPE    _IOW(ESCC_IOC_MAGIC,10,status_t)
#define ESCC_SET_TX_ADDRESS _IOW(ESCC_IOC_MAGIC,11,status_t)
#define ESCC_FLUSH_RX       _IO(ESCC_IOC_MAGIC,12)
#define ESCC_FLUSH_TX       _IO(ESCC_IOC_MAGIC,13)
#define ESCC_CMDR           _IOW(ESCC_IOC_MAGIC,14,status_t)
#define ESCC_GET_DSR        _IOWR(ESCC_IOC_MAGIC,15,status_t)
#define ESCC_SET_DTR        _IOW(ESCC_IOC_MAGIC,16,status_t) 
#define ESCC_SET_CLOCK      _IOW(ESCC_IOC_MAGIC,17,clkset)
#define ESCC_IMMEDIATE_STATUS _IOR(ESCC_IOC_MAGIC,18,status_t)
#define ESCC_REMOVE_BOARD    _IO(ESCC_IOC_MAGIC,19)
#define ESCC_CLEAR_STUCK_IRQ _IO(ESCC_IOC_MAGIC,20)
#define ESCC_TX_BYTES_LEFT   _IOR(ESCC_IOC_MAGIC,21,status_t)

#define ESCC_SET_FREQ   _IOW(ESCC_IOC_MAGIC,22,status_t)
#define ESCC_GET_FREQ   _IOR(ESCC_IOC_MAGIC,23,status_t)
#define ESCC_SET_FEATURES   _IOW(ESCC_IOC_MAGIC,24,status_t)
#define ESCC_GET_FEATURES   _IOR(ESCC_IOC_MAGIC,25,status_t)


#define ESCC_IOC_MAXNR 25


//CMDR commands
#define XRES 1
#define XME 2
#define XIF 4
#define HUNT 4
#define XTF 8
#define STIB 16
#define RNR 32
#define RFRD 32
#define RHR 64
#define RMC 128
//STAR codes
#define WFA 1

#define CTS 2
#define CEC 4
#define RLI 8
#define TEC 8
#define RRNR 16
#define SYNC 16
#define FCS 16
#define XRNR 32
#define RFNE 32
#define XFW 64
#define XDOV 128
//ISR0
#define RPF 1
#define RFO 2
#define CDSC 4
#define PLLA 8
#define PCE 16
#define RSC 32
#define RFS 64
#define RME 128
//ISR1
#define XPR 1
#define XMR 2
#define CSC 4
#define TIN 8
#define EXE 16
#define XDU 16
#define ALLS 32
#define AOLP 32
#define RDO 64
#define OLP 64
#define EOP 128


//port addresses
//hdlc defines
#define STAR port +0x20
#define CMDR port +0x20
#define RSTA port+0x21
#define PRE port+0x21
#define MODE port+0x22
#define TIMR port+0x23
#define XAD1 port+0x24
#define XAD2 port+0x25
#define RAH1 port+0x26
#define RAH2 port+0x27
#define RAL1 port+0x28
#define RHCR port+0x29
#define RAL2 port+0x29
#define RBCL port+0x2a
#define XBCL port+0x2a
#define RBCH port+0x2b
#define XBCH port+0x2b
#define CCR0 port+0x2c
#define CCR1 port+0x2d
#define CCR2 port+0x2e
#define CCR3 port+0x2f
#define TSAX port+0x30
#define TSAR port+0x31
#define XCCR port+0x32
#define RCCR port+0x33
#define VSTR port+0x34
#define BGR port+0x34
#define RLCR port+0x35
#define AML port+0x36
#define AMH port+0x37
#define GIS port+0x38
#define IVA port+0x38
#define IPC port+0x39
#define ISR0 port+0x3a
#define IMR0 port+0x3a
#define ISR1 port+0x3b
#define IMR1 port+0x3b
#define PVR port+0x3c
#define PIS port+0x3d
#define PIM port+0x3d
#define PCR port+0x3e
#define CCR4 port+0x3f
#define FIFO port
//async defines
#define XON port+0x24
#define XOFF port+0x25
#define TCR port+0x26
#define DAFO port+0x27
#define RFC port+0x28
#define TIC port+0x35
#define MXN port+0x36
#define MXF port+0x37
//bisync defines
#define SYNL port+0x24
#define SYNH port+0x25

//defines for 2053b
#define STARTWRD 0x1e05
#define MIDWRD   0x1e04
#define ENDWRD   0x1e00


#define OPMODE_HDLC		0
#define OPMODE_ASYNC		3
#define OPMODE_BISYNC		2
#define OPMODE_SDLC_LOOP	1
//device io control STATUS function return values
#define ST_RX_DONE		0x00000001
#define ST_OVF			0x00000002
#define ST_RFS			0x00000004
#define ST_RX_TIMEOUT	0x00000008
#define ST_RSC			0x00000010
#define ST_PERR			0x00000020
#define ST_PCE			0x00000040
#define ST_FERR			0x00000080
#define ST_SYN			0x00000100
#define ST_DPLLA		0x00000200
#define ST_CDSC			0x00000400
#define ST_RFO			0x00000800
#define ST_EOP			0x00001000
#define ST_BRKD			0x00002000
#define ST_ONLP			0x00004000
#define ST_BRKT			0x00008000
#define	ST_ALLS			0x00010000
#define ST_EXE			0x00020000
#define ST_TIN			0x00040000
#define ST_CTSC			0x00080000
#define ST_XMR			0x00100000
#define ST_TX_DONE		0x00200000
#define ST_DMA_TC		0x00400000
#define ST_DSR1C		0x00800000
#define ST_DSR0C		0x01000000
#define ST_FUBAR_IRQ    0x02000000

#ifdef WAIT_FOR_CEC                         
#define WAIT_WITH_TIMEOUT				\
do							\
	{						\
	unsigned long timeout_val;				\
    timeout_val = 0;					\
	do						\
		{					\
		timeout_val++;				\
		if((inb(STAR)&CEC)==0x00) break;        \
		}while(timeout_val<MAX_TIMEOUT_VALUE);		\
		if(timeout_val>=MAX_TIMEOUT_VALUE) PDEBUG("WAIT_WITH_TIMEOUT--TIMED OUT\n");   \
	}while(0)
#else
#define WAIT_WITH_TIMEOUT
#endif    

