#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>

#define BUFFER_SIZE 4096 // 缓冲区大小
// 主状态机的两种可能状态：
enum CHECK_STATE
{
    CHECK_STATE_REQUESTLINE = 0, // 当前正在分析请求行
    CHECK_STATE_HEADER           // 当前正在分析头部字段
};
// 行的读取状态：
enum LINE_STATUS
{
    LINE_OK = 0, // 读取到完整行
    LINE_BAD,    // 行出错
    LINE_OPEN    // 行数据还不完整
};
// 服务器处理HTTP请求的结果：
enum HTTP_CODE
{
    NO_REQUEST,        // 请求不完整，需要继续读取客户数据
    GET_REQUEST,       // 获得了完整请求
    BAD_REQUEST,       // 请求有错误
    FORBIDDEN_REQUEST, // 客户对资源没有足够访问权限
    INTERNAL_ERROR,    // 服务器内部错误
    CLOSED_CONNECTION  // 客户端已经关闭连接
};
// 构造的伪回复信息
static const char *szret[] = {"I get a correct result\n", "Something wrong\n"};

// 从状态机，用于解析一行内容
LINE_STATUS parse_line(char *buffer, int &checked_index, int &read_index)
{
    // 分析用户缓冲区中[checked_index, read_index)的内容
    char temp;
    for (; checked_index < read_index; ++checked_index)
    {
        temp = buffer[checked_index];
        if (temp == '\r') // 回车符，说明可能读到一个完整的行
        {
            if ((checked_index + 1) == read_index)
            {
                return LINE_OPEN;
            }
            else if (buffer[checked_index + 1] == '\n') // 读到了完整行，并且将\r\n置为\0\0
            {
                buffer[checked_index++] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n') // 换行符，也可能读取到一个完整的行
        {
            if ((checked_index > 1) && buffer[checked_index - 1] == '\r') // 读到了完整行，并且将\r\n置为\0\0
            {
                buffer[checked_index - 1] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}
// 分析请求行，比如  GET https://www.baidu.com/content-search.xml HTTP/1.1
HTTP_CODE parse_requestline(char *temp, CHECK_STATE &checkstate)
{
    char *url = strpbrk(temp, " \t"); // 返回第一个为"\t"或" "的字符索引
    if (!url)
    {
        return BAD_REQUEST;
    }
    *url++ = '\0';

    char *method = temp;
    if (strcasecmp(method, "GET") == 0) // 比较两个字符串是否相等,仅支持GET方法
    {
        printf("The request method is GET\n");
    }
    else
    {
        return BAD_REQUEST;
    }

    url += strspn(url, " \t"); // 返回字符串中" \t"的位置
    char *version = strpbrk(url, " \t");
    if (!version)
    {
        return BAD_REQUEST;
    }
    *version++ = '\0';
    version += strspn(version, " \t");
    // 仅支持 HTTP/1.1
    if (strcasecmp(version, "HTTP/1.1") != 0)
    {
        return BAD_REQUEST;
    }
    // 检查URL是否合法
    if (strncasecmp(url, "http://", 7) == 0)
    {
        url += 7;
        url = strchr(url, '/');
    }

    if (!url || url[0] != '/')
    {
        return BAD_REQUEST;
    }

    printf("The request URL is: %s\n", url);
    // HTTP请求行处理完毕，状态转移到头部字段的分析
    checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}
// 分析头部字段
HTTP_CODE parse_headers(char *temp)
{
    if (temp[0] == '\0')
    {
        return GET_REQUEST;
    }
    else if (strncasecmp(temp, "Host:", 5) == 0) // 处理"HOST"头部字段
    {
        temp += 5;
        temp += strspn(temp, " \t");
        printf("the request host is: %s\n", temp);
    }
    else
    {
        printf("I can not handle this header\n");
    }

    return NO_REQUEST;
}
// 分析HTTP请求的入口函数
HTTP_CODE parse_content(char *buffer, int &checked_index, CHECK_STATE &checkstate, int &read_index, int &start_line)
{
    LINE_STATUS linestatus = LINE_OK; // 记录当前行状态
    HTTP_CODE retcode = NO_REQUEST;   // 记录HTTP请求的处理结果
    // 直到从buffer中读到完整行
    while ((linestatus = parse_line(buffer, checked_index, read_index)) == LINE_OK)
    {
        char *temp = buffer + start_line;
        start_line = checked_index;
        // 主状态机当前的状态
        switch (checkstate)
        {
        case CHECK_STATE_REQUESTLINE: // 分析请求行
        {
            retcode = parse_requestline(temp, checkstate);
            if (retcode == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        }
        case CHECK_STATE_HEADER: // 分析头部字段
        {
            retcode = parse_headers(temp);
            if (retcode == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            else if (retcode == GET_REQUEST)
            {
                return GET_REQUEST;
            }
            break;
        }
        default:
        {
            return INTERNAL_ERROR;
        }
        }
    }
    if (linestatus == LINE_OPEN)
    {
        return NO_REQUEST;
    }
    else
    {
        return BAD_REQUEST;
    }
}

int main()
{
    const char *ip = "127.0.0.1";
    int port = 50000;

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    int ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int fd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
    if (fd < 0)
    {
        printf("errno is: %d\n", errno);
    }
    else
    {
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE);
        int data_read = 0;
        int read_index = 0;    // 当前已经读取了多少字节的数据
        int checked_index = 0; // 当前已经分析完了多少字节的数据
        int start_line = 0;    // 行在buffer中的起始位置
        CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;
        while (1)
        {
            data_read = recv(fd, buffer + read_index, BUFFER_SIZE - read_index, 0);

            if (data_read == -1)
            {
                printf("reading failed\n");
                break;
            }
            else if (data_read == 0)
            {
                printf("remote client has closed the connection\n");
                break;
            }

            read_index += data_read;
            HTTP_CODE result = parse_content(buffer, checked_index, checkstate, read_index, start_line);
            if (result == NO_REQUEST)
            {
                continue;
            }
            else if (result == GET_REQUEST)
            {
                send(fd, szret[0], strlen(szret[0]), 0);
                break;
            }
            else
            {
                send(fd, szret[1], strlen(szret[1]), 0);
                break;
            }
        }
        close(fd);
    }

    close(listenfd);
    return 0;
}
