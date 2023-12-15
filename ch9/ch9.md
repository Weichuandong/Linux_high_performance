## I/O复用

- I/O复用使得程序能够同时监听多个文件描述符。以下网络程序可能会用到：
  - 客户端程序要同时处理多个socket。
  - 客户端程序要同时处理用户输入和网络连接。
  - TCP服务器要同时监听socket和连接socket。
  - 服务器要同时处理TCP请求和UDP请求。
  - 服务器要同时监听多个端口，或处理多种服务。
- I/O复用本身是阻塞的。

### select系统调用

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











​	