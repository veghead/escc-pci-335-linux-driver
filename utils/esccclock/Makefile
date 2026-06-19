CC=gcc
LDFLAGS=-lm
NAME=esccclock

${NAME} : ${NAME}.o
	${CC} ${LDFLAGS} -o $@ $<
${NAME}.o : ${NAME}.c
	${CC} -c $<
	
all: ${NAME}

install:
	cp esccclock ../bin

clean:
	${RM} -f *.o ${NAME}
