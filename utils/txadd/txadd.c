/* $Id: txadd.c,v 1.2 2003/11/11 21:01:54 carl Exp $ */
/*
Copyright(c) 2003, Commtech, Inc.
txadd.c -- user mode function to set the tx address bytes

usage:
 txadd port address

 The port can be any valid escc port (0,1) 
 address is the bytes to program into XAD1/XAD2 as XAD2=address&ff,XAD1=(address>>8)&0xfc

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
unsigned long address;
if(argc<3)
{
printf("usage:%s port address\n",argv[0]);
exit(1);
}
port = atoi(argv[1]);
sscanf(argv[2],"%x",&address);
sprintf(nbuf,"/dev/escc%u",port);
escc = open(nbuf,O_RDWR);
if(escc == -1)
{
printf("cannot open %s\n",nbuf);
perror(NULL);
exit(1);
}
printf("opened %s\r\n",nbuf);


if(ioctl(escc,ESCC_SET_TX_ADDRESS,&address)==-1) perror(NULL);
else printf("%s TX address ==> %x\n",nbuf,address);
close(escc);
return 0;
}

