/******************************************************
 *
 * Copyright (C) 2000 Commtech, Inc. Wichita KS
 *
 * support.c -- misc internal support functions for the escc-pci module
 *
 * Tested with Linux version 2.2 12
 ******************************************************/

/* $Id: support.c,v 1.14 2005/03/31 16:32:10 carl Exp $ */

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

If you encounter problems or have suggestions and/or bug fixes please email them to:

custserv@commtech-fastcom.com

or mailed to:

Commtech, Inc.
9011 E. 37th N.
Wichita, KS 67226
ATTN: Linux BugFix Dept

*/

#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#define __NO_VERSION__
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h> /* get MOD_DEC_USE_COUNT, not the version string */
#include <linux/version.h> /* need it for conditionals in scull.h */

#include <linux/kernel.h> /* printk() */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
#include <linux/slab.h>
#else
#include <linux/malloc.h> /* kmalloc() */
#endif
#include <linux/vmalloc.h>
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/fcntl.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/div64.h>
#include <linux/delay.h>
#include "esccdrv.h"        /* local definitions */

//function to set the ICD2053B clock generator
void set_clock_generator(unsigned port, unsigned long hval,unsigned nmbits)
{
unsigned base;
unsigned curval;
unsigned long tempval;
unsigned i;

base = port;
curval = 0;
//bit 0 = data
//bit 1 = clock;
outb((char)curval,base+0x3c);


tempval = STARTWRD;
for(i=0;i<14;i++)
	{
	curval = 0;
	curval = (char)(tempval&0x1);   //set bit
	outb((char)curval,base+0x3c);
	curval = curval |0x02;          //force rising edge
	outb((char)curval,base+0x3c);              //clock in data
	curval = curval &0x01;          //force falling edge
	outb((char)curval,base+0x3c);
	tempval = tempval >> 1;         //get next bit
	}
	
tempval = hval;
for(i=0;i<nmbits;i++)
	{
	curval = 0;
	curval = (char)(tempval&0x1);   //set bit
	outb((char)curval,base+0x3c);
	curval = curval |0x02;          //force rising edge
	outb((char)curval,base+0x3c);
	curval = curval &0x01;          //force falling edge
	outb((char)curval,base+0x3c);
	tempval = tempval >> 1;         //get next bit
	}

tempval = MIDWRD;
for(i=0;i<14;i++)
	{
	curval = 0;
	curval = (char)(tempval&0x1);   //set bit
	outb((char)curval,base+0x3c);
	curval = curval |0x02;          //force rising edge
	outb((char)curval,base+0x3c);
	curval = curval &0x01;          //force falling edge
	outb((char)curval,base+0x3c);
	tempval = tempval >> 1;         //get next bit
	}
//pause for >10ms --should be replaced with a regulation pause routine
for(i=0;i<50000;i++)inb(PVR);

tempval = ENDWRD;
for(i=0;i<14;i++)
	{
	curval = 0;
	curval = (char)(tempval&0x1);   //set bit
	outb((char)curval,base+0x3c);
	curval = curval |0x02;          //force rising edge
	outb((char)curval,base+0x3c);
	curval = curval &0x01;          //force falling edge
	outb((char)curval,base+0x3c);
	tempval = tempval >> 1;         //get next bit
	}

}

//function to add a port to the driver list
//(note it had more to do in the ISA driver...)
int add_port(board_settings *board_switches, Escc_Dev *dev)
{
unsigned port;
//unsigned i,j;


//do the region check , if OK then allocate it,
//if not check to see if we previously allocated it for the other channel
//on this card if so OK, otherwise dump
#ifdef USE_2_6
#else
if((err=check_region(board_switches->base,0x40))<0)
{
//someone allready has the region requested
PDEBUG("region request failed:%i\n",err);
 return err; //busy
}
request_region(board_switches->base,0x40,"esccp");//allways succeeds (was checked above)
#endif
//note also that we should probably also request the AMCC region in I/O space...

//if we have a previously allocated irq unhook it here
if(dev->irq_hooked==1)
{
//release current irq mapping.
free_irq(dev->irq,escc_devices);
dev->irq_hooked=0;
}

//save off switch info in hardware device storage area
	dev->amcc = board_switches->amcc;
	dev->base = board_switches->base;
	dev->irq = board_switches->irq;
	dev->dmar = board_switches->dmar;
	dev->dmat = board_switches->dmat;
	dev->channel = board_switches->channel;
//set port for defined port offsets
port = dev->base;
//setup for 82532
outb(0x60,PCR);//must be , set port config register on 82532
outb(0x03,IPC);//must be , setup interrupt line config on 82532
outb(0x00,IVA);//must be , not using interrupt vector, set to 0
outb(0xff,IMR0);//mask ints
outb(0xff,IMR1);//mask ints
outb(0xff,PIM);//mask ints
outb(0xff,IMR0+0x40);//mask ints (ch2)
outb(0xff,IMR1+0x40);//mask ints (ch2)
#ifdef VER_3_0
#else
//setup for S5933
inb(dev->amcc+0x3a);//get irq status from S5933 (not really necessary)
outb(0x02,dev->amcc+0x3a);//reset interrupt on incoming mailbox MB4B3
inb(dev->amcc+0x1f);//read the mailbox MB4B3

outb(0x1F,dev->amcc+0x39);//enable interrupt on incommming mailbox MB4B3

outb(0x02,dev->amcc+0x3a);//reset interrupt on incoming mailbox MB4B3
inb(dev->amcc+0x1f);//read the mailbox MB4B3
#endif
return 0;
}


// Function name	: GetICD2053Bits
// Description	    : Taken from calculate_bits function.  Calculates the programming word
//					  and number of bits for an ICD2053 clock generator.  Input is an frequency
//					  returns a clkset structure that contains the word, numbits, and actual frequency.
// Return type		: clkset 
// Argument         : DOUBLE desired
int  GetICD2053Bits(unsigned long desired, clkset *clk)
{
	unsigned long P;
	unsigned long Pprime;
	unsigned long Q;
	unsigned long Qprime;
	unsigned long M;
	unsigned long I=0;
	unsigned long D;
	unsigned long fvco;
	unsigned long desired_ratio;
	unsigned long actual_ratio;
	unsigned long bestP;
	unsigned long bestQ=0;
	unsigned long bestM=0;
	unsigned long bestI=0;
	unsigned long best_actual_ratio;
	unsigned long Progword;
	unsigned long Stuffedword;
	unsigned long bit1;
	unsigned long bit2;
	unsigned long bit3;
	unsigned long i,j;
	unsigned long rangelo;
	unsigned long rangehi;
	unsigned long best_diff;
	unsigned long reffreq=18432000;
	long long n,n2;

	if ((desired>=390625)&&(desired<=100000000))
	{
		D = 0x00000000;//from bitcalc, but the datasheet says to make this 1...
		bestP = 0;
		bestQ = 0;
		best_actual_ratio = 1;//hopefully we can do better than this...
		best_diff = 10000;
		rangelo = (reffreq/1000000) +1;
		rangehi = (reffreq/200000);
		if(rangelo <3) rangelo = 3;
		if(rangehi>129) rangehi = 129;
		for(i=0;i<=7;i++)
		{
			M = i;
			fvco = desired * (1<<i);
			if(fvco<80000000) I = 0x00000000;
			if(fvco>=80000000) I = 0x00000008;
			if((fvco>50000000)&&(fvco<150000000))
			{
			n = (long long)fvco*10000;
			n2= do_div(n,2*reffreq);
				  desired_ratio = (unsigned long)(n);
//                                desired_ratio = (unsigned long)(((uint64_t )fvco*10000)/(2 * reffreq));
				for(P=4;P<=130;P++)for(Q=rangelo;Q<=rangehi;Q++)
				{
					actual_ratio = (P*10000)/Q;
					if(actual_ratio==desired_ratio) 
					{
						bestP = P;
						bestQ = Q;
						bestM = M;
						bestI = I;
						best_actual_ratio = actual_ratio;
						goto donecalc;
					}
					else 
					{
						if((unsigned long)abs(desired_ratio - actual_ratio)<(best_diff)) 
						{
							best_diff = abs(desired_ratio - actual_ratio);
							best_actual_ratio = actual_ratio;
							bestP = P;
							bestQ = Q;
							bestM = M;
							bestI = I;
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
			Progword = 0;
			Progword =  (Pprime<<15) | (D<<14) | (M<<11) | (Qprime<<4) | I;
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
				}
				bit1 = bit2;
				bit2 = bit3;
				bit3 = ((Progword>>i)&1);
				Stuffedword |=  (bit3<<j);
				i++;
				j++;
			}
//printf("clkbits:0x%x\r\n",Stuffedword);
//printf("numbits:%d\r\n",(USHORT)j-1);
//printf("P:%d\r\n",P);
//printf("Q:%d\r\n",Q);
//printf("M:%d\r\n",M);
//printf("actual:%ld\r\n", ((2 * (__int64)reffreq) * P)/ (Q * (1<<M) ));
			clk->clockbits = Stuffedword;
			clk->numbits = (uint16_t)j-1;
//                        clk->actualclock[0] = (unsigned long)(((2 * (__int64)reffreq) * P)/ (Q * (1<<M) ));
			return 0;
		}
		else
		{
		return -1;
		}
	}
	else 
	{
		return -2;
	}
return -3;	
}
// Function name	: GetICD2053Freq
// Description	    : Decodes ICD2053 clock bits into the actual frequency attained
// Return type		: double 
// Argument         : clkset clock

unsigned long GetICD2053Freq(clkset clock)
{
	 
	unsigned long fout=0;
	unsigned long fref=18432000;
	unsigned long p=0;
	unsigned long q=0;
//	double p=0.0;
//	double q=0.0;
	unsigned long pprime=0;	//p counter value
	unsigned long qprime=0;	//q counter value
	unsigned long d=0;	//Duty cycle adjust up
	unsigned long m=0;	//Mux
	unsigned long i=0;
	unsigned long stuffed=0;
	unsigned long unstuffed=0;
	unsigned long temp=0;
	unsigned long lcv1=0;	//loop control variable 1
	unsigned long numbits = 0;
	long long n,n2;
	if (clock.numbits==22)
	{
		i = (clock.clockbits & 0x0000000f);
		qprime = (clock.clockbits & 0x000007f0);
		qprime >>= 4;
		m = (clock.clockbits & 0x00003800);
		m >>= 11;
		d = (clock.clockbits & 0x00004000);
		d >>= 14;
		pprime = (clock.clockbits & 0x00ff8000);
		pprime >>= 15;
		q = qprime + 2;
		p = pprime + 3;

	}
	else
	{
		stuffed = clock.clockbits;
		unstuffed = 0;
		while(lcv1<clock.numbits)
		{
			if ((stuffed & 0x00000001)&&(stuffed & 0x00000002)&&(stuffed & 0x00000004))
			{
				temp <<= 3;
				temp |= 0x00000007;
				stuffed >>= 4;
				lcv1 += 4;
				numbits += 3;
			}
			else
			{
				temp <<= 1;
				if (stuffed & 0x00000001) temp |= 0x00000001;
				stuffed >>= 1;
				lcv1++;
				numbits += 1;
			}
		}
		lcv1 = 0;
		while (lcv1<22)
		{
			unstuffed <<= 1;
			if (temp & 0x00000001) unstuffed |= 0x00000001;
			temp >>= 1;
			lcv1++;
		}
		i = (unstuffed & 0x0000000f);
		qprime = (unstuffed & 0x000007f0);
		qprime >>= 4;
		q = qprime + 2;
		m = (unstuffed & 0x00003800);
		m >>= 11;
		d = (unstuffed & 0x00004000);
		d >>= 14;
		pprime = (unstuffed & 0x00ff8000);
		pprime >>= 15;
		p = pprime + 3;

	}
//	printf("\n\n\tThe unstuffed word is 0x%X\n\n",unstuffed);
//	fvco = ((2*fref*p)/q));
//	printf("\tFvco = %lf\n",fvco);
//	printf("\tP = 0x%X   P' = 0x%X \n",p,pprime);
//	printf("\tQ = 0x%X   Q' = 0x%X \n",q,qprime);
//	printf("\tD = 0x%X\n",d);
//	printf("\tM = 0x%X\n",m);
//	printf("\tI = 0x%X\n\n",i);
//	printf("\tFvco = 2*18.432*(p/q)\n");
//	printf("\tFout = Fvco/2^m\n\n");
	n = (long long)(fref)*2*(long long)p;
	n2 = do_div(n,q*(1<<m));
	fout = (unsigned long)(n);
//	fout =  (unsigned long)((uint64_t)(2*(uint64_t)fref*(uint64_t)p) / (uint64_t)((uint64_t)q*(uint64_t)(1<<m)));
//	printf("---------------------------------------------\r\n");
	return fout;
}

// Function name	: GetICS307Bits
// Description	    : Function takes desired frequency as a unsigned long and returns the best
//					  programming bit stream to achieve that frequency on the ICS307 clock generator.
// Return type		: unsigned long 
// Argument         : DOUBLE desired
int GetICS307Bits(unsigned long desired,unsigned long *bits,unsigned long *actual)
{
	unsigned long bestVDW=1;	//Best calculated VCO Divider Word
	unsigned long bestRDW=1;	//Best calculated Reference Divider Word
	unsigned long bestOD=1;		//Best calculated Output Divider
	unsigned long result=0;
	unsigned long vdw=1;		//VCO Divider Word
	unsigned long rdw=1;		//Reference Divider Word
	unsigned long od=1;			//Output Divider
	unsigned long lVDW=1;		//Lowest vdw
	unsigned long lRDW=1;		//Lowest rdw
	unsigned long lOD=1;		//Lowest OD
	unsigned long hVDW=1;		//Highest vdw
	unsigned long hRDW=1;		//Highest rdw
	unsigned long hOD=1;		//Highest OD
	
	unsigned long hi;			//initial range freq Max
	unsigned long low;			//initial freq range Min
	unsigned long check;		//Calculated clock
	unsigned long clk1;			//Actual clock 1 output
	unsigned long inFreq=18432000;	//Input clock frequency
	unsigned long range1=0;		//Desired frequency range limit per ics307 mfg spec.
	unsigned long range2=0;		//Desired frequency range limit per ics307 mfg spec.
	long long n,n2;
	int odskip=0;

	while ((desired < 6000000 )||(desired>33000000))
	{
		if (desired < 6000000 )
		{
			return -1;
		}
		else
		{
			return -2;
		}
	}
	hi=(desired+(desired/10));
	low=(desired-(desired/10));
//	printf("Hi = %lf  Low = %lf \n",hi, low);
	od = 2;
	while (od <= 10)
	{
                switch(od)
                {
                case 2:
                        if(desired>180000000)
                                odskip=1;
                        break;
                case 3:
                        if(desired>120000000)
                                odskip=1;
                        break;
                case 4: if(desired>90000000)
                                odskip=1;
                        break;
                case 5:
                        if(desired>72000000)
                                odskip=1;
                        break;
                case 6:
                        if(desired>60000000)
                                odskip=1;
                        break;
                case 7:
                        if(desired>50000000)
                                odskip=1;
                        break;
                case 8:
                        if(desired>45000000)
                                odskip=1;
                        break;
                case 9:
                        odskip=1;
					fallthrough;
                case 10:
                        if(desired>36000000)
                                odskip=1;
                        break;
                default:
                        printk("Case 1 Invalid OD = %ld.\n",od);
                        return -1;
                }
		rdw = 1;
		while ( (rdw <= 127) && (odskip==0) )
		{
			vdw = 4;
			while (vdw <= 511)
			{
			n = (long long)inFreq * 2 * ((long long)vdw+8);
			n2 = do_div(n,(((rdw+2)*od)));
				check = (unsigned long)(n);
//				check = (unsigned long)(( (((uint64_t)inFreq * 2)* ((uint64_t)vdw + 8)) / (((rdw + 2)*od)))  );	//calculate a check frequency
			n = (long long)inFreq * 2 * ((long long)vdw+8);
			n2 = do_div(n,((rdw+2)*10));
				range1 = (unsigned long)(n);
//				range1 = (unsigned long)(((uint64_t)inFreq * 2 * ((uint64_t)vdw + 8)) / ((rdw + 2)*10));
			n = (long long)inFreq;
			n2 = do_div(n,((rdw+2)*10));
				range2 = (unsigned long)(n);
//				range2 = (unsigned long)((uint64_t)inFreq / ((rdw + 2)*10));
				if ( ((range1) > 6000000) && ((range1) < 36000000) && ((range2) > 20000) )
				{
//if(check<7000000)DbgPrint("check:%d, range1:%d, range2,%d\r\n",check,range1,range2);

					if (check == low)
					{
//DbgPrint("L=rdw:%d,vdw:%d,od:%d,freq:%d\r\n",rdw,vdw,od,check);
						if (lRDW > rdw)
						{
							lVDW=vdw;
							lRDW=rdw;
							lOD=od;
							low=check;
						}
						else if ((lRDW == rdw) && (lVDW < vdw))
						{
							lVDW=vdw;
							lRDW=rdw;
							lOD=od;
							low=check;
						}
					}
					else if (check == hi)
					{
//DbgPrint("H=rdw:%d,vdw:%d,od:%d,freq:%d\r\n",rdw,vdw,od,check);
						if (hRDW > rdw)
						{
							hVDW=vdw;
							hRDW=rdw;
							hOD=od;
							hi=check;
						}
						else if ((hRDW == rdw) && (hVDW < vdw))
						{
							hVDW=vdw;
							hRDW=rdw;
							hOD=od;
							hi=check;
						}
					}
					if ((check > low) && (check < hi))		//if difference is less than previous difference
					{
//DbgPrint("<>rdw:%d,vdw:%d,od:%d,freq:%d\r\n",rdw,vdw,od,check);
						if ((check) > desired)
						{
							hi=check;
							hVDW=vdw;
							hRDW=rdw;
							hOD=od;
						}
						else 
						{
							low=check;
							lVDW = vdw;
							lRDW = rdw;
							lOD = od;
						}
				}
			}
			vdw++;
		}
		rdw++;
	}
	odskip=0;
	od++;
	if (od==9)
		od++;
	}
	if (((hi) - desired) < (desired - (low)))
	{
		bestVDW=hVDW;
		bestRDW=hRDW;
		bestOD=hOD;
		clk1=hi;
	}
	else
	{
		bestVDW=lVDW;
		bestRDW=lRDW;
		bestOD=lOD;
		clk1=low;
	}
	switch(bestOD)
	{
	case 2:
		result=0x11;
		break;
	case 3:
		result=0x16;
		break;
	case 4:
		result=0x13;
		break;
	case 5:
		result=0x14;
		break;
	case 6:
		result=0x17;
		break;
	case 7:
		result=0x15;
		break;
	case 8:
		result=0x12;
		break;
	case 10:
		result=0x10;
		break;
	default:
printk("ESCCPDRV:ics307clkset..AAAARRRG!!!\r\n");
		return -3;
		
	}
//	range1 = (ULONG)(((__int64)inFreq * 2 * ((__int64)bestVDW + 8)) / (bestRDW + 2));
//	range2 = (inFreq/(bestRDW + 2));
n = (long long)inFreq * 2 * ((long long)bestVDW+8);
n2 = do_div(n,(((bestRDW+2)*bestOD)));
	clk1 = (unsigned long)(n);
//	clk1 = (unsigned long)(( (((uint64_t)inFreq * 2)* ((uint64_t)bestVDW + 8)) / (((bestRDW + 2)*bestOD)))  );	//calculate a check frequency

	result<<=9;
	result|=bestVDW;
	result<<=7;
	result|=bestRDW;
//printk("bits:%lx\n",result);
//printk("act:%ld\n",clk1);
	bits[0] = result;
	actual[0] = clk1;
	return 0;
	
}
//setics307clock, 
//takes ESCC base address in port
//and ics307 programming word in hval
//physically sets clock generator to desired frequency
void setics307clock(unsigned port, unsigned long hval)
{
	unsigned long tempValue = 0;
	int i=0;
	unsigned char data=0;
	unsigned char savedval;
	savedval = inb(port+0x3c);
	
	outb((unsigned char)tempValue,port+0x3c);

	tempValue = (hval & 0x00ffffff);
//	printf("tempValue = 0x%X\n",tempValue);


	for(i=0;i<24;i++)
	{
		//data bit set
		if ((tempValue & 0x800000)!=0) 
		{
		data = 1;
//			printf("1");
//		data <<=8;
		}
		else 
		{
			data = 0;
//			printf("0");
		}
		outb(data,port+0x3c);
		udelay(10);

		//clock high, data still there
		data |= 0x02;
		outb(data,port+0x3c);
		udelay(10);
		//clock low, data still there
		data &= 0x01;
		outb(data,port+0x3c);
		udelay(10);

		tempValue<<=1;
	}
//	printf("\n");

	data = 0x4;		//strobe on
	outb(data,port+0x3c);
	udelay(10);
	data = 0x0000;		//all off
	outb(data,port+0x3c);
	udelay(10);

	outb((unsigned char)(savedval&0xF8),port+0x3c);	//force sdta,sclk,sstb to 0

}	//void set_clock_generator_307(unsigned long hval)


uint64_t get__tsc(void)
{
	unsigned int lo;
	unsigned int hi;

	asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)hi << 32) | lo;
}


static unsigned char xtab[256] ={0x0,
0x80,0x40,0xc0,0x20,0xa0,0x60,0xe0,0x10,0x90,0x50,
0xd0,0x30,0xb0,0x70,0xf0,0x8,0x88,0x48,0xc8,0x28,
0xa8,0x68,0xe8,0x18,0x98,0x58,0xd8,0x38,0xb8,0x78,
0xf8,0x4,0x84,0x44,0xc4,0x24,0xa4,0x64,0xe4,0x14,
0x94,0x54,0xd4,0x34,0xb4,0x74,0xf4,0xc,0x8c,0x4c,
0xcc,0x2c,0xac,0x6c,0xec,0x1c,0x9c,0x5c,0xdc,0x3c,
0xbc,0x7c,0xfc,0x2,0x82,0x42,0xc2,0x22,0xa2,0x62,
0xe2,0x12,0x92,0x52,0xd2,0x32,0xb2,0x72,0xf2,0xa,
0x8a,0x4a,0xca,0x2a,0xaa,0x6a,0xea,0x1a,0x9a,0x5a,
0xda,0x3a,0xba,0x7a,0xfa,0x6,0x86,0x46,0xc6,0x26,
0xa6,0x66,0xe6,0x16,0x96,0x56,0xd6,0x36,0xb6,0x76,
0xf6,0xe,0x8e,0x4e,0xce,0x2e,0xae,0x6e,0xee,0x1e,
0x9e,0x5e,0xde,0x3e,0xbe,0x7e,0xfe,0x1,0x81,0x41,
0xc1,0x21,0xa1,0x61,0xe1,0x11,0x91,0x51,0xd1,0x31,
0xb1,0x71,0xf1,0x9,0x89,0x49,0xc9,0x29,0xa9,0x69,
0xe9,0x19,0x99,0x59,0xd9,0x39,0xb9,0x79,0xf9,0x5,
0x85,0x45,0xc5,0x25,0xa5,0x65,0xe5,0x15,0x95,0x55,
0xd5,0x35,0xb5,0x75,0xf5,0xd,0x8d,0x4d,0xcd,0x2d,
0xad,0x6d,0xed,0x1d,0x9d,0x5d,0xdd,0x3d,0xbd,0x7d,
0xfd,0x3,0x83,0x43,0xc3,0x23,0xa3,0x63,0xe3,0x13,
0x93,0x53,0xd3,0x33,0xb3,0x73,0xf3,0xb,0x8b,0x4b,
0xcb,0x2b,0xab,0x6b,0xeb,0x1b,0x9b,0x5b,0xdb,0x3b,
0xbb,0x7b,0xfb,0x7,0x87,0x47,0xc7,0x27,0xa7,0x67,
0xe7,0x17,0x97,0x57,0xd7,0x37,0xb7,0x77,0xf7,0xf,
0x8f,0x4f,0xcf,0x2f,0xaf,0x6f,0xef,0x1f,0x9f,0x5f,
0xdf,0x3f,0xbf,0x7f,0xff};

void convertlsb2msb(unsigned char *data, unsigned size)
{
unsigned i;
for(i=0;i<size;i++) data[i] = xtab[data[i]];
}

