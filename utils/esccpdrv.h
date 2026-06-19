/* $Id: esccpdrv.h,v 1.7 2004/12/16 18:42:50 carl Exp $ */
/*
Copyright(c) 2003, Commtech, Inc.
esccdrv.h -- user mode definitions

*/

#include <linux/ioctl.h>

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
	unsigned dmar;
	unsigned dmat;
} board_settings;

 /* Ioctl definitions
 */
typedef unsigned long status_t;

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
#define STAR 0 
#define CMDR 0 
#define RSTA 1
#define PRE 1
#define MODE 2
#define TIMR 3
#define XAD1 4
#define XAD2 5
#define RAH1 6
#define RAH2 7
#define RAL1 8
#define RHCR 9
#define RAL2 9
#define RBCL 10
#define XBCL 10
#define RBCH 11
#define XBCH 11
#define CCR0 12
#define CCR1 13
#define CCR2 14
#define CCR3 15
#define TSAX 16
#define TSAR 17
#define XCCR 18
#define RCCR 19
#define VSTR 20
#define BGR 20
#define RLCR 21
#define AML 22
#define AMH 23
#define GIS 24
#define IVA 24
#define IPC 25
#define ISR0 26
#define IMR0 26
#define ISR1 27
#define IMR1 27
#define PVR 28
#define PIS 29
#define PIM 29
#define PCR 30
#define CCR4 31
#define FIFO 32
//async defines
#define XON 4
#define XOFF 5
#define TCR 6
#define DAFO 7
#define RFC 8
#define TIC 21
#define MXN 22
#define MXF 23
//bisync defines
#define SYNL 4
#define SYNH 5

//defines for 2053b
#define STARTWRD 0x1e05
#define MIDWRD   0x1e04
#define ENDWRD   0x1e00

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

