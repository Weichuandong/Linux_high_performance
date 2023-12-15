## 第七章 Linux服务器程序规范

- 本章主要讨论服务器程序规范，也就是需要考虑的一些细节问题，这些问题涉及面广且零碎，而且基本是模板式的。
- 比如：
  - Linux服务器程序一般是以后台进程形式运行。
  - Linux服务器程序通常由一套日志系统，至少能输出到文件，有些能输出到专门的udp服务器。
  - Linux服务器程序一般以某个专门的非root身份运行。
  - Linux服务器程序通常是可配置的。
  - Linux服务器进程通常会在启动的时候生成一个PID文件并存入/var/run目录中，以记录该后台进程的PID。
  - Linux服务器程序通常需要考虑系统资源和限制，如进程可用文件描述符和内存总量等。

### 日志

#### Linux系统日志

- Linux提供了一个守护进程来处理日志—–syslogd，现在一般使用它的升级版—–rsyslogd。
- rsyslogd既能接收用户进程输出的日志，又能接收内核日志。

### 用户信息

#### UID,EUID,GID和EGID

- 分别为真实用户ID(UID)，有效用户ID(EUID)，真实组ID(GID)，有效组ID(EGID)。

  ```c++
  uid_t getuid();
  uid_t geteuid();
  gid_t getgid();
  gid_t getegid();
  int setuid(uid_t uid);
  int seteuid (uid_t uid);
  int setgid (gid_t gid);
  int setegid (gid_t gid);
  ```

  - uid_t和gid_t的真实类型都是unsigned int。

- 一个进程有两个用户ID：UID和EUID。EUID存在的目的是为了方便资源访问：它使得运行程序的用户拥有该程序的有效用户的权限。

- 比如su程序的所有者是root，并且设置了set-user-id标志。这个标志表明，任何普通用户运行su程序时，其有效用户就是su程序的所有者root。这样普通用户就能通过su程序访问/etc/passwd这个root才能访问的文件。

- 有效用户为root的进程称为特权进程。

- EGID的含义和EUID类似：给运行目标程序的组用户提供有效组的权限。

- 示例程序：

  ```c++
  #include <unistd.h>
  #include <stdio.h>
  
  int main()
  {
      uid_t uid = getuid();
      uid_t euid = geteuid();
      printf("userid is : %d, effective userid is : %d\n", uid, euid);
      return 0;
  }
  ```

  - 然后设置该程序的所有者为root，并设置set-user-id标志，然后运行程序。

#### 切换用户

- 可以将以root身份启动的进程切换为以一个普通用户身份运行

### 进程间关系

#### 进程组

- Linux下每个进程都属于一个进程组，因此他们除了PID还有PGID。

  ```c++
  pid_t getpgid(pid_t pid);  //获取进程的进程组id
  ```

  

- 每个进程组都有一个首领进程，其PID和PGID相同。

- 进程组将一直存在，直到其中所有进程都退出或加入其他进程组。

#### 会话

- 一些有关联的进程组将形成一个会话(session)。创建会话函数：

  ```c++
  pid_t setsid(void);
  ```

  - 不能由首领进程调用。
  - 调用进程将成为会话的首领，并且是新会话的唯一成员；
  - 新建一个进程组，其PGID就是调用进程的PID，调用进程成为该组的首领；
  - 调用进程将甩开终端(？)

#### 用ps命令查看进程关系

- 比如：

  ``` c++
  ps -o pid,ppid,pgid,sid,comm | less
  ```

### 系统资源限制

- 如物理资源限制(cpu,gpu，内存)，系统策略限制(cpu时间等)，具体实现的限制(文件名的最大长度)。

- 读取和设置系统资源限制：

  ```c++
  int getrlimit(int resource, struct rlimit *rlim);
  int setrlimit(int resource, const struct rlimit *rlim);
  ```

  - resource类型有比如：虚拟内存总量，cpu时间限制，文件大小限制等。

  ```c++
  struct rlimit{
  	rlim_t rlim_cur;
  	rlim_t rlim_max;
  }
  ```

  - rlim_t是整数类型，描述资源级别。
  - rlim_cur指定资源的软限制，建议性的限制，超过系统可能向进程发送信号以终止其运行。
  - rlim_max指定资源的硬限制，一般是软限制的上限。普通程序可以减小硬限制，只有root可以增加。

- 可以使用ulimit修改`当前shell`的资源限制，当shell关闭后失效。

- 可以通过修改`配置文件`来改变系统资源限制，永久生效。

### 改变工作目录和根目录

- 有些服务器需要改变工作目录和根目录。比如web服务器的根目录一般是/var/www。

### 服务器程序后台化

