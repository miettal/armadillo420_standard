# New ports collection makefile for:    ser
# Date created:        6 November 2002
# Whom:                andrei
#
# $NetBSD$
#
# $Id: Makefile,v 1.3.2.1 2003/11/14 15:55:58 andrei Exp $
#

COMMENT= "SIP Express Router"

PORTNAME= ser
PORTVERSION= 0.8.12
CATEGORIES= net
MASTER_SITES= ftp://ftp.berlios.de/pub/ser/0.8.12/src/

MAINTAINER= pelinescu-onciul@fokus.fraunhofer.de

MAN8=ser.8
MAN5=ser.cfg.5
# MANCOMPRESSED must not be specified (if defined is assumed to be yes)


USE_GMAKE= yes
# we override the DISTFILE NAME
DISTNAME= ${PORTNAME}-${PORTVERSION}
DISTFILES= ${DISTNAME}_src.tar.gz
WRKSRC= ${WRKDIR}/${PORTNAME}-${PORTVERSION}

# build by default mysql and jabber
MAKE_ENV= include_modules="mysql jabber"

# depends on expat and mysqlclient because of the above modules
DEPENDS= expat-[0-9]*:../../textproc/expat \
         mysql-client>=3.23.35:../../databases/mysql-client

#.include "/usr/pkgsrc/mk/bsd.prefs.mk"

pre-fetch:

.include "../../mk/bsd.pkg.mk"


