/* $Id: pty.c,v 1.2 2005/05/19 19:52:41 carl Exp $ */
/*
pty.c -- a utility to connect the SuperFastcom character driver to a pseudoterminal (for use with PPP).

Code originally from Michael Eriksson <Michael.Eriksson@era-t.ericsson.se>
taken from the netbsd-help list archive
modified to do what I was after, ie connecting a SFC/ESCC port to a pty.

must be compiled with -lutil as the openpty() function is located there
ie

cc -o pty pty.c -lutil

*/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <pty.h>

int uselog=0;

void reaper()
{
  int pid;
  union wait status;

  if ((pid = wait3((int *) &status, WNOHANG, 0)) > 0)
    exit(0);
}

/*
This function takes data from the pty and delivers it to the SuperFastcom/ESCC port, as is
*/
void shuffle2sfc(int from, int to, char *name)
{
  int n, n2;
  char buf[4096];//note changed from BUFSIZ to 4096 as the limit of the escc is 4k

  FILE *log;
if(uselog==1)
  {
  log = fopen(name,"wb");
  }
//get data from the pty
  while ((n = read(from, buf, (sizeof buf)-2)) > 0) {
if(uselog==1)
{ 
   fprintf(log,"[%s: ", name);
   fprintf(log,"%d]\n",n);
   fflush(log);
}
//if not an error then write it to the SuperFastcom/ESCC
    if ((n2 = write(to, buf, n)) != n)
      if (n2 < 0)
	if(uselog==1) fprintf(log,"%s writeerror:%d\n",name,n2);//err(1, "%s write", name);
      else
	if(uselog==1) fprintf(log,"%s shortwrite:(%d<%d)\n",name,n2,n);//	err(1, "%s short write (%d < %d)", name, n2, n);
  }
  if (n < 0)
	if(uselog==1) fprintf(log,"%s readerror:%d\n",name,n);//    err(1, "%s read", name);
if(uselog==1) fclose(log);
}

/*
This function takes data from the SuperFastcom, strips the last byte (RSTA byte)
and delivers the rest to the pty as is.  Note that this assumes that the
SuperFastcom port is configured in HDLC mode (HDLC transparent mode 0 to be exact).
Other modes are possible with suitable modificaiton to this routine
*/
void shuffle2pty(int from, int to, char *name)
{
  int n, n2;
  char buf[4096];//again chaned from BUFSIZ to 4k, as that is the limit of the escc
  FILE *log;
if(uselog==1)  log = fopen(name,"wb");

//read data from the SuperFastcom/ESCC
  while ((n = read(from, buf, sizeof buf)) > 0) {
 
if(uselog==1)
{
    fprintf(log,"[%s: ", name);
    fprintf(log,"%d]\n",n);
    fflush(log);
}
//while not an error write it to the pty (don't include the RSTA byte)
    if ((n2 = write(to, buf, n-1)) != n-1)
      if (n2 < 0)
	if(uselog==1) fprintf(log,"%s writeerror:%d\n",name,n2);//err(1, "%s write", name);
      else
	if(uselog==1) fprintf(log,"%s shortwrite:(%d<%d)\n",name,n2,n-1);//	err(1, "%s short write (%d < %d)", name, n2, n-1);
  }
  if (n < 0)
	if(uselog==1) fprintf(log,"%s readerror:%d\n",name,n);//    err(1, "%s read", name);
if(uselog==1) fclose(log);
}

int main(int argc, char **argv)
{
  int master1, slave1, master2, slave2;
  struct termios tt;
  struct winsize ws;
  char ptyname[1024];		/* XXX ? */

  if (argc < 2) {
    fprintf(stderr, "Usage: pty /dev/esccX\n");
    exit(1);
  }
if(argc >2) uselog=1;
else uselog=0;

master2 = open(argv[1],O_RDWR);//open the SuperFastcom/ESCC port
  if(master2<0) err(1,"openescc");
  /* open and set up ptys */
  if (tcgetattr(0, &tt) < 0)
    err(1, "tcgetattr");
  if (ioctl(0, TIOCGWINSZ, &ws) < 0)
    err(1, "ioctl(TIOCGWINSZ)");
  if ( openpty(&master1, &slave1, ptyname, &tt, &ws) < 0 )
    err(1, "openpty");
printf("using:%s\n",ptyname);
  cfmakeraw(&tt);
  tt.c_lflag &= ~ECHO;
  if (tcsetattr(slave1, TCSAFLUSH, &tt) < 0)
    err(1, "tcsetattr");

  /* fork off... */
  //create two processes, one that runs shuffle2pty
  //and one that runs shuffle2sfc

  signal(SIGCHLD, reaper);
  switch (fork()) {
  case -1:
    err(1, "fork");
  case 0:
    daemon(0, 0);
    switch (fork()) {
    case -1:
      err(1, "fork");
    case 0:
      /* shuffle data program -> pty */
      close(slave1);
      close(slave2);
      shuffle2pty(master2, master1, "copyin");
      exit(0);
    }
    /* shuffle data pty -> program */
    close(slave2);
    shuffle2sfc(master1, master2, "copyout");
    exit(0);
  }
//display our allocated pty 
//  puts(ptyname);
  return 0;
}
/* $Id: pty.c,v 1.2 2005/05/19 19:52:41 carl Exp $ */
