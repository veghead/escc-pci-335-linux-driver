/* $Id: esccpsettings.c,v 1.3 2003/11/13 21:25:42 carl Exp $ */
/*
Copyright(c) 2003, Commtech, Inc.
esccpsettings.c -- user mode function to set the 82532 registers

usage:
 esccpsettings port settingsfile

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
#include <ctype.h>
#include <string.h>
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
FILE *fin;
setup settings;
char nbuf[80];
int port;
int escc = -1;
int i;
unsigned value;
char *numptr;
char inputline[512];


memset(inputline,0,512);

if(argc<3)
{
printf("usage:\nesccsettings port settingsfile\n");
exit(1);
}
fin = fopen(argv[2],"rb");
if(fin==NULL)
{
printf("cannot open settings file :%s\n",argv[2]);
exit(1);
}

port = atoi(argv[1]);
sprintf(nbuf,"/dev/escc%u",port);
printf("opening %s\n",nbuf);
escc = open(nbuf,O_RDWR);
if(escc == -1)
{
printf("cannot open %s\n",nbuf);
error(NULL);
exit(1);
}
memset(&settings,0,sizeof(setup));

//default settings just in case they give us a bogus
//settings file, this will keep the 82532 from enabling 
//interrupts (which is what would happen if we just wrote all 0's to it)
settings.imr0 = 0xff;
settings.imr1 = 0xff;
settings.pim = 0xff;

while(fgets(inputline,256,fin)!=NULL)
{
//pull settings from file
i = 0;
while(inputline[i]!=0) 
			{
			inputline[i] = tolower(inputline[i]);
			i++;
			}
if( (numptr = strchr(inputline,'='))!=NULL)
	{
	value = strtol(numptr+1,NULL,16);
	}
if(strncmp(inputline,"mode",4)==0) settings.mode = value;
if(strncmp(inputline,"ccr0",4)==0) settings.ccr0 = value;
if(strncmp(inputline,"ccr1",4)==0) settings.ccr1 = value;
if(strncmp(inputline,"ccr2",4)==0) settings.ccr2 = value;
if(strncmp(inputline,"ccr3",4)==0) settings.ccr3 = value;
if(strncmp(inputline,"ccr4",4)==0) settings.ccr4 = value;
if(strncmp(inputline,"bgr",3)==0) settings.bgr = value;
if(strncmp(inputline,"timr",4)==0) settings.timr = value;
if(strncmp(inputline,"imr0",4)==0) settings.imr0 = value;
if(strncmp(inputline,"imr1",4)==0) settings.imr1 = value;
if(strncmp(inputline,"pim",3)==0) settings.pim = value;
if(strncmp(inputline,"ipc",3)==0) settings.ipc = value;
if(strncmp(inputline,"iva",3)==0) settings.iva = value;
if(strncmp(inputline,"pcr",3)==0) settings.pcr = value;
if(strncmp(inputline,"n_rbufs",7)==0) settings.n_rbufs = value;
if(strncmp(inputline,"n_tbufs",7)==0) settings.n_tbufs = value;
if(strncmp(inputline,"n_rfsize_max",12)==0) settings.n_rfsize_max = value;
if(strncmp(inputline,"n_tfsize_max",12)==0) settings.n_tfsize_max = value;

if(strncmp(inputline,"xbcl",4)==0) settings.xbcl = value;
if(strncmp(inputline,"xbch",4)==0) settings.xbch = value;
if(strncmp(inputline,"tsax",4)==0) settings.tsax = value;
if(strncmp(inputline,"tsar",4)==0) settings.tsar = value;
if(strncmp(inputline,"xccr",4)==0) settings.xccr = value;
if(strncmp(inputline,"rccr",4)==0) settings.rccr = value;

if(strncmp(inputline,"xad1",4)==0) settings.xad1 = value;
if(strncmp(inputline,"xad2",4)==0) settings.xad2 = value;
if(strncmp(inputline,"rah1",4)==0) settings.rah1 = value;
if(strncmp(inputline,"rah2",4)==0) settings.rah2 = value;
if(strncmp(inputline,"ral1",4)==0) settings.ral1 = value;
if(strncmp(inputline,"ral2",4)==0) settings.ral2 = value;
if(strncmp(inputline,"rlcr",4)==0) settings.rlcr = value;
if(strncmp(inputline,"aml",3)==0) settings.aml = value;
if(strncmp(inputline,"amh",3)==0) settings.amh = value;
if(strncmp(inputline,"pre",3)==0) settings.pre = value;

if(strncmp(inputline,"xon",3)==0) settings.xon = value;
if(strncmp(inputline,"xoff",4)==0) settings.xoff = value;
if(strncmp(inputline,"tcr",3)==0) settings.tcr = value;
if(strncmp(inputline,"dafo",4)==0) settings.dafo = value;
if(strncmp(inputline,"rfc",3)==0) settings.rfc = value;
if(strncmp(inputline,"mxn",3)==0) settings.mxn = value;
if(strncmp(inputline,"mxf",3)==0) settings.mxf = value;

if(strncmp(inputline,"synl",4)==0) settings.synl = value;
if(strncmp(inputline,"synh",4)==0) settings.synh = value;

}


if(ioctl(escc,ESCC_INIT,&settings)==-1)
{
 perror(NULL);
 printf("failed settings ioctl call\n");
close(escc);
exit(1);
}
else 
{
printf("%s settings set\n",nbuf);
}
fclose(fin);
close(escc);
return 0;
}





