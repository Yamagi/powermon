PROG=	raplctl
SRCS=	src/main.c
MAN=

WARNS?=	2

BINDIR?=	/usr/local/sbin
BINMODE=	755
BINOWN=		root

.include <bsd.prog.mk>

