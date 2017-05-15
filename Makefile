PROG=	raplctl
SRCS=	src/cpuid.c src/main.c src/monitor.c src/msr.c
MAN=

WARNS?=	2
LDFLAGS+= -lcurses -lm

BINDIR?=	/usr/local/sbin
BINMODE=	755
BINOWN=		root

.include <bsd.prog.mk>

