/* $Id: status.c,v 1.2 2003/11/11 21:01:54 carl Exp $ */
/*
Copyright(c) 2003, Commtech, Inc.
status.c -- user mode function to get the escc status

usage:
 status port 

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

int main(int argc,char*argv[])
{
int escc;
char nbuf[128];
unsigned port;
unsigned long status;
if(argc<2)
{
printf("usage:%s port\n",argv[0]);
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
printf("opened %s\r\n",nbuf);


if(ioctl(escc,ESCC_STATUS,&status)==-1) perror(NULL);
else
{
printf("%s Status: %8.8lx\n",nbuf,status);
if((status& ST_RX_DONE)!=0)	printf("STATUS:receive complete\n");
if((status& ST_OVF)!=0)		printf("STATUS:receive buffers overflow\n");
if((status& ST_RFS)!=0)		printf("STATUS:receive frame start\n");
if((status& ST_RX_TIMEOUT)!=0)	printf("STATUS:receive timeout\n");
if((status& ST_RSC)!=0)		printf("STATUS:receive status change\n");
if((status& ST_PERR)!=0)	printf("STATUS:parity error\n");
if((status& ST_PCE)!=0)		printf("STATUS:protocol error\n");
if((status& ST_FERR)!=0)	printf("STATUS:framing error\n");
if((status& ST_SYN)!=0)		printf("STATUS:SYN detect\n");
if((status& ST_DPLLA)!=0)	printf("STATUS:DPLL async\n");
if((status& ST_CDSC)!=0)	printf("STATUS:CD state change\n");
if((status& ST_RFO)!=0)		printf("STATUS:receive frame overflow (hardware)\n");
if((status& ST_EOP)!=0)		printf("STATUS:EOP\n");
if((status& ST_BRKD)!=0)	printf("STATUS:break detect\n");
if((status& ST_ONLP)!=0)	printf("STATUS:on loop\n");
if((status& ST_BRKT)!=0)	printf("STATUS:break terminated\n");
if((status& ST_ALLS)!=0)	printf("STATUS:all sent\n");
if((status& ST_EXE)!=0)		printf("STATUS:transmit data underrun\n");
if((status& ST_TIN)!=0)		printf("STATUS:timer expired\n");
if((status& ST_CTSC)!=0)	printf("STATUS:CTS state change\n");
if((status& ST_XMR)!=0)		printf("STATUS:transmit message repeat\n");
if((status& ST_TX_DONE)!=0)	printf("STATUS:transmit complete\n");
if((status& ST_DMA_TC)!=0)	printf("STATUS:DMA TC reached\n");
if((status& ST_DSR1C)!=0)	printf("STATUS:CH1 DSR state change\n");
if((status& ST_DSR0C)!=0)	printf("STATUS:CH2 DSR state change\n");
if((status& ST_FUBAR_IRQ)!=0)	printf("STATUS:IRQ FUBAR detected\n");

}

close(escc);
return 0;
}



