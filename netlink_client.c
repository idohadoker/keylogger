#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <errno.h>

#define NETLINK_USER 30  // same customized protocol as in my kernel module
#define MAX_PAYLOAD 1024 // maximum payload size

struct sockaddr saddr;
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct nlmsghdr *nlh2 = NULL;
struct msghdr msg, resp; // famous struct msghdr, it includes "struct iovec *   msg_iov;"
struct iovec iov, iov2;

static int sendtoserver(int, char *);
static int init_server();
static int init_socket();
static int bindsockets(int);

int main()
{
    int sock_fd;
    int server_fd;
    sock_fd = init_socket();
    if (sock_fd < 0)
    {
        exit(1);
    }
    server_fd = init_server();
    if (server_fd < 0)
    {
        exit(1);
    }

    if (bindsockets(sock_fd) != 1)
        exit(1);

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    if (nlh == NULL)
    {
        exit(1);
    }

    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid(); // self pid
    nlh->nlmsg_flags = 0;

    // nlh2: contains received msg
    nlh2 = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    if (nlh2 == NULL)
    {
        exit(1);
    }

    memset(nlh2, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh2->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh2->nlmsg_pid = getpid(); // self pid
    nlh2->nlmsg_flags = 0;

    strcpy(NLMSG_DATA(nlh), "Hello this is a msg from userspace"); // put "Hello" msg into nlh

    iov.iov_base = (void *)nlh; // iov -> nlh
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr; // msg_name is Socket name: dest
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov; // msg -> iov
    msg.msg_iovlen = 1;

    iov2.iov_base = (void *)nlh2; // iov -> nlh2
    iov2.iov_len = nlh2->nlmsg_len;
    resp.msg_name = (void *)&dest_addr; // msg_name is Socket name: dest
    resp.msg_namelen = sizeof(dest_addr);
    resp.msg_iov = &iov2; // resp -> iov
    resp.msg_iovlen = 1;

    int ret = sendmsg(sock_fd, &msg, 0);
    if (ret < 0)
        exit(1);

    recvmsg(sock_fd, &resp, 0); // msg is also receiver for read

    while (1)
    {
        printf("\nPressed : %s\n", (char *)NLMSG_DATA(nlh2));
        if (sendtoserver(server_fd, (char *)NLMSG_DATA(nlh2)) != 1)
        {
            exit(1);
        }
        strncpy(NLMSG_DATA(nlh), "1", 1);

        ret = sendmsg(sock_fd, &msg, 0);
        if (ret < 0)
        {
            exit(1);
        }

        recvmsg(sock_fd, &resp, 0);
    }
    free(nlh);
    free(nlh2);
    close(sock_fd);

    return 0;
}

static int init_socket()
{
    return socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
}

static int bindsockets(int sock_fd)
{

    memset(&src_addr, 0, sizeof(src_addr));
    memset(&dest_addr, 0, sizeof(dest_addr));

    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;    /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

    // int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)))
    {
        exit(1);
    }
    return 1;
}
static int init_server()
{
    return socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
}
static int sendtoserver(int server_fd, char *message)
{

    return 1;
}
