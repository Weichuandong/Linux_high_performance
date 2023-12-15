## 第五章 Linux网络编程基础API

- 探讨Linux网络编程基础API与内核TCP/IP协议族之间的关系。

### socket地址API

#### 主机字节序和网络字节序

- 字节序指的是多个字节如何在内存中排列组成数字。
- 大端字节序(big endian)：整数的高位字节存储在内存的低地址处，低位字节存储在内存的高地址处。
- 小端字节序(small endian)：和大端字节序相反。整数的高位字节存储在内存的高地址处，低位字节存储在内存的低地址处
- 假设有一个两字节的数字0x0102，占据了内存0x00000001~0x00000002地址，那他们在内存中的存放顺序应该是：

| 字节序 | 0x00000001(低地址) | 0x00000002(高地址) |
| ------ | ------------------ | ------------------ |
| 大端   | 01(高字节)         | 02(低字节)         |
| 小端   | 02(低字节)         | 01(高字节)         |

- 现代PC一般采用小端字节序，又称为主机字节序。
- 网络中传输一般采用大端字节序，故又称为网络字节序。

#### 通用socket地址

- socket网络编程中表示socket地址的结构体sockaddr，定义如下：

  ```c++
  #include <bits/socket.h>
  struct sockaddr
  {
  	sa_family_t	sa_family;	/* address family, AF_xxx	*/
  	char	  sa_data[14];	/* 14 bytes of protocol address	*/
  }
  ```

- sa_family_t是地址族类型，本质类型是unsigned short，它和协议族类型对应。常见的如下：

  | 协议族   | 地址族   | 描述             |
  | -------- | -------- | ---------------- |
  | PF_UNIX  | AF_UNIX  | UNIX本地域协议族 |
  | PF_INET  | AF_INET  | TCP/IPv4协议族   |
  | PF_INET6 | AF_INET6 | TCP/IPv6协议族   |

- 宏`PF_*`和`AF_*`都定义在bits/socket.h头文件中，并且有相同的值，所以两者通常混用。

- `sa_data`用于存放socket地址值。不同的协议族的地址值有不同的含义和长度。如：

  | 协议族   | 地址值含义和长度                                   |
  | -------- | -------------------------------------------------- |
  | PF_UNIX  | 文件路径名，可达108字节                            |
  | PF_INET  | 16bit端口号和32bitIPv4地址，共6字节                |
  | PF_INET6 | 16bit端口号和128bitIPv6地址，32bit范围ID，共26字节 |

- 而14字节的`sa_data`无法存放这些地址，所以linux定义了如下结构体，这个结构体不仅提供了足够的空间存放地址值，而且内存是对齐的(__ss_align成员的作用)

  ```c++
  #include <bits/socket.h>
  struct sockaddr_storage
  {
  	ss_family_t sa_family;                     
  	unsigned long int __ss_align;
  	char __ss_padding[128-sizeof(__ss_align)];
  }
  ```

#### 专用socket地址

- 通用的地址并不好用，所以linux又提供了专门的地址结构体，如：

  - UNIX本地域协议族：

  ```c++
  #include <sys/un.h>
  
  #define UNIX_PATH_MAX	108
  
  struct sockaddr_un
  {
      sa_family_t sin_family;          /* AF_UNIX */
      char sun_path[UNIX_PATH_MAX];    /* pathname */
  }
  ```

- TCP/IPv4协议族：


  ```c++
  stuct sockaddr_in{
  	sd_family_t sin_family;    /* Address family		*/
      u_int16_t sin_port;		   /* Port number			*/
      struct in_addr sin_addr;   /* Internet address		*/
  }
  
  struct in_addr{
      u_int32_t s_addr;          
  }
  ```


- 所有的`socket专用地址`(以及sockaddr_storage)类型的变量在使用时都需要转换为`通用socket地址`类型sockaddr(强制转换即可)，因为所有socket编程接口使用的地址参数都是sockaddr。

#### IP地址转换函数

- IP地址有多种的表达方式。出于可读性考虑就可以使用点分十进制；出于编程中使用目的需要使用二进制数字。所以需要有转换的方法。

  - 适用于IPv4的：

  ````c++
  #include <arpa/inet.h>
  
  in_addr_t inet_addr(const char* strptr); // 将点分十进制转换为网络字节序表示的IPv4地址
  int inet_aton(const char* cp, struct in_addr* inp); //同上，但是结果保存在inp；
  char* inet_ntoa(struct in_addr in);//将网络字节序表示的IPv4地址转换为点分十进制。
  ````

  - IPv4和IPv6通用：

  ```c++
  #include <arpa/inet.h>
  
  int inet_pton(int af, const char* src, void* dst); //将用字符串表示的地址src转换为网络字节序表示的IP地址并存放于dst中，af表示地址族；
  const char* inet_ntop(int af, const void* src, char* dst, socklen_t cnt); //与以上相反，将网络字节序表示的IP地址src转换为字符串保存于dst中，cnt表示目标存储单元的大小。
  ```

### 创建socket

- UNIX/Linux的一个哲学是：所有东西都是文件。socket也不例外，它是可读，可写，可控制，可关闭的文件描述符。

  ```c++
  int socket(int domain, int type, int protocol);
  ```

  - domain表示协议族。如PF_INET表示IPv4，PF_INET6表示IPv6。
  - type指定服务类型。主要有SOCK_STREAM(流服务)和SOCK_UGRAM(数据报)服务。对TCP/IP而言，流服务表示传输层使用TCP，数据报服务使用UDP。
  - protocol是在前两个参数构成的协议集合下，再选择一个具体的协议。不过这个值一般都是唯一的(前两个参数已经可以唯一确认)。故大多情况都直接设置为0，表示使用默认协议。

### 命名socket

- 将一个socket和socket地址绑定称为给socket命名。

  - `服务端`程序中，一般需要命名socket，这样客户端才能知道如何连接他。
  - `客户端`程序通常不需要命名，而是采用匿名方式，由操作系统自动分配socket地址。

- 命名的系统调用为bind：

  ```c++
  int bind(int sockfd, const struct sockaddr* my_addr, socklen_t addrlen);
  ```

  - 将my_addr所指的socket地址分配给未命名的sockfd文件描述符，addrlen参数指出该socket地址的长度。

### 监听socket

- socket命名后，还需要创建监听队列以存放待处理的客户连接：

  ```c++
  int listen(int sockfd, int backlog);
  ```

  - sockfd指定被监听的socket。backlog参数提示内核监听队列的最大长度，如果监听队列长度超过backlog，服务器将不受理新的客户连接，客户端也将收到ECONNREFUSED错误信息。
  - 监听队列分为`全连接队列`（linux2.2后，backlog参数仅指定全连接队列长度，队列长度一般能略大于backlog）和`半连接队列`（linux2.2后由系统参数 net/ipv4/tcp_max_syn_backlog限制）：
    - 服务器收到第一次握手后，会将sock放入`半连接队列`，队列中都处于`SYN_RECV`状态。数据结构是`哈希表`，因为涉及一个查找sock并放入全连接队列的过程，哈希表查找效率为O(1)。
    - 服务器收到第三次握手后，会将sock放入`全连接队列`，队列中都处于`ESTABLISHED`状态。数据结构是`链表`。
  - 从这也能看出，`一个服务器端口`能够建立`多个TCP连接`。因为客户端的端口或IP会不同，所以还是能区分数据是由哪个客户发送而来的。

### 接受连接

- 从listen监听队列中接受一个连接：

  ```c++
  int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
  ```

  - sockfd是执行过listen系统调用的`监听socket`。addr获取被接受连接的源端socket地址，该地址长度由addrlen指出。
  - 该socket唯一地标识了被接受的这个连接，服务器可通过读写该socket来与被接受连接对应的客户端通信。
  - accept只是从监听队列中取出连接，而不论连接出于何种状态，不关心网络的变化。

### 发起连接

- 客户端需要主动发起连接：

  ```c++
  int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
  ```

  - sockfd是socket系统调用返回的一个描述符，serv_addr是服务器监听的socket地址，addrlen指定了这个地址的长度。

### 关闭连接

- 关闭连接实际就是关闭该连接对应的socket，这可以通过关闭普通文件描述符的系统调用实现。

  ```c++
  int close(int fd);
  ```

  - fd参数是需要关闭的socket。但close系统调用并非总是立即关闭一个连接，而是将fd的引用计数减1，直到为0时才真正的关闭。

- 如果无论如何都需要立即关闭连接，可以使用shutdown系统调用：

  ```c++
  int shutdown(int sockfd, int howto);
  ```

  - sockfd是待关闭的socket，howto是如何关闭，如：

    | 可选值    | 含义                                             |
    | --------- | ------------------------------------------------ |
    | SHUT_RD   | 关闭读的一半，并将读缓冲区中数据都丢弃           |
    | SHUT_WR   | 关闭写的一半，但是会将发送缓冲区中的数据都发出去 |
    | SHUT_RDWR | 综上两种                                         |

### 数据读写

#### TCP数据读写

- 专用于TCP流数据读写的系统调用：

  ```c++
  ssize_t recv(int sockfd, void *buf, size_t len, int flags);
  ssize_t send(int sockfd, const void *buf, size_t len, int flags)
  ```

  - recv根据sockfd去读取对应连接中数据，buf和len分别指定`读缓冲区`的位置和大小。
  - send往sockfd上写入数据，buf和len分别指定`写缓冲区`的位置和大小
  - flag是一些可选值。

#### UDP数据读写

- 用于UDP数据报读写的系统调用：

  ```c++
  ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
  ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, struct sockaddr* dest_addr, socklen_t* addrlen);
  ```

  - 前四个参数含义和TCP系统调用相同；
  - 由于UDP没有连接的概念，所以每次发送或接收都需要指定地址。后面两个参数，src_addr代表发送端的socket地址，dest_addr代表接收端的socket地址。

#### 通用数据读写函数

- 不仅能用于TCP流数据，也能用于UDP数据报：

  ```c++
  ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags);
  ssize_t sendmsg(int sockfd, struct msghdr* msg, int flags);
  ```

  ```c++
  struct msghdr
  {
  	void* msg_name;            /* socket地址 */
      socklen_t msg_namelen;     /* socket地址的长度 */
      struct iovec* msg_iov;	   /* 分散的内存块 */
      int msg_iovlen;			   /* 分散内存块的数量 */
      void* msg_control; 		   /* 指向辅助数据的起始位置 */
      socklen_t msg_contollen;   /* 辅助数据大小 */
      int msg_flags;			   /* 复制函数中的flags参数，并在调用过程中更新 */
  }
  ```

  ```c++
  struct iovec
  {
  	void *iov_base;  /* 内存起始地址 */
      size_t iov_len;  /* 这块内存长度 */
  }
  ```

  - msg_name对TCP来说没有意义，UDP则需要设置。

  - iovec指定了一块内存的起始位置和长度；而msg_iov则指定这样的iovec结构体有多少。
    - `分散读`：对recvmsg来说，数据将被读取并存放在这些分散内存中；
    - `集中写`：对于sendmsg来说，这些分散内存中的数据将被一并发送；

### 地址信息函数

- 可以用于获取一个连接的本端和远端的socket地址：

  ```c++
  int getsockname(int sockfd, struct sockaddr* address, socklen_t* address_len);
  int getpeername(int sockfd, struct sockaddr* address, socklen_t* address_len);
  ```

  - getsockname获取sockfd对应的本端socket地址并存储与address中；
  - getpeername获取sockfd对应的远端socket地址并存储与address中；

### socket选项

- 用于读取和设置socket文件描述符属性：

  ```c++
  int getsockopt(int sockfd, int level, int option_name, void* option_value, socklen_t* restrict option_len);
  int setsockopt(int sockfd, int level, int option_name, const void* option_value, socklen_t option_len);
  ```

  - sockfd指定被操作的目标socket；
  - level指定要操作哪个协议的属性，如IPv4，IPv6，TCP等；
  - option_name指定选项的名字；
  - option_value和option_len 则是被操作选项的值和长度。

#### SO_REUSEADDR选项

- 通用的socket选项，表示可以重用本地地址。也就是服务器可以强制使用处于`TIME_WAIT`状态的socket地址。

  ```c++
  int sock = socket(PF_INET, SOCK_STREAM, 0);
  assert(sock >= 0);
  int reuse = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  ```

#### SO_RCVBUF和SO_SNDBUF选项

- 分别表示TCP接收缓冲区和发送缓冲区的大小。当使用这两个参数来设置时，系统都会将其值加倍，并且不小于某个值，比如256B和2048B。

  ```c++
  int sock = socket(PF_INET, SOCK_STREAM, 0);
  assert(sock >= 0);
  int recvbuf = 50;
  int sendbuf = 2000;
  setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));
  setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(revbbuf));
  
  getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuf, (socklen_t*)&sizeof(sendbuf));
  getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &sendbuf, (socklen_t*)&sizeof(recvbuf));
  ```

  - 程序最后设置的接收缓存和发送缓冲分别是256和4000。

#### SO_RCVLOWAT和SO_SNDLOWAT选项

- 分别表示TCP接收缓冲区和发送缓冲区的低水位标志。一般被I/O复用系统调用表示socket是否可读或可写。
  - 接收缓冲区的可读数据大于其低水位标志；
  - 发送缓冲区的空闲空间大于其低水位标志。

#### SO_LINGER选项

- 用来控制close系统调用在关闭TCP连接时的行为。

### 网络信息API

- socket地址的两个要素IP和端口都是用数值表示的。不便于记忆，也不便于扩展。可以通过某些方法实现主机名到IP地址，服务名称到端口号的转换。

#### gethostbyname和gethostbyaddr

- gethostbyname可以根据主机名获取主机的完整信息。

- gethostbyaddr根据IP地址获取主机的完整信息。

- 定义如下：

  ````c++
  struct hostent* gethostbyname(const char* name);
  struct hostent* gethostbyaddr(const void* addr, size_t len, int tye);
  ````

  ```c++
  struct hostent
  {
  	char*  h_name;       /* 主机名 */
  	char** h_aliases;    /* 主机别名 */
  	int h_addrtype;		 /* 地址类型 */
  	int h_length;		 /* 地址长度 */
  	char** h_addr_list	 /* 地址列表(网络字节序) */
  }
  ```

#### getservbyname和getservbyport

- getservbyname根据名称获取某个服务的完整信息。

- getservbyport根据端口号获取某个服务的完整信息。

- 定义如下：

  ```c++
  struct servent* getservbyname(const char* name, const char* proto);
  struct servent* getservbyport(int port, const char* proto);
  ```

  ```c++
  struct servent
  {
  	char* s_name;		 /* 服务名称 */
  	char** s_aliases;    /* 服务别名 */
  	int s_port;			 /* 端口号 */
  	char* s_proto;		 /* 服务类型 */
  }
  ```

#### getaddrinfo

- 既能通过主机名获得IP地址(内部使用gethostbyname),又能使用服务名获得端口号(内部使用getservbyname函数)。

- 定义如下：

  ```c++
  int getaddrinfo(const char *hostname,
  			const char *service,
  			const struct addrinfo *hints,
  			struct addrinfo **result);
  ```

  - hostname可以接受主机名或字符串表示的IP地址；service可以接收服务名或字符串表示的十进制端口号。

  ```c++
  struct addrinfo
  {
    int ai_flags;			/* Input flags.  */
    int ai_family;		/* Protocol family for socket.  */
    int ai_socktype;		/* Socket type.  */
    int ai_protocol;		/* Protocol for socket.  */
    socklen_t ai_addrlen;		/* Length of socket address.  */
    struct sockaddr *ai_addr;	/* Socket address for socket.  */
    char *ai_canonname;		/* Canonical name for service location.  */
    struct addrinfo *ai_next;	/* Pointer to next in list.  */
  };
  ```

#### getnameinfo

- 能通过socket地址同时获得字符串表示的主机名(内部使用gethostbyaddr)和服务名(内部使用getservbyport)。

- 定义如下：

  ```c++
  int getnameinfo (const struct sockaddr *sockaddr, socklen_t addrlen, 
                   char *host, socklen_t hostlen, 
                   char *serv, socklen_t servlen, 
                   int __flags);
  ```

