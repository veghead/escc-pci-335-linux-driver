/* $Id: esccclock.c,v 1.4 2004/04/02 21:12:58 carl Exp $ */
/*
Copyright(c) 2003, Commtech, Inc.
esccclock.c -- user mode function to program the programmable clock

usage:
 setclock port frequency

 The port can be any valid escc port (0,1) 
 frequency can be any number between 1000000 and 33333333
 (this code only works on pre V3 hardware)
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

int calculate_bits(double reffreq,double desired_freq,double *m_actual_clock,clkset *clk);

int main(int argc, char * argv[])
{
clkset clock;
char nbuf[80];
int port;
int escc = -1;
double desfreq;
double reffreq = 18432000.0;
double actualfreq;

if(argc<3)
{
printf("usage:\nesccclock port frequency\n");
exit(1);
}
port = atoi(argv[1]);
desfreq = atof(argv[2]);

sprintf(nbuf,"/dev/escc%u",port);
printf("opening %s\n",nbuf);

escc = open(nbuf,O_RDWR);
if(escc == -1)
{
printf("cannot open escc%u\n",port);
perror(NULL);
exit(1);
}
printf("opened escc%u\r\n",port);
printf("reference frequency	:%f\n",reffreq);
printf("desired frequency	:%f\n",desfreq);
if(calculate_bits(reffreq,desfreq,&actualfreq,&clock))
{
printf("setting clock to 	:%f\n",actualfreq);
printf("clkbits:%lx , numbits:%u\n",clock.clockbits,clock.numbits);
if(ioctl(escc,ESCC_SET_CLOCK,&clock)==-1) perror(NULL);
}
close(escc);
return 0;
}



//function to calcluate input clock params.
int calculate_bits(double reffreq,double desired_freq,double *m_actual_clock,clkset *clk)
{
unsigned long P;
unsigned long Pprime;
unsigned long Q;
unsigned long Qprime;
unsigned long M;
unsigned long I;
unsigned long D;
double fvco;
double desired_ratio;
double actual_ratio;
unsigned long bestP;
unsigned long bestQ;
unsigned long bestM;
unsigned long bestI;
double best_actual_ratio;
unsigned long Progword;
unsigned long Stuffedword;
unsigned long bit1;
unsigned long bit2;
unsigned long bit3;
unsigned long i,j;
//char buf[256];
unsigned long rangelo;
unsigned long rangehi;
double best_diff;

D = 0x00000000;//from bitcalc, but the datasheet says to make this 1...
bestP = 0;
bestQ = 0;
best_actual_ratio = 1000000.0;//hopefully we can do better than this...
best_diff = 1000000.0;
rangelo = (unsigned long)floor(reffreq/1000000.0) +1;
rangehi = (unsigned long)floor(reffreq/200000.0);
if(rangelo <3) rangelo = 3;
if(rangehi>129) rangehi = 129;
//sprintf(buf,"%u(%f), %u(%f)",rangelo,(reffreq/1000000.0) +1.0,rangehi,(reffreq/200000.0));
//MessageBox(buf,"rl(fl),rh(fh)",MB_OK);
for(i=0;i<=7;i++)
{
M = i;
fvco = desired_freq * (pow(2,i));
//sprintf(buf,"%f",fvco);
//MessageBox(NULL,buf,"Fvco",MB_OK);

if(fvco<80000000.0) I = 0x00000000;
if(fvco>=80000000.0) I = 0x00000008;
if((fvco>50000000.0)&&(fvco<150000000.0))
{
//sprintf(buf,"%f",fvco);
//MessageBox(NULL,buf,"Fvco",MB_OK);
desired_ratio = fvco/(2.0 * reffreq);
//sprintf(buf,"%f",desired_ratio);
//MessageBox(NULL,buf,"desired",MB_OK);

for(P=4;P<=130;P++)for(Q=rangelo;Q<=rangehi;Q++)
{

actual_ratio = (double)P/(double)Q;
if(actual_ratio==desired_ratio) 
	{
	//sprintf(buf,"%u,%u",P,Q);
	//MessageBox(NULL,buf,"Direct Hit",MB_OK);
	bestP = P;
	bestQ = Q;
	bestM = M;
	bestI = I;
	best_actual_ratio = actual_ratio;
	goto donecalc;
	}
else 
	{
	if(fabs(desired_ratio - actual_ratio)<(best_diff)) 
		{
		best_diff = fabs(desired_ratio - actual_ratio);
		best_actual_ratio = actual_ratio;
		bestP = P;
		bestQ = Q;
		bestM = M;
		bestI = I;
		//sprintf(buf,"desired:%f,actual:%f, best%f P%u,Q%u,fvco:%f,M:%u",desired_ratio,actual_ratio,best_diff,bestP,bestQ,fvco,M);
		//MessageBox(buf,"ratiocalc",MB_OK);
		}
	}	
}
}
}
donecalc:
if((bestP!=0)&&(bestQ!=0))
	{
	//here bestP BestQ are good to go.
	I = bestI;
	M = bestM;
	P = bestP;
	Q = bestQ;
	Pprime = bestP - 3;
	Qprime = bestQ - 2;
	//sprintf(buf,"P':%u, Q':%u, M:%u, I:%u",Pprime,Qprime,M,I);
	//MessageBox(buf,"P,Q,M,I",MB_OK);
	Progword = 0;
	Progword =  (Pprime<<15) | (D<<14) | (M<<11) | (Qprime<<4) | I;
//	sprintf(buf,"%lx",Progword);
//	MessageBox(buf,"Progword",MB_OK);
	bit1 = 0;
	bit2 = 0;
	bit3 = 0;
	Stuffedword = 0;
	i = 0;
	j = 0;
	bit1 = ((Progword>>i)&1);
	Stuffedword |=  (bit1<<j);
	i++;
	j++;
	bit2 = ((Progword>>i)&1);
	Stuffedword |=  (bit2<<j);
	i++;
	j++;
	bit3 = ((Progword>>i)&1);
	Stuffedword |=  (bit3<<j);
	j++;
	i++;
	while(i<=22)
		{
		if((bit1==1)&&(bit2==1)&&(bit3==1))
			{
			//force a 0 in the stuffed word;
			j++;
			bit3 = 0;
//			sprintf(buf,"i,j : %u,%u",i,j);
//			MessageBox(buf,"Stuffing",MB_OK);
			}
		bit1 = bit2;
		bit2 = bit3;
		bit3 = ((Progword>>i)&1);
		Stuffedword |=  (bit3<<j);
		i++;
		j++;
		}
//	sprintf(buf,"SW:%lx ,numbits:%u",Stuffedword,j);

clk->clockbits = Stuffedword;
clk->numbits = (unsigned)j-1;
*m_actual_clock = ((2.0 * reffreq) * ((double)P/(double)Q)) / pow(2,M);
//	MessageBox(buf,"stuffeunsigned long, numbits",MB_OK);
return 1;
	}
else
{
	printf("\r\nError in ICD calculation\r\n");
	return 0;

}
}

