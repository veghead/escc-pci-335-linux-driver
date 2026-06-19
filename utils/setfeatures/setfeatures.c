/* $Id: setfeatures.c,v 1.2 2003/11/11 18:32:27 carl Exp $ */
/*
Copyright(c) 2003, Commtech, Inc.
setfeatures.c -- user mode function to set the onboard feature setting for a ESCC-PCIv3 port


usage:
 setfeatures port [0|1] [0|1] [0|1] [0|1] [0|1] [0|1]

 The port can be any valid escc port (0,1)

parameter1 rx echo cancel control, 1==echo cancel ON, 0== OFF
parameter2 SD 485 control, 1== SD is RS-485, 0== SD is RS-422
parameter3 TT 485 control, 1== TT is RS-485, 0== TT is RS-422
parameter4 CTS disable control, 1== CTS is disabled, 0== CTS used from connector
parameter5 1== ST connected to txclk pin, 0== ST disconnected.
parameter6 1== TT connected to txclk pin, 0== TT disconnected.



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
	unsigned long rxecho;
	unsigned long sd485;
	unsigned long tt485;
	unsigned long ctsdisable;
	unsigned long sttxclk;
	unsigned long tttxclk;


	if(argc<8) 
		{
		printf("usage:\r\n");
		printf("%s port [0|1] [0|1] [0|1] [0|1] [0|1] [0|1]\r\n",argv[0]);
		printf("\r\n");
		printf("The port can be any valid escc port (0,1)\r\n");
		printf("\r\n");
		printf("parameter1 rx echo cancel control, 1==echo cancel ON, 0== OFF\r\n");
		printf("parameter2 SD 485 control, 1== SD is RS-485, 0== SD is RS-422\r\n");
		printf("parameter3 TT 485 control, 1== TT is RS-485, 0== TT is RS-422\r\n");
		printf("parameter4 CTS disable control, 1== CTS is disabled, 0== CTS from connector\r\n");
		printf("parameter5 1== ST connected to txclk pin, 0== ST disconnected.\r\n");
		printf("parameter6 1== TT connected to txclk pin, 0== TT disconnected.\r\n");
		printf("\r\n");
		exit(1);
		}

	port = atoi(argv[1]);
	rxecho = atol(argv[2]);
	sd485 = atol(argv[3]);
	tt485 = atol(argv[4]);
	ctsdisable = atol(argv[5]);
	sttxclk = atol(argv[6]);
	tttxclk = atol(argv[7]);

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
if(rxecho     == 0)		desreg|=0x01;
if(sd485      == 0)		desreg|=0x02;
if(tt485      == 0)		desreg|=0x04;
if(ctsdisable == 0)	    desreg|=0x08;
if(sttxclk    == 0)		desreg|=0x10;
if(tttxclk    == 0)		desreg|=0x20;
printf("des:%8.8x\r\n",desreg);
if(ioctl(escc,ESCC_SET_FEATURES,&desreg)==-1) perror(NULL);
else
{
	if(rxecho     == 0)		printf("RX allways on\r\n");
	else					printf("RX echo	cancel ENABLED\r\n");
	if(sd485      == 0)		printf("SD is RS-422\r\n");
	else					printf("SD is RS-485\r\n");
	if(tt485      == 0)		printf("TT is RS-422\r\n");
	else					printf("TT is RS-485\r\n");
	if(ctsdisable == 0)	    printf("CTS from connector\r\n");
	else					printf("CTS allways active\r\n");
	if(sttxclk    == 0)		printf("ST disconnected\r\n");
	else					printf("ST connected to txclk pin\r\n");
	if(tttxclk    == 0)		printf("TT disconnected\r\n");
	else					printf("TT connected to txclk pin\r\n");
	}

close(escc);

	return 0;
}



/* $Id: setfeatures.c,v 1.2 2003/11/11 18:32:27 carl Exp $ */
