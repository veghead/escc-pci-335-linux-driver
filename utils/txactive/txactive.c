/* $Id: txactive.c,v 1.2 2003/11/11 21:01:54 carl Exp $ */
/*
Copyright(c) 2003, Commtech, Inc.
txactive.c -- user mode function to determine if the transmitter is sending data

usage:
 txactive port 

 The port can be any valid escc port (0,1) 
 
   The txactive indication will show the transmitter as active as soon as 
   a write operation has started.  It will show the transmitter as inactive
   as soon as the last portion of a frame (or queued frames) is transfered into
   the TXFIFO of the 82532.  The INACTIVE inidcation is no an indication that
   all of the data has been shifted out of the device, simply an indication that
   the driver is finished with it.
   Use the ST_ALLS indication to determine when the physical transmitter is
   finished sending data.

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
unsigned long txactive;
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


if(ioctl(escc,ESCC_TX_ACTIVE,&txactive)==-1) perror(NULL);
else
{
	if(txactive==0)printf("%s TX ==> INACTIVE\n",nbuf);
	else printf("%s TX ==> ACTIVE\n",nbuf);
}
close(escc);
return 0;
}

