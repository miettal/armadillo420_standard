

#CC=gcc

CPPFLAGS+= -D__BSD_SOURCE -D__FAVOR_BSD -DHAVE_NET_ETHERNET_H -DLIBNET_LIL_ENDIAN
#CFLAGS=-O2 -Wall -W
LDFLAGS=


OBJS=zcip.o lockfile.o




all: zcip make-arp


make-arp: make-arp.o
	$(CC) make-arp.o $(LDFLAGS) -o make-arp -lpcap -lnet -D__BSD_SOURCE \
		-D__FAVOR_BSD -DHAVE_NET_ETHERNET_H -DLIBNET_LIL_ENDIAN

zcip: $(OBJS)
	$(CC) -Wall -W $(OBJS) $(LDFLAGS) -o zcip -lpcap -lnet -D__BSD_SOURCE \
		-D__FAVOR_BSD -DHAVE_NET_ETHERNET_H -DLIBNET_LIL_ENDIAN

romfs:
	$(ROMFSINST) /bin/zcip

clean:
	rm -f $(OBJS) zcip make-arp make-arp.o *~


