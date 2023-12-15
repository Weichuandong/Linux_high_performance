#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

int main()
{
    const char *ip = "127.0.0.1";
    int port = 12000;
    const char *file_name = "6-1_CGI_clie.cpp";

    int filefd = open(file_name, O_RDONLY);
    assert(filefd > 0);
    struct stat stat_buf;     // 一个关于文件的描述信息
    fstat(filefd, &stat_buf); // 填充文件信息

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
        sendfile(connfd, filefd, NULL, stat_buf.st_size); // 传输大小为文件的大小
        close(connfd);
    }

    close(sock);
    return 0;
}