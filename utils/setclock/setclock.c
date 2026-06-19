/* $Id: setclock.c,v 1.4 2005/03/01 20:31:34 matt Exp $ */
/*
Copyright(c) 2003, Commtech, Inc.
setclock.c -- user mode function to set the programmable clock

usage:
 setclock port frequency

 The port can be any valid escc port (0,1) 
 The frequency can be any in the range 6000000 - 33333333

 (this function only works on V3 hardware)

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


int main(int argc, char * argv[])
{
	clkset clock;
	char nbuf[80];
	int port;
	int escc = -1;
	unsigned long desfreq;

	if(argc<3)
	{
		printf("usage:\n%s port frequency\n",argv[0]);
		exit(1);
	}
	port = atoi(argv[1]);
	desfreq = atol(argv[2]);

	sprintf(nbuf,"/dev/escc%u",port);
	escc = open(nbuf,O_RDWR);
	if(escc == -1)
	{
		printf("cannot open %s\n",nbuf);
		perror(NULL);
		exit(1);
	}
	printf("opened %s\r\n",nbuf);
	printf("setting clock to 	:%u\n",desfreq);
	if(ioctl(escc,ESCC_SET_FREQ,&desfreq)==-1) perror(NULL);
	close(escc);
	return 0;
}
