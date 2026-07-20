# 深入探讨文件 IO

本章围绕 Linux 文件 IO 展开，主要讨论以下话题：

- 函数返回错误的处理（`errno` / `perror`）以及进程退出方式 `exit()` / `_Exit()` / `_exit()`
- 空洞文件（sparse file）的概念与观察方式
- `open()` 标志位：`O_APPEND`、`O_TRUNC`、`O_CREAT`
- 复制文件描述符（`dup` / `dup2` / `dup3`）
- 文件共享：进程表项、文件描述符表、`struct file` 三层结构
- 原子操作与竞争冒险（`O_APPEND` 与 `pread` / `pwrite`）
- 系统调用 `fcntl()` / `ioctl()`
- 截断文件（`truncate()` / `ftruncate()`）

---

## 错误处理与 errno

`open(2)` 的手册页对返回值有明确约定：

```txt
RETURN VALUE
       open(), openat(), and creat() return the new file descriptor
       (a nonnegative integer), or -1 if an error occurred (in which
       case, errno is set appropriately).
```

失败时，函数返回 -1，并把错误码写入全局变量 `errno`。常见用法有两种：

- `strerror(errno)`：把错误码翻译成可读字符串，自己负责打印。
- `perror("open")`：把“前缀 + errno 对应文本”直接打到 stderr 上，使用最多。

最后再决定退出方式：`exit()`（C 库，会刷新 stdio buffer）、`_Exit()` 与 `_exit()`（直接进入内核，不刷 buffer）。

---

## 空洞文件（hole file / sparse file）

空洞文件是指逻辑大小中存在未被写入的区域，这些区域不占用实际磁盘块；读操作时返回全零。这类文件也叫稀疏文件（sparse file）。

### 创建方式

用 `lseek` 把文件偏移推到超过末尾的位置，再 `write`，中间的“未写区域”就成为空洞：

```c
fd = open("./hole_file", O_WRONLY | O_CREAT | O_EXCL,
          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

/* 偏移到 4096，中间 0~4095 字节成为空洞 */
lseek(fd, 4096, SEEK_SET);

/* 写入 4KB 数据，文件逻辑大小为 8KB，实际磁盘占用仅 4KB */
write(fd, buf, 4096);
```

### 如何观察空洞

| 命令          | 显示内容                       |
|---------------|--------------------------------|
| `ls -lh`      | 文件逻辑大小（包含空洞）       |
| `du -h`       | 实际磁盘占用（空洞不占块）     |

`ls` 和 `du` 二者差值即为空洞大小。比如 `ls` 显示 8K、`du` 显示 4K，说明有 4K 的空洞。

### 空洞文件存在的意义

**1. 节省磁盘空间**
逻辑 100GB 的文件，如果大部分区域是空洞，实际占用可能只有几百 MB。

**2. 高效支持随机写入**
没有空洞机制时，往“末尾之前”的位置写入会强制把中间区域填零，产生大量无意义 IO 和磁盘占用。

**3. 简化应用层语义**
文件系统天然保证空洞读出 `\0`，应用层不需要额外特殊处理。

### 典型应用场景

- 数据库文件（SQLite、Berkeley DB）：按页管理，创建时预分配逻辑大小，按需写入，未写入页就是空洞。
- 虚拟机磁盘镜像（qcow2、raw）：精简置备（thin provisioning）。
- 核心转储（core dump）：未映射地址段对应空洞。
- BitTorrent 下载：预分配文件大小，按块写入，未完成的块保持空洞。

### 注意事项

- 取决于文件系统。ext4 / XFS / btrfs 完整支持；**WSL 的 DrvFs 不传递稀疏能力**，在 `/mnt/d/` 等 Windows 盘路径下空洞会被实际填零并占盘。要观察空洞，应把代码放到 WSL 原生 ext4 路径（如 `/home/`）下编译运行。
- 备份工具：`tar` / `rsync` 默认会展开空洞（填零），要用 `tar --sparse` 等选项保留。
- 文件传输：`cp --sparse=always`，或 `rsync --sparse`，才能保留空洞。

---

## open() 的标志位：O_CREAT / O_TRUNC / O_APPEND

`open()` 的 flags 是位掩码，常用三个：`O_CREAT`、`O_TRUNC`、`O_APPEND`。

### O_CREAT：缺失 mode 参数是个老坑

`open()` 第三个参数只在带 `O_CREAT` / `O_TMPFILE` 时才有意义，类型是 `mode_t`（也就是八进制权限位）。常见正确写法：

```c
fd = open("append.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);
```

如果不写 mode：

```c
fd = open("append.txt", O_WRONLY | O_APPEND | O_CREAT);  /* 错误 */
```

编译器通常只给一个“unused parameter”这类弱警告，但运行时会从栈上抓一段“脏数据”当文件权限，结果文件权限不可预测。**带了 `O_CREAT` 就一定要写 `mode`。**

### O_TRUNC：截断已有内容

`O_TRUNC` 的语义是：仅当文件以可写方式成功打开时，才把文件长度截断为 0，丢弃已有内容。**对只读打开无作用**，对不存在的文件也无作用。

练习示例（`practice/part3/o_trunc.c`）：

```c
fd = open("test.txt", O_WRONLY | O_TRUNC);  /* 文件被清空 */
if (fd == -1) {
    perror("open");
    exit(-1);
}
close(fd);
```

使用上的注意：

- `O_TRUNC` 不会备份，只清空。开发中涉及“修改已有文件”前，养成先备份再动的习惯。
- 不要混淆 `O_TRUNC`（只动 file size）和后面章节会讲的 `truncate()` / `ftruncate()`（同一个语义，但通过系统调用触发）。

### O_APPEND：每次 write 都强制追加到末尾

`O_APPEND` 的语义是：**每次 `write()` 之前**，内核先把文件偏移设到文件末尾，再写入。

关于 `O_APPEND` 的几个常见误解，以及这次踩到的坑，逐条澄清：

#### 1. O_APPEND 与访问模式：必须给读权限才能 read

`read()` 不关心当前文件偏移，只检查 fd 是否有读权限。`O_WRONLY` 打开的 fd 上 `read()` 会失败，错误码是 `EBADF`，`perror("read")` 输出 `Bad file descriptor`：

```c
fd = open("append.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);
read(fd, buffer, 4);   /* errno = EBADF */
```

POSIX 把“fd 无效”和“fd 访问模式不允许”都归到 `EBADF`，名称虽然叫 Bad file descriptor，其实是“访问模式不对”。

要 read，最省事的就是把打开方式改成 `O_RDWR`：

```c
fd = open("append.txt", O_RDWR | O_APPEND | O_CREAT, 0644);
```

或者另外 `open()` 一个只读 fd，但单 fd 自包含读写的最小改动就是上面这行。

#### 2. O_APPEND 不“锁定”偏移：lseek 之后还能随机写

`O_APPEND` 只保证：每次 `write()` 前先把偏移设到末尾。**它不会改变后续 `lseek()` 的行为。**

也就是说，下面这段代码在 `O_APPEND` 下完全合法：

```c
fd = open("append.txt", O_RDWR | O_APPEND | O_CREAT, 0644);
memset(buf, 0x55, sizeof(buf));
write(fd, buf, sizeof(buf));        /* 追加到末尾，O_APPEND 兜底 */
lseek(fd, -4, SEEK_END);            /* 移动到末尾前 4 字节 */
write(fd, "ABCD", 4);               /* 不会强制追加，OK，写到这里 */
```

要“严格只在末尾写”，得自己用 `lseek(fd, 0, SEEK_END)`，或者直接用 `pwrite()`，不要把 O_APPEND 当成“防随机写”的保险。

#### 3. strlen() 写在没有 '\0' 的 buffer 上，结果未定义

练习代码里写过这种调用：

```c
memset(buf, 0x55, sizeof(buf));  /* buf 里没有任何 '\0' */
write(fd, buf, strlen(buf));     /* strlen 会读到下一个 NUL，长度未定义 */
```

`strlen()` 只在遇到 `'\0'` 时停下，buffer 里全是 `0x55` 没有终止符时，它会越过 `sizeof(buf)` 继续往下读，**最终写入字节数不可预期**，严重的会踩栈、写花数据。想写满整个 buffer 用 `sizeof(buf)`，不要用 `strlen()`。

#### 4. O_APPEND 的真正价值：并发原子追加

`O_APPEND` 的真正用处是**多进程并发追加到同一文件**。在 POSIX 要求下，`write()` + `O_APPEND` 在内核里是原子定位 + 写入，不会出现“被另一个进程的写插队”的乱序。如果不用 `O_APPEND`，两个进程同时 `lseek(0, SEEK_END) + write`，大概率出现字节级别交错。

等讲到“竞争冒险”时再展开 `pwrite()` 与 `O_APPEND` 的等价性讨论。

---

## 复制文件描述符

`dup` / `dup2` / `dup3`：复制一个已有的 fd，返回新的 fd。它们和 `fcntl(F_DUPFD, ...)` 的关系，以及 `dup2` 的原子性差别这一章收尾会用到，放到 `fcntl` 那节统一整理。

---

## 文件共享

Linux 内核对进程打开的文件有三层结构：

- 进程 task_struct → 文件描述符表（fd 数组）
- fd 表项 → `struct file *` 指针
- `struct file` → 文件偏移、状态标志、inode 等

多个 fd（不管是同一进程内的 `dup`，还是 fork 出来的子进程）只要指向同一个 `struct file`，就共享偏移、共享 `O_APPEND` 之外的状态标志。

---

## 原子操作与竞争冒险

`O_APPEND` 是 Linux/POSIX 上少数几个“原子 + 文件偏移”打包的接口。其他位置要保持原子，得用 `pread()` / `pwrite()`（带偏移的 IO，不动当前偏移）。

---

## 系统调用 fcntl() / ioctl()

`fcntl()` 一个 fd 一个句柄，能改：

- 状态标志（`F_GETFL` / `F_SETFL`，常用 `O_NONBLOCK`）
- 复制 fd（`F_DUPFD`）
- 锁（`F_GETLK` / `F_SETLK`，留给文件锁那一章）
- 各种扩展 cmd

`ioctl()` 是“设备驱动杂项出口”，磁盘 IO 一般用不到，tty、网络设备上常见。

---

## 截断文件

`truncate(path, len)` / `ftruncate(fd, len)`：把文件长度强行截到 `len`，多余的字节被丢弃。和 `O_TRUNC` 的区别是，它们**只改长度、不打开文件**，常用于“保留 inode 元数据但清空内容”的场景。

`len` 超过当前文件大小时，会在尾部补零产生空洞（关联到前面的稀疏文件一节）。