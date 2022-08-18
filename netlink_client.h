#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>

#define NETLINK_USER 30  // same customized protocol as in my kernel module
#define MAX_PAYLOAD 1024 // maximum payload size
#define ip "127.1.1.1"
#define port 553

struct sockaddr_in srv;
struct sockaddr saddr;
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct nlmsghdr *nlh2 = NULL;
struct msghdr msg, resp; // famous struct msghdr, it includes "struct iovec *   msg_iov;"
struct iovec iov, iov2;
