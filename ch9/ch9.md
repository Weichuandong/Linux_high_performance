## I/O复用

- I/O复用使得程序能够同时监听多个文件描述符。以下网络程序可能会用到：
  - 客户端程序要同时处理多个socket。
  - 客户端程序要同时处理用户输入和网络连接。
  - TCP服务器要同时监听socket和连接socket。
  - 服务器要同时处理TCP请求和UDP请求。
  - 服务器要同时监听多个端口，或处理多种服务。
- I/O复用本身是阻塞的。

### select系统调用 [code](./9_1_select.cpp)

- 在一段指定时间内，监听用户感兴趣的文件描述符上的可读，可写和异常等事件。

#### select API

- 系统调用原型：

  ```c++
  int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);
  ```

  - nfds参数指定被监听的文件描述符的总数。

  - readfds，writefds，exceptfds分别指向可读，可写和异常等事件对应的文件描述符集合。fs_set结构体仅包含一个整数数组，该数组的每个元素的每一位(bit)标记一个文件描述符。fd_set能容纳的文件描述符数量由FD_SETSIZE指定，一般是1024。

  - 可以用下列宏来访问fd_set结构体中的位：

    ```c++
    FD_SET(int fd, fd_set *fdset)			//设置fdset的位fd
    FD_CLR(int fd, fd_set *fdset)			//清除fdset的位fd
    FD_ZERO(fd_set *fdset)		 			//清除fdset中的所有位
    FD_ISSET(int fd, fd_set *fdset)			//测试fdset的位fd是否被设置
    ```

  - timeout参数用来设置select函数的超时时间。内核可以修改它以告诉用户程序select等待了多久。

    ```c++
    struct timeval
    {
    	long tv_sec;		//秒数
    	long tv_usec;		//微秒
    }
    ```

#### 文件描述符就绪条件

- 以下情况socket可读：
  - socket内核接收缓冲区中的字节数大于等于其低水位标记SO_RCVLOWAT。
  - socket通信的对方关闭连接。此时对该socket的读操作将返回0。
  - 监听socket上有新的连接请求。
  - socket上由未处理的错误。
  - socket内核发送缓冲区中的可用字节数大于或等于其低水位标记SO_SNDLOWAT。
  - socket的写操作被关闭。
  - socekt使用非阻塞connect连接成功或失败之后。

#### 处理带外数据

- socket上接收到普通数据和带外数据都将使select返回，但是socket处于不同的就绪状态：前者处于可读状态，后者处于异常状态。

### poll系统调用

- poll和select类似，在指定时间内轮询一定数量的文件描述符，测试其中是否有就绪者。

  ```c++
  int poll(struct pollfd* fds, nfds_t nfds, int timeout);
  ```

  - `pollfd结构体fds`指定所有感兴趣的文件描述符上发生的可读，可写和异常事件。

    ```c++
    struct pollfd{
    	int fd;             //文件描述符
    	short events; 		//注册的事件
    	short revents;		//实际发生的事件，由内核填充
    }
    ```

    - poll事件类型有数据可读，数据可写，TCP连接被对方关闭，错误等。

  - `nfds`参数指定被监听事件集合fds的大小，其定义为：

    ```c++
    typedef unsigned long int nfds_t
    ```

  - `timeout`参数指定poll的超时值，单位为毫秒。为-1表示阻塞到某个事件发生；为0表示立即返回。

### epoll系列系统调用

#### 内核事件表

- epoll是linux特有的I/O复用函数。它的实现和使用和select，poll有很大差异。

- epoll使用一组函数而不是单个函数；

- epoll将用户关心的文件描述符上的事件放在内核的一个事件表里，无需每次调用都要重复传入文件描述符或事件集；

- epoll需要一个额外的文件描述符来唯一标识内核中事件表，这个文件描述符创建方式：

  ```c++
  int epoll_create(int size)
  ```

- 操作内核事件表：

  ```c++
  int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
  ```

  - epfd是内核事件表；

  - fd参数是要操作的文件描述符；

  - op指定操作类型：

    - EPOLL_CTL_ADD，往事件表中注册fd上的事件；
    - EPOLL_CTL_MOD，修改fd上的注册事件；
    - EPOLL_CTL_DEL，删除fd上的注册事件。

  - event参数指定事件，它是epoll_event结构指针类型，定义如下：

    ````c++
    struct epoll_event{
    	uint32_t   events;         //epoll事件
    	epoll_data_t data;		   //用户数据
    }
    ````
    
    ```c++
    struct union epoll_dat{
    	void* ptr;
    	int fd;
    	uint32_t u32;
    	uint64_t u64;
    }epoll_data_t;
    ```
    
    

#### epoll_wait函数

- epoll_wait函数在一段超时时间内等待一组文件描述符上的事件：

  ```c++
  int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);
  ```

  - 函数成功时返回就绪的文件描述符的个数；
  - timeout含义和poll接口的timeout参数相同；
  - maxevents参数指定最多监听多少个事件；
  - epoll_wait检测到事件，就将所有就绪的事件从内核事件表中复制到第二个参数events中。

### [poll_epoll](./9_2_poll_epoll.cpp)

#### LT和ET模式 [code](./9_3_LT_ET.cpp)

- epoll对文件描述符的操作有两种模式：LT(Level Trigger,电平触发)模式和ET(Edge Trigger，边沿模式)。
- LT是默认的工作模式，这种模式下epoll相当于一个效率较高的poll。当epoll_wait检测到其上有事件发生并通知应用程序后，应用程序可以不立即处理该事件，这样下一次epoll_wait还会向应用程序通告此事件直到该事件被处理。
- 当往epoll内核事件表中注册一个文件描述符上的EPOLLET事件时，epoll将以ET模式来操作该文件描述符，ET模式是epoll的高效工作模式。当epoll_wait检测到其上有事件发生并通知应用程序后，应用程序必须立即处理该事件，后续的epoll_wait不会再通知这一事件。

#### EPOLLONESHOT事件 [code](./epolloneshot.cpp)

- 保证操作系统最多触发其上注册的一个可读，可写或者异常事件，且只触发一次

- 注册了EPOLLONESHOT事件的socket一旦被某个线程处理完毕，该线程就应该立即重置这个socket上的EPOLLONESHOT事件，确保这个socket下一次可读时，其EPOLLIN事件能被触发。

- 本节代码由于使用了pthread，所以需要这样编译`g++ -pthread -o oneshot 9_4_epolloneshot.cpp `，原理可以参考[this](https://eli.thegreenplace.net/2013/07/09/library-order-in-static-linking)。大致是说程序链接的顺序很关键，如果A需要B的库，那么A的链接顺序必须在B前。

### 三组I/O复用函数的比较

| 比较                 | select                                                       | poll                                                         | epoll                                                        |
| -------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 事件集               | 用于通过3个参数分别传入感兴趣的可读，可写以及异常事件，内核通过对这些参数的在线修改来反馈其中的就绪事件。这也使得用户每次调用select都需要重置这3个参数 | 统一处理所有事件类型，因此只需要一个事件集参数。用户通过pollfd.events传入感兴趣的事件，内核通过修改pollfd.revents反馈其中就绪的事件 | 内核通过一个事件表管理用户所有感兴趣的事件。因此每次调用epoll_wait时，无须返回传入用户感兴趣的事件。epoll_wait的参数events仅用来反馈就绪事件 |
| 最大支持文件描述符数 | 有最大限制                                                   | 65535                                                        | 65535                                                        |
| 工作模式             | LT                                                           | LT                                                           | 支持ET高效模式                                               |
| 具体实现             | 轮询方式来检测就绪事件，时间复杂度O(n)                       | 轮询方式来检测就绪事件，时间复杂度O(n)                       | 回调方式来检测就绪事件，时间复杂度O(1)                       |

### I/O 复用的高级应用之一：非阻塞connect [code](./9_5_connect.cpp)

- connect函数需要等待服务器发送ACK报文后在返回，因此这个函数总会阻塞其调用进程一段时间
- 为了避免这段阻塞时间，可以使用非阻塞connect；
- 非阻塞I/O的返回值可能是EINPROGRESS，这表明连接还未建立，但是也不意味建立失败。需要等select，poll等函数返回后，再利用getsocketopt来读取错误码并清除该socket上的错误。如果错误码为0，表示连接建立成功

### I/O 复用的高级应用之一：聊天室程序 [code](./9_6_chat.cpp)















































