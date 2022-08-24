#ifndef PACKETSEND_H
#define PACKETSEND_H
#define DESTMAC0 0x20 // destination mac
#define DESTMAC1 0x4e
#define DESTMAC2 0xf6
#define DESTMAC3 0xf3
#define DESTMAC4 0x07
#define DESTMAC5 0x91
#define destination_ip 192.168.1.54 // destination ip address

int sendPacket(char *data);

#endif