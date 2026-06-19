/* $Id: reconlyp.c,v 1.2 2003/11/11 21:01:54 carl Exp $ */
/*
Copyright(c) 2003, Commtech, Inc.
reconlyp.c -- user mode function to receive data

usage:
 reconlyp port 

 The port can be any valid escc port (0,1) 
 

*/

//#define ESCC
#define ESCCP

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef ESCCP
 #include "../esccpdrv.h"
#endif
#ifdef ESCC
 #include "../esccdrv.h"
#endif
#include <sys/ioctl.h>
#include <errno.h>
#include <math.h>
#include <sys/poll.h>

void decode_status(unsigned long stat);

int main(int argc, char * argv[])
{
char buf[4096];
int escc = -1;
unsigned j;
char nbuf[80];
unsigned rcount;
unsigned port;

if(argc<2)
{

printf("usage:\n");
printf("%s port\n",argv[0]);
exit(1);
}

port = atoi(argv[1]);
sprintf(nbuf,"/dev/escc%u",port);

escc = open(nbuf,O_RDWR);
if(escc == -1)
	{
	printf("cannot open %s\n",nbuf);
	perror(NULL);
	exit(1);
	}
printf("opened %s\n",nbuf);

if(ioctl(escc,ESCC_FLUSH_RX,NULL)==-1) perror(NULL);

do
{
if((rcount = read(escc,buf,4096))==-1)
	{
	printf("read failed:errno = %i\n",errno);
	perror(NULL);
	}
else
{
for(j=0;j<40;j++)printf("-");
printf("\n");
printf("Read returned:%i\n",rcount);
for(j=0;j<rcount-1;j++) printf("%c",buf[j]);
printf("\n");
if((buf[rcount-1]&0x20)==0x00)printf("CRC FAILED\n");
else printf("CRC Passed\n");
}

}while(1);


close(escc);
exit(1);
return 0;
}


void decode_status(unsigned long stat)
{
if(stat==0)return;
printf("STATUS DECODE:\n");
if(stat&ST_RX_DONE) printf("Receive Done\n");
if(stat&ST_OVF) printf("RX BUFFERS overflow\n");
if(stat&ST_RFS) printf("Receive Frame Start\n");
if(stat&ST_RX_TIMEOUT) printf("RX timeout\n");
if(stat&ST_RSC) printf("Receive Status Change\n");
if(stat&ST_PERR) printf("Parity Error\n");
if(stat&ST_PCE) printf("Protocol Error\n");
if(stat&ST_FERR) printf("Framing Error\n");
if(stat&ST_SYN) printf("SYN detect\n");
if(stat&ST_DPLLA) printf("DPLL asyncronous\n");
if(stat&ST_CDSC) printf("CD changed state\n");
if(stat&ST_RFO) printf("Receive Frame Overflow\n");
if(stat&ST_EOP) printf("End of Poll\n");
if(stat&ST_BRKD) printf("Break Detected\n");
if(stat&ST_ONLP) printf("On Loop\n");
if(stat&ST_BRKT) printf("Break Terminated\n");
if(stat&ST_ALLS) printf("All Sent\n");
if(stat&ST_EXE) printf("Transmit Underrun\n");
if(stat&ST_TIN) printf("Timer Expired\n");
if(stat&ST_CTSC) printf("CTS changed state\n");
if(stat&ST_XMR) printf("Transmit Message Repeat\n");
if(stat&ST_TX_DONE) printf("Transmit Done\n");
if(stat&ST_DMA_TC) printf("DMA TC reached\n");
if(stat&ST_DSR0C) printf("DSR0 changed state\n");
if(stat&ST_DSR1C) printf("DSR1 changed state\n");
if(stat&ST_FUBAR_IRQ) printf("IRQ is FUBAR\n");
}


