(this set of instructions has been modified from the SuperFastcom
instructions.  As with most things SuperFastcom related, this will be similar
but different).

Also included are a handful of utilities that I used to test this mode, and
simulate your application.

I had port 1 connected to port 2 via a crossover cable (SD->RD, TT->RT).

open a console window and execute:


setclock 0 6000000                
esccpsettings 1 extset
esccpsettings 0 extset_r

At this point it is good to go.
I opened two console windows and executed:

write_random 1 4096 10000000 wout

in one window and:

read 0 4096 out 5000000

in the other.
let it run until the receive program exits


To verify the files a couple of utilities were generated (ts_comp,shifter,disp_bin)
The out file may or may not be the same as wout, since more than likely the
two programs aren't started at the same time, the beginning points in the file
will be different, but more importantly the receiver will not do any kind of byte
alignment in the mode we are using, so the entire stream could very well be shifted by 0-7 bit positions
from the output stream.  (if you need a more detailed description of this issue
let me know and I will expound on it).
If you run the shifter program on the read output file as:

shifter 1 out out1
shifter 2 out out2
shifter 3 out out3
shifter 4 out out4
shifter 5 out out5
shifter 6 out out6
shifter 7 out out7

Then one of the files out,out1,out2,out3,out4,out5,out6,out7 will compare with
the wout file successfully.
The ts_comp program can be used to verify the data, it scans the input file for
the framecounter, and then scans the wout file for the same counter value, then
compares the rest of the bytes in the file.
You should expect that the last couple of bytes will fail, as they are artifacts
of the shifter program.
one of:

ts_comp out wout
ts_comp out1 wout
ts_comp out2 wout
ts_comp out3 wout
ts_comp out4 wout
ts_comp out5 wout
ts_comp out6 wout
ts_comp out7 wout

should successfully compare the bulk of the saved files, and indicate only
the handfull of errors at the end of the file.
(the script 'doit' was generated to execute all of these operations)

The read program will dump data read from a port using the given blocksize
for the reads, and save the data to the specified file.
For the ESCC series, keep in mind that you need to specify the block_size to be the same value as given as n_rfsize_max in the extset_r file. (in this case 4096).


The write_random program will generate a random bitstream with a 12 byte
framecounter placed every (blocksize) bytes in the file. (the 12 byte sequence
is 0xff,0x00,8 digit ascii counter,0x00,0xff).  It outputs this stream to the
given escc port, and optionally also to a specified file.
(if you make the blocksize small and the bitrate fast, and the system is
loaded too much you will not end up with a continuous output, which you
will see as long strings of 0xff's in the received file) increasing the blocksize
should take care of this. (if the blocksize is at 4096, then the only
alternative you have with the ESCC is to reduce the bitrate, or get a faster
CPU...)

The disp_bin program dumps a binary file to the screen as hex digits,
25 bytes to a row.

The lsb2msb program can be used to change the bit order of a file from lsb to
msb first.  This code might be necessary if you expect your data to be msb first
 on the line.
(as the 82532/20534 receives and transmits data lsb first)

The shifter implements an algorithm to shift a lsb received data stream by the
given number of bits.

The ts_comp program searches the input file for the 12 byte framecounter sequence
(as per write_random) and when it finds a likely suspect, it then looks for that
framecount in the compare file, and then compares the rest of the input file
to the data in the compare file.

The extset file is the settings file to setup transmitting in extended transparent mode,
it sets up clock mode 0b which is the desired mode to work with (external receive
clock, generated transmit clock).  You may need to modify this file to setup
for clock mode 0a, if you end up using an external transmit clock.  You will
also have to use the setfeatures program to enable
the st->txclk connection for an external transmit clock).
The write_file program can be used to dump any file out a port (for instance a
captured file generated via read...)

The read_hdlc program reads hdlc frames and dumps the data to a file, assuming
that the last byte of each read will be the RSTA byte(ie that the port is
setup in HDLC transparent mode)

If you have any questions/problems just let me know.

cg
orig 2/28/2002
cur  7/7/2004




