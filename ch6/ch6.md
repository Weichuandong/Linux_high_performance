## 第六章 高级I/O函数

- 创建文件描述符的函数，如pipe，dup/dup2函数；
- 读写数据的函数，如readv/writev等；
- 控制I/O行为和属性的函数，如fcntl函数。

### pipe函数

- 创建一个管道，实现进程间通信。

  ```c++
  int pipe(int fd[2]);
  ```

  - 参数为一个包含两个int整数的数组。
  - 实现一个单向的通道，数据传输方向为fd[0] <- fd[1]；也就是说fd[0]只能读，fd[1]只能写。
  - 默认情况下，这一对文件描述符都是阻塞的。
  - 如果管道的写端文件描述符fd[1]的引用计数减少至0，即没有进程往管道中写入文件，那么针对该管道的读端文件描述符fd[0]的read操作将返回0，即读到了文件结束标志(End of File, EOF)；反之，如果管道的读端文件描述符fd[0]的引用计数减少至0，即没有进程从管道中读取文件，那么针对该管道的写端文件描述符fd[1]的write操作将返回失败，并引发SIGPIPE信号。
  - 管道内部传输的数据是字节流，和TCP字节流概念相同。区别在于大小默认为65536字节(可通过fcntl修改)。

- 创建双向管道函数：

  ```c++
  int socketpair(int domain, int type, int protocol, int fd[2]);
  ```

  - 前三个参数的含义和socket系统调用完全相同，但domain只能为AF_UNIX，因为只能在本地使用这个双向通道。fd的含义和pipe系统调用一样。

### dup函数和dup2函数

- 需要将标准输入重定向到一个文件，或者将标准输出重定向到一个网络连接(如CGI编程)。就可以通过这两个函数实现：

  ```c++
  int dup(int file_descriptor);
  int dup2(int file_descriptor_one, int file_descriptor_two);
  ```

- 问题： 写的程序6-1_serv和6-1_clie能够建立连接，但是客户程序收不到服务程序发的数据。

  - 解决：服务器启动6_1_serv程序，再用telnet访问。


### readv和writev函数

- readv函数将数据从文件描述符读到分散的内存块中；

- writev函数将多块分散的内存数据写入文件描述符中；

  ```c++
  ssize_t readv(int fd, const struct iovec* vector, int count);
  ssize_t writev(int fd, const struct iovec* vector, int count);
  ```

- 比如服务器端响应一个http请求。http应答头部等信息往往在一个内存块，而具体文件在另一个内存块中，可以通过writev将这个内存块中的内容同时发出。

### sendfile函数

- 在两个文件描述符之间直接传输数据(完全在内核中操作),从而避免的内核缓冲区和用户缓冲区之间的数据拷贝。

- 这是零拷贝(指计算机执行操作时，CPU不需要先将数据从某处内存复制到另一个特定区域，常用于通过网络传输文件时节省CPU周期和内存带宽)

  ````c++
  ssize_t sendfile(int out_fd, int in_fd, off_t* offset, size_t count);
  ````

  - in_fd是待读出内容的文件描述符，必须指向真实的文件。
  - out_fd是待写入内容的文件描述符，必须为socket。
  - offset指定从读入文件流的哪个位置开始读，为空则使用起始位置。
  - count指定传输的字节数。

### mmap函数和munmap函数

- mmap函数用于申请一段内存空间。可以将这段内存用于进程间通信的共享内存，也可以将文件映射到其中。

- munmap用于释放mmap创建的空间。

  ```c++
  void* mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
  int munmap(void *start, size_t length);
  ```

  - start允许用户自己指定某个地址作为申请内存的起始地址。如果为NULL，则系统自动分配。
  - length指定内存段的长度。
  - prot设置权限，读，写，执行等
  - fd参数是被映射文件对应的文件描述符，一般通过open系统调用获得；offset设置从文件的何处开始映射。

### splice函数

- 用于在两个文件描述符之间移动数据。是零拷贝。

  ```c++
  ssize_t splice(int fd_in, loff_t* off_in, int fd_out, loff_t* off_out, size_t len, unsigned int flags);
  ```

  - fd_in 是待输入数据的文件描述符；如果为管道文件描述符，那么off_in必须为NULL，其他情况off_in表示从输入数据流的何处开始读取数据。
  - fd_out和off_out除了表示输出数据流，其他和fd_in/off_in相同。
  - fd_in和fd_out至少有一个管道文件描述符。
  - len表示传输文件的长度。

### tee函数

- 用于在两个管道文件描述符之间复制数据，也是零拷贝。

  ```c++
  ssize_t tee(int fd_in, int fd_out, size_t len, unsigned int flags);
  ```

  - 参数含义基本和splice相同，只不过都是管道文件描述符。

### fcntl函数

- file control，提供了对文件描述符的各种控制操作。

  ```c++
  int fcntl(int fd, int cmd, ...);
  ```

- 在网络编程中，fcntl常用来将一个文件描述符设置为非阻塞的。

