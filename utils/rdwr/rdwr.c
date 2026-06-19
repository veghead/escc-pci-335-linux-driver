// rdwr.c a simple loopback example
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <termio.h>
#include <sys/ioctl.h>
#include "../esccpdrv.h"

static struct termio save_term;

void SetRaw(int fd)
{
    struct termio s;
    //Get terminal modes.
    (void)ioctl(fd, TCGETA, &s);
    //Save modes and set certain variables dependent on modes.
    save_term = s;
    //Set the modes to the way we want them.
    s.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL);
    s.c_oflag |= (OPOST|ONLCR|TAB3);
    s.c_oflag &= ~(OCRNL|ONOCR|ONLRET);
    s.c_cc[VMIN] = 0;
    s.c_cc[VTIME] = 0;
    (void)ioctl(fd, TCSETAW, &s);
}

static void TermRestore(int fd)
{
    struct termio s;
    //Restore saved modes.
    s = save_term;
    (void)ioctl(fd, TCSETAW, &s);
}

int main(int argc, char *argv[])
{
	int i,size,j,fd,n;
	char buf;
	int fd1,fd2;
	int hdlc;
	long rsz,wsz,rrsz;
	unsigned long fct;
	char RxBuf[4096];
	char TxBuf[4096];
	char fname[40];
	long ltime;
	int stime;
	unsigned long command;
	unsigned long errors=0;
	unsigned long status;

	if(argc<5)
	{
		printf("%s port1 port2 size h|a|b\n",argv[0]);
		printf("PORT1 0-3\tTransmit\nPORT2 0-3\tRecieve\n");
		exit(1);
	}
	// " main port1 port2 "
	sprintf(fname,"/dev/escc%d",atol(argv[1]));	
	fd1 = open(fname,O_RDWR);
	if(strcmp(argv[1],argv[2])!=0)
	{
		sprintf(fname,"/dev/escc%d",atol(argv[2]));	
		fd2 = open(fname,O_RDWR);
	}
	else fd2 = fd1;
	if(fd1==-1 || fd2==-1)
	{
		printf("Cannot open Devices!\n");
		exit(1);
	}
	
	fd=0;
	SetRaw(fd);
	printf("Enter q to quit\n\n");
	
	ltime = time(NULL);
	stime = (unsigned int) ltime/2;
	srand(stime);
		

	size = atol(argv[3]);
	if(size>4096) size=4096;
	if(size==0)
	{
		printf("size cannot be 0\n");
		exit(1);
	}
	if((argv[4][0]=='h')||(argv[4][0]=='H')) hdlc=1;
	else if((argv[4][0]=='b')||(argv[4][0]=='B'))  hdlc=2;
	else hdlc=0;
	if(hdlc==0) rrsz = size;
	else rrsz = 4096;
	ioctl(fd2,ESCC_FLUSH_RX,0);
	ioctl(fd1,ESCC_FLUSH_TX,0);
	fct=0;
	while(1)
	{
		n=read(fd,&buf,1);
		if(n==1)
		{
			if((buf=='q')||(buf=='Q')) 
			{
				goto fini;
			}
		}
		for(j=0;j<=size;j++)
		{
			TxBuf[j] = rand();
			if((hdlc==2)&&((TxBuf[j]&0xff)==0xff)) TxBuf[j]=0xfe;
		}
		command = HUNT;
		if(hdlc==2) ioctl(fd2,ESCC_CMDR,&command);//issue hunt
		printf("WRITE[%ld]: %d   Press Q to Quit\n",fct,wsz=write(fd1,TxBuf,size));
		printf("READ [%ld]: %d   Press Q to Quit\n",fct,rsz=read(fd2,RxBuf,rrsz));
		//process read/write data here...
		//if hdlc mode check CRC on last byte received (RSTA)
		//otherwise just check the data
		if(hdlc==1)
		{
			if(rsz!=wsz+1) 
			{
				errors++;
				printf("Error, count mismatch\n");//hdlc version
			}
			for(j=0;j<rsz-1;j++)
			{
				if(RxBuf[j]!=TxBuf[j])
				{
					errors++;
					printf("data mismatch: %x != %x @ %d\n",RxBuf[j]&0xff,TxBuf[j]&0xff,j);
				}
			}
		}
		else if(hdlc==2)
		{
			if(rsz!=wsz+1) 
			{
				errors++;
				printf("Error, count mismatch\n");//bisyncversion (assumes termination char!)
			}
			for(j=0;j<rsz-1;j++)
			{
				if(RxBuf[j]!=TxBuf[j])
				{
					errors++;
					printf("data mismatch: %x != %x @ %d\n",RxBuf[j]&0xff,TxBuf[j]&0xff,j);
				}
			}
		}
		else
		{
			if(rsz!=wsz)
			{
				errors++;
				printf("Error, count mismatch\n");//async version
			}
			for(j=0;j<rsz;j++)
			{
				if(RxBuf[j]!=TxBuf[j])
				{
					errors++;
					printf("data mismatch: %x != %x @ %d\n",RxBuf[j]&0xff,TxBuf[j]&0xff,j);
				}
			}
		}
		fct++;
/*		if(errors) 
		{
			if(ioctl(fd1,ESCC_IMMEDIATE_STATUS,&status)==-1) perror(NULL);
			else printf("%s Immediate Status: %8.8lx\n",fname,status);
			goto fini;
		}
*/	}
fini:
	TermRestore(fd);
	close(fd1);
	close(fd2);
	printf("Error count: %ld\n",errors);
	
	return 0;
}
