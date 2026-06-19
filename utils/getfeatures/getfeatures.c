/* $Id: getfeatures.c,v 1.2 2003/11/11 18:32:27 carl Exp $ */
/*
Copyright(c) 2003, Commtech, Inc.
getfeatures.c -- user mode function to get/display the onboard feature setting for a ESCC-PCIv3 port

usage:
 getfeatures port 

 The port can be any valid escc port (0,1)



*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define ESCCP
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
	//clkset clock;
	char nbuf[80];
	int port;
	int escc; 
	unsigned long  desreg;
	unsigned long temp;


	if(argc<2) {
                printf("usage:  %s port \r\n",argv[0]);
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



//	Feature register (32 bit register at AMCCport) consists of:
//	bit 0:  receive echo cancel if '0' RTS controls RD, if '1' RD allways on
//  bit 1:  SD 485 control      if '0'/RTS controls SD, if '1' SD allways on
//  bit 2:  TT 485 control      if '0'/RTS controls TT, if '1' SD allways on
//  bit 3:  CTS disable         if '0' CTS allways active, if '1' CTS from connector
//  bit 4:  txclk <= ST         if '0' txclk connected to ST, if '1' txclk output is tri-state
//  bit 5:  txclk => TT         if '0' txclk connected to TT, if '1' TT output is '1'

desreg = 0;
if(ioctl(escc,ESCC_GET_FEATURES,&desreg)==-1) perror(NULL);
else
{
	printf("des:%8.8x\r\n",desreg);
	if((desreg&1)==1)		printf("RX allways on\r\n");
	else					printf("RX echo	cancel ENABLED\r\n");
	if((desreg&2)==2)		printf("SD is RS-422\r\n");
	else					printf("SD is RS-485\r\n");
	if((desreg&4)==4)		printf("TT is RS-422\r\n");
	else					printf("TT is RS-485\r\n");
	if((desreg&8)==8)	    printf("CTS from connector\r\n");
	else					printf("CTS allways active\r\n");
	if((desreg&0x10)==0x10)	printf("ST disconnected\r\n");
	else					printf("ST connected to txclk pin\r\n");
	if((desreg&0x20)==0x20)	printf("TT disconnected\r\n");
	else					printf("TT connected to txclk pin\r\n");
	}
close(escc);
	return 0;
}



/* $Id: getfeatures.c,v 1.2 2003/11/11 18:32:27 carl Exp $ */
