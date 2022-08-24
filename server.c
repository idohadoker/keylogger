#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "packetSend.h"
#define ip "127.1.1.1"
#define port 553

// variables the socket he is sending and receiving
//  sending and receiving the key we pressed
static void send_recv(int);
// returns the socket we are sending the key to
// initializes the socket
static int init_socket();
int main()
{
    int client_sock;

    client_sock = init_socket();
    send_recv(client_sock);
    close(client_sock);

    return 0;
}
static void send_recv(int client_sock)
{
    char buffer[20];

    while (1)
    {

        recv(client_sock, buffer, sizeof(buffer), 0);
        printf(" pressed %s\n", buffer);
        sendPacket(buffer);
        send(client_sock, "ok", 2, 0);
    }
}
static int init_socket()
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        exit(1);
    }

    listen(server_sock, 5);

    addr_size = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
    return client_sock;
}