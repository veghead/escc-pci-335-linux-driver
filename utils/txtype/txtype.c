/* $Id: txtype.c,v 1.2 2003/11/11 21:01:54 carl Exp $ */
/*
Copyright(c) 2003, Commtech, Inc.
txtype.c -- user mode function to set the transmit type as XIF or XTF

usage:
 txtype port xif|xtf

 The port can be any valid escc port (0,1) 
 xif uses the XIF command to send frames (automode)
 xtf uses the XTF command to send frames (transparent,etc.)
 
   !under allmost all circumstances xtf should be specified.!
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
unsigned long txtype;
if(argc<3)
{
printf("usage:%s port xtf|xif\n",argv[0]);
exit(1);
}
port = atoi(argv[1]);
txtype = XTF;
if(strncmp(argv[2],"xtf",3)==0) txtype=XTF;
if(strncmp(argv[2],"xif",3)==0) txtype=XIF;

sprintf(nbuf,"/dev/escc%u",port);
escc = open(nbuf,O_RDWR);
if(escc == -1)
{
printf("cannot open %s\n",nbuf);
perror(NULL);
exit(1);
}
printf("opened %s\r\n",nbuf);


if(ioctl(escc,ESCC_SET_TX_TYPE,&txtype)==-1) perror(NULL);
else
	{
	if(txtype==XTF)printf("%s txtype ==> XTF\n",nbuf);
	else printf("%s txtype ==> XIF\n",nbuf);
	}
close(escc);
return 0;
}

