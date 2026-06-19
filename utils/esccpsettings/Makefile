CC=gcc
LDFLAGS=-lm
NAME=esccpsettings

${NAME} : ${NAME}.o
	${CC} ${LDFLAGS} -o $@ $<
${NAME}.o : ${NAME}.c
	${CC} -c $<
	
all: ${NAME}
install: 
	cp ${NAME} ../bin
	cp hdlcset ../bin
clean:
	${RM} -f *.o ${NAME}
