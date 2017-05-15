PROG=	raplctl
SRCS=	src/cpuid.c src/main.c src/monitor.c src/msr.c
MAN=

WARNS?=	2

BINDIR?=	/usr/local/sbin
BINMODE=	755
BINOWN=		root

.include <bsd.prog.mk>

