/*
shifter.c	-- a program to shift a bit stream that is received lsb first
			-- by 1 to 7 bits.

  usage:

  shifter #bits_to_shift inputfile outputfile

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
static unsigned char carrymask[9] = {	0xff,//shift 0
										0xfe,//shift 1
										0xfc,
										0xf8,
										0xf0,
										0xe0,
										0xc0, //shift 6
										0x80};//shift 7


int main(int argc, char *argv[])
{
unsigned shiftcount=0;
unsigned tempword=0;
unsigned nextcarryword=1;
unsigned carryword=1;
char databyte=0;
char outbyte=0;
FILE *fin;
FILE *fout;

if(argc<4)
{
printf("usage:\n");
printf("shifter count infile outfile\n");
exit(1);
}

shiftcount = atoi(argv[1]);


if(shiftcount==0)
{
printf("shiftcount must be (1-7)\n");
exit(1);
}
fin = fopen(argv[2],"rb");
if(fin==NULL)
{
printf("cannot open input file %s\n",argv[2]);
exit(1);
}
fout = fopen(argv[3],"wb");
if(fout==NULL)
{
printf("cannot open output file %s\n",argv[3]);
fclose(fin);
exit(1);
}

if(shiftcount!=0)
	{
	//shift all bits one position
	while(!feof(fin))
		{
		fread(&databyte,1,1,fin);
		tempword = databyte;
		tempword = tempword << shiftcount;
		nextcarryword = ((tempword&0xff00)>>8)&(~carrymask[shiftcount]);
		outbyte = (char)((tempword & carrymask[shiftcount])|carryword);
		carryword = nextcarryword;
		fwrite(&outbyte,1,1,fout);
		}
	}
fclose(fin);
fclose(fout);
return 0;
}
