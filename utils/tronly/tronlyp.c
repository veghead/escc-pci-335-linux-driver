/* $Id: tronlyp.c,v 1.3 2003/11/13 21:25:42 carl Exp $ */
/*
Copyright(c) 2003, Commtech, Inc.
tronlyp.c -- user mode function to write data out a port

usage:
 tronlyp port file [delay] [loop]

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
unsigned long passval;
unsigned long status;
char nbuf[80];
int port;
FILE *fin;
int escc = -1;
char buf[4096];
unsigned rcount;
unsigned wcount;
unsigned long wtotal = 0;
unsigned loop = 0;
unsigned delay = 0;


if(argc<3)
{
printf("usage:\ntronly port file [delay] [loop]\n");
exit(1);
}
fin = fopen(argv[2],"rb");
if(fin==NULL)
{
printf("cannot open input file :%s\n",argv[2]);
exit(1);
}
if(argc>3) delay = atoi(argv[3]);
if(delay<0) delay = 0;

if(argc>4) loop = atoi(argv[4]);
if(loop<0) loop = 0;


port = atoi(argv[1]);
sprintf(nbuf,"/dev/escc%u",port);

escc = open(nbuf,O_RDWR);
if(escc == -1)
{
printf("cannot open %s\n",nbuf);
perror(NULL);
exit(1);
}
printf("opened %s\r\n",nbuf);
//lets start clean

if(ioctl(escc,ESCC_FLUSH_TX,NULL)==-1) perror(NULL);
else printf("tx flushed\n");

passval = XTF;
if(ioctl(escc,ESCC_SET_TX_TYPE,&passval)==-1) perror(NULL);

do
{
while((rcount=fread(buf,1,1024,fin))!=0)
	{
		if((wcount = write(escc,buf,rcount))==-1)
		{
		printf("write failed:errno = %i\n",errno);
		perror(NULL);
		}
		else
		{
		wtotal += wcount;
		printf("wtotal:%lu\n",wtotal);
		}
	}
loop--;
rewind(fin);
sleep(delay);
}while(loop > 0);

printf("\n\n");
printf("paused\n");
getchar();

fclose(fin);
close(escc);
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

