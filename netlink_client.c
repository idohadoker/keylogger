#include "netlink_client.h"

static int sendtoserver(int, char *);
static int init_client();
static int init_socket();
static int bindsockets(int);
static void init_all(int, int);
static void send_recv(int, int);
static void free_all(int);
int main()
{
    int sock_fd;
    int client_fd;
    sock_fd = init_socket();
    client_fd = init_client();
    if (bindsockets(sock_fd) != 1)
        exit(1);
    init_all(sock_fd, client_fd);
    free_all(sock_fd);

    return 0;
}

static int init_socket()
{
    int sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock_fd < 0)
    {
        exit(1);
    }
    return sock_fd;
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
    if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0)
    {
        exit(1);
    }
    return 1;
}
static int init_client()
{
    int sock;
    struct sockaddr_in addr;
    socklen_t addr_size;
    char buffer[20];
    int n;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        exit(1);
    }
    if (sock < 0)
    {
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        exit(1);
    return sock;
}
static void init_all(int sock_fd, int client_fd)
{

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

    send_recv(sock_fd, client_fd);
}
static void send_recv(int sock_fd, int client_fd)
{
    while (1)
    {
        char buffer[20];
        printf("\nPressed : %s\n", (char *)NLMSG_DATA(nlh2));

        strncpy(NLMSG_DATA(nlh), "1", 1);

        sendmsg(sock_fd, &msg, 0);
        send(client_fd, (char *)NLMSG_DATA(nlh2), 2, 0);

        recvmsg(sock_fd, &resp, 0);
        recv(client_fd, buffer, sizeof(buffer), 0);
    }
}

static void free_all(int sock_fd)
{
    free(nlh);
    free(nlh2);
    close(sock_fd);
}