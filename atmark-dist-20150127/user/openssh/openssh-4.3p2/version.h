/* $OpenBSD: version.h,v 1.46 2006/02/01 11:27:22 markus Exp $ */

#define SSH_VERSION	"OpenSSH_4.3"

#define SSH_PORTABLE	"p2"
#ifndef SSH_EXTRAVERSION
#define SSH_EXTRAVERSION
#endif
#define SSH_RELEASE	SSH_VERSION SSH_PORTABLE SSH_EXTRAVERSION