INCLUDES        = -I. -I/usr/include

LIBS		= libsocklib.a  \
			-ldl -lpthread -lm

COMPILE_FLAGS   = ${INCLUDES} -c
COMPILE         = gcc ${COMPILE_FLAGS}
LINK            = gcc -o

C_SRCS		= \
		stat_cli.c\
		producers.c \
		consumers.c \
		passivesock.c \
		connectsock.c \
		prodcon_server.c

SOURCE          = ${C_SRCS}

OBJS            = ${SOURCE:.c=.o}

EXEC		= mstats mproducers mconsumers mpcserver

.SUFFIXES       :       .o .c .h

all		:	library stats producers consumers pcserver

.c.o            :	${SOURCE}
			@echo "    Compiling $< . . .  "
			@${COMPILE} $<

library		:	passivesock.o connectsock.o
			ar rv libsocklib.a passivesock.o connectsock.o

pcserver	:	prodcon_server.o
			${LINK} $@ prodcon_server.o ${LIBS}

producers	:	producers.o
			${LINK} $@ producers.o ${LIBS}

consumers	:	consumers.o
			${LINK} $@ consumers.o ${LIBS}

stats		:	stat_cli.o
			${LINK} $@ stat_cli.o ${LIBS}

clean           :
			@echo "    Cleaning ..."
			rm -f tags core *.out *.o *.lis *.a ${EXEC} libsocklib.a
