#include "packetSend.h"       //declartions
#include <stdio.h>            //for output
#include <string.h>           //strings functions
#include <malloc.h>           //allocations
#include <errno.h>            //displaying errors
#include <sys/socket.h>       //sockadd structure
#include <sys/ioctl.h>        //system call to get mac,ip,network index
#include <net/if.h>           //ireq struct
#include <netinet/ip.h>       //ip header
#include <netinet/if_ether.h> //ethernet header
#include <netinet/udp.h>      //udp header
#include <linux/if_packet.h>  //sockaddr
#include <arpa/inet.h>        //print ip address
// gets the socket and prints it
static void printpacket(char *);
// gets the socket and saves the network index
static void get_eth_index(int);
// build the ethernet header
// returns the ethernet header
static char *get_mac(int, char *);
// build ip header
// returns the ip header
static char *get_ip(int, char *, char *);
// builds udp header
// returns the udp header
static char *get_udp(char *, char *);
// copies the key pressed to the packet
// returns buffer with data copied
static char *get_data(char *, char *);
// checks if the packet is built in the correct way
// returns checksum
static unsigned short checksum(unsigned short *buff, int _16bitword);
// initialize the socket
// return socket if successful
static int init_rawsocket();
// builds the packet using all the functions
static void build_packet(int, char *, char *);

struct ifreq ifreq_c, ifreq_i, ifreq_ip; /// for each ioctl
struct sockaddr_ll sadr_ll;
int total_len = 0, send_len;
void printpacket(char *sendbuff) // print packet
{
    struct ethhdr *eth = (struct ethhdr *)(sendbuff);
    printf("\nEthernet Header\n");                                                                                                                                             // print ethernet header
    printf("\t|-Source Address :%.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5]); // print mac address in host form,printing mac address in hex base with fields of 2 width
    printf("\t|-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);       // print mac address in host form,printing mac address in hex base with fields of 2 width
    printf("\t|-Packet Type : %x\n", ntohs(eth->h_proto));                                                                                                                     // print packet type in host form
    struct sockaddr_in source;                                                                                                                                                 // make those struct to print source and destination ip adresses
    struct sockaddr_in dest;
    struct iphdr *iph = (struct iphdr *)(sendbuff + sizeof(struct ethhdr));
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;
    printf("\nIP Header\n"); // print ip header
    printf("\t|-Version %d\n", iph->version);
    printf("\t|-Size of header in dwords %d and bytes %d\n", iph->ihl, iph->ihl * 4); // ihl in dwords thus ihl*4 to display in bytes
    printf("\t|-Type Of Service : %d\n", iph->tos);
    printf("\t|-Total Length : %d Bytes\n", ntohs(iph->tot_len));
    printf("\t|-Identification : %d\n", ntohs(iph->id));
    printf("\t|-Time To Live : %d\n", iph->ttl);
    printf("\t|-Protocol : %d\n", iph->protocol);
    printf("\t|-Header Checksum : %d\n", ntohs(iph->check));
    printf("\t|-Source IP : %s\n", inet_ntoa(source.sin_addr));
    printf("\t|-Destination IP : %s\n", inet_ntoa(dest.sin_addr));
    struct udphdr *uh = (struct udphdr *)(sendbuff + sizeof(struct iphdr) + sizeof(struct ethhdr)); // get pointer to udp header
    printf("\nUDP Header\n");                                                                       // print udp header
    printf("\t|-Source Port : %d\n", ntohs(uh->uh_sport));                                          // print soucre port in host form
    printf("\t|-Destination Port : %d\n", ntohs(uh->uh_dport));                                     // print destination port in host form
    printf("\t|-UDP Length : %d\n", ntohs(uh->len));                                                // print length of packet without ip+eth size in host form
    printf("\t|-UDP Checksum : %d\n", ntohs(uh->check));
    printf("\nDATA:\n");                                                                             // print data header
    unsigned char *data = (sendbuff + iph->ihl * 4 + sizeof(struct ethhdr) + sizeof(struct udphdr)); // get data part by adding eth,ip,icmp headers size
    int remaining_data = 64 - (iph->ihl * 4 + sizeof(struct ethhdr) + sizeof(struct udphdr));
    int i;
    for (i = 0; i < remaining_data; i++)
    {
        if (i != 0 && i % 16 == 0)
            printf("\n");
        printf("%c", data[i]);
    }
    printf("\n----------------END OF PACKET----------------\n");
}
int init_rawsocket() // init raw socket
{
    int sock_raw = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW); // send ipv4 packets
    if (sock_raw == -1)
    {
        printf("error in socket");
        return -1;
    }
    return sock_raw;
}
void get_eth_index(int sock_raw) // get network interface index
{
    memset(&ifreq_i, 0, sizeof(ifreq_i));
    strncpy(ifreq_i.ifr_name, "wlo1", IFNAMSIZ - 1);   // copy network interface name in order to make system call
    if ((ioctl(sock_raw, SIOCGIFINDEX, &ifreq_i)) < 0) // system call to get network interface index and put it in ifreq struct
        printf("error in index ioctl reading");
}
char *get_mac(int sock_raw, char *sendbuff) // make data link header and then returns updated packet
{
    memset(&ifreq_c, 0, sizeof(ifreq_c));
    strncpy(ifreq_c.ifr_name, "wlo1", IFNAMSIZ - 1);    // copy network interface name in order to make system call
    if ((ioctl(sock_raw, SIOCGIFHWADDR, &ifreq_c)) < 0) // system call to get mac address and put it in ifreq struct
        printf("error in SIOCGIFHWADDR ioctl reading");
    printf("Mac= %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]), (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]), (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]), (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]), (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]), (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]));
    struct ethhdr *eth = (struct ethhdr *)(sendbuff);
    eth->h_source[0] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]); // put source mac address
    eth->h_source[1] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]);
    eth->h_source[2] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]);
    eth->h_source[3] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]);
    eth->h_source[4] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]);
    eth->h_source[5] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]);
    eth->h_dest[0] = DESTMAC0; // put destination mac address
    eth->h_dest[1] = DESTMAC1;
    eth->h_dest[2] = DESTMAC2;
    eth->h_dest[3] = DESTMAC3;
    eth->h_dest[4] = DESTMAC4;
    eth->h_dest[5] = DESTMAC5;
    eth->h_proto = htons(ETH_P_IP); // 0x800
    total_len += sizeof(struct ethhdr);
    return sendbuff;
}
char *get_ip(int sock_raw, char *dataToSend, char *sendbuff) // make ip header and returns updated packet
{
    memset(&ifreq_ip, 0, sizeof(ifreq_ip));
    strncpy(ifreq_ip.ifr_name, "wlo1", IFNAMSIZ - 1); // copy network interface name in order to make system call
    if (ioctl(sock_raw, SIOCGIFADDR, &ifreq_ip) < 0)  // system call to get ip address and put it in ifreq struct
    {
        printf("error in SIOCGIFADDR \n");
    }
    printf("%s\n", inet_ntoa((((struct sockaddr_in *)&(ifreq_ip.ifr_addr))->sin_addr)));
    struct iphdr *iph = (struct iphdr *)(sendbuff + sizeof(struct ethhdr));
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 16;
    iph->id = htons(10201);
    iph->ttl = 64;
    iph->protocol = 17;
    iph->saddr = inet_addr(inet_ntoa((((struct sockaddr_in *)&(ifreq_ip.ifr_addr))->sin_addr)));
    iph->daddr = inet_addr("destination_ip"); // put destination IP address
    total_len += sizeof(struct iphdr);
    sendbuff = get_udp(dataToSend, sendbuff);
    iph->tot_len = htons(total_len - sizeof(struct ethhdr));
    iph->check = htons(checksum((unsigned short *)(sendbuff + sizeof(struct ethhdr)), (sizeof(struct iphdr) / 2)));
    return sendbuff;
}
char *get_udp(char *dataToSend, char *sendbuff) // make udp header and returns updated packet
{
    struct udphdr *uh = (struct udphdr *)(sendbuff + sizeof(struct iphdr) + sizeof(struct ethhdr)); // get pointer to udp header
    uh->source = htons(19241);
    uh->dest = htons(21242);
    uh->check = 0;
    total_len += sizeof(struct udphdr);
    sendbuff = get_data(dataToSend, sendbuff);                                   // copying buffer to data header
    uh->len = htons((total_len - sizeof(struct iphdr) - sizeof(struct ethhdr))); // get len in host form
    return sendbuff;
}
char *get_data(char *dataToSend, char *sendbuff) // copy buffer to data and returns updated packet
{
    for (int i = 0; i < strlen(dataToSend); i++) // copying buffer to data header
    {
        sendbuff[total_len++] = dataToSend[i];
        printf("%c", sendbuff[total_len - 1]);
    }
    return sendbuff;
}

unsigned short checksum(unsigned short *buff, int _16bitword) // checksum function to check data
{
    unsigned long sum;
    for (sum = 0; _16bitword > 0; _16bitword--)
        sum += htons(*(buff)++);
    do
    {
        sum = ((sum >> 16) + (sum & 0xFFFF));
    } while (sum & 0xFFFF0000);
    return (~sum); // return the opposite value -1
}
void build_packet(int sock_raw, char *dataToSend, char *sendbuff) // build packet headers
{
    get_eth_index(sock_raw);                           // get network interface number
    sendbuff = get_mac(sock_raw, sendbuff);            // make data link header
    sendbuff = get_ip(sock_raw, dataToSend, sendbuff); // make ip header
    printpacket(sendbuff);                             //    printpacket(sendbuff);
    struct sockaddr_ll sadr_ll;                        // make struct with info to put in send_to function
    sadr_ll.sll_family = AF_PACKET;
    sadr_ll.sll_ifindex = ifreq_i.ifr_ifindex;
    sadr_ll.sll_halen = ETH_ALEN;
    sadr_ll.sll_addr[0] = DESTMAC0;
    sadr_ll.sll_addr[1] = DESTMAC1;
    sadr_ll.sll_addr[2] = DESTMAC2;
    sadr_ll.sll_addr[3] = DESTMAC3;
    sadr_ll.sll_addr[4] = DESTMAC4;
    sadr_ll.sll_addr[5] = DESTMAC5;
    send_len = sendto(sock_raw, sendbuff, 64, 0, (const struct sockaddr *)&sadr_ll, sizeof(struct sockaddr_ll)); // send sendbuff(packet) to destination address
    memset(sendbuff, 0, 64);
    total_len = 0; // for new packet
    free(sendbuff);
    if (send_len < 0)
    {
        perror("error in sending packet");
    }
}
int sendPacket(char *data)
{
    int send_len;
    char *sendbuff;                // packet
    sendbuff = (char *)malloc(64); // allocate packet
    memset(sendbuff, 0, 64);
    if (sendbuff == NULL)
        printf("allocation error");
    int sock_raw;                // sock id
    sock_raw = init_rawsocket(); // init raw socket to send packet

    build_packet(sock_raw, data, sendbuff); // build packet and then send it
}