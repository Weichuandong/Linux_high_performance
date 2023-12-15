#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main()
{
    const char *ip = "127.0.0.1";
    int port = 50002;

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr *)&client, &client_addrlength);
    if (connfd < 0)
    {
        printf("errno is : %d\n", errno);
    }
    else
    {
        close(STDOUT_FILENO); // 关闭标准输出文件描述符
        // const char *buffer = "hfasdnoifnsaof";
        // send(connfd, buffer, strlen(buffer), 0);
        dup(connfd);      // 复制socket文件描述符connfd，因为dup总是返回系统中最小可用的文件描述符，所以返回是1.即为之前关闭的标准输出文件描述符的值。
        printf("abcd\n"); // 服务器的标准输出就会发送到connfd上
        close(connfd);
    }

    close(sock);
    return 0;
}