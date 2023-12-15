#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>

int main()
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    const char *ip = "127.0.0.1";
    int port = 50000;

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int connfd = connect(sock, (struct sockaddr *)&address, sizeof(address));
    if (connfd < 0)
    {
        printf("errno is : %d\n", errno);
    }
    else
    {
        char remote[INET_ADDRSTRLEN];
        printf("connected with ip: %s and port: %d\n", inet_ntop(AF_INET, &address.sin_addr, remote, sizeof(remote))), ntohs(address.sin_port);
        while (true)
        {
            char data[108];
            memset(data, '\0', sizeof(data));
            int rst = recv(connfd, data, sizeof(data), 0);
            if (rst > 0)
            {
                printf("ret = %d, data = %s", rst, data);
                break;
            }
        }
    }

    return 0;
}