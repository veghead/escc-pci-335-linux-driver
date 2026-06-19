NAME=raw	

all: ${NAME}

${NAME}:
	gcc -o disp_bin disp_bin.c
	gcc -o lsb2msb lsb2msb.c
	gcc -o read read.c
	gcc -o shifter shifter.c
	gcc -o ts_comp ts_comp.c
	gcc -o write_file write_file.c
	gcc -o write_random write_random.c
	echo "RAW" > ${NAME} 

install:

clean:
	${RM} -f *.o ${NAME}
	${RM} -f disp_bin
	${RM} -f lsb2msb
	${RM} -f read
	${RM} -f shifter
	${RM} -f ts_comp
	${RM} -f write_file
	${RM} -f write_random

