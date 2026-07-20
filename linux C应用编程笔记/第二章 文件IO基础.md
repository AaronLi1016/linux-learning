# 文件IO基础

Linux 应用编程中最基础的操作是文件 IO（输入/输出）。Linux 的核心理念是"一切皆文件"——普通文件、设备、管道、socket 等都以文件的形式暴露给用户态程序，通过统一的文件 IO 接口进行操作。

通用的文件 IO 模型包含四个基本操作：`open`、`read`、`write`、`close`，以及用于移动读写位置的 `lseek`。

## 1. 文件描述符

文件描述符是内核为每个进程维护的非负整数句柄，应用程序通过它来引用已打开的文件。进程启动时，内核默认分配三个文件描述符：

| 文件描述符 | 宏常量 | 用途 |
|:---:|:---|:---|
| 0 | `STDIN_FILENO` | 标准输入 |
| 1 | `STDOUT_FILENO` | 标准输出 |
| 2 | `STDERR_FILENO` | 标准错误 |

这三个描述符通常指向终端。当调用 `open` 创建新的文件描述符时，内核会分配当前可用的最小非负整数，因此新打开的文件描述符通常从 3 开始。

每个进程能同时打开的文件描述符数量有上限，可通过 `ulimit -n` 查看。`close` 会释放文件描述符，使其可被后续的 `open` 复用。进程退出时，内核会自动关闭该进程所有打开的文件描述符。

## 2. open —— 打开文件

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);
```

参数说明：

- `pathname`：文件路径，可以是相对路径或绝对路径。若路径为符号链接，`open` 默认会跟随链接打开目标文件；若指定 `O_NOFOLLOW` 且路径为符号链接，则打开失败。
- `flags`：打开方式与行为控制，必须包含以下**访问模式之一**，并可搭配其他可选标志（通过按位或组合）：

**必选访问模式：**

| 标志 | 含义 |
|:---|:---|
| `O_RDONLY` | 只读打开 |
| `O_WRONLY` | 只写打开 |
| `O_RDWR` | 读写打开 |

**常用可选标志：**

| 标志 | 含义 |
|:---|:---|
| `O_CREAT` | 若文件不存在则创建，需同时提供 `mode` 参数 |
| `O_EXCL` | 与 `O_CREAT` 配合使用，若文件已存在则返回错误，常用于原子性创建锁文件 |
| `O_TRUNC` | 若文件存在且为普通文件，将长度截断为 0 |
| `O_APPEND` | 每次写入前将文件偏移量移至文件末尾，保证追加写入 |
| `O_NONBLOCK` | 以非阻塞方式打开，适用于 FIFO、设备等 |
| `O_SYNC` | 每次 write 等待数据及元数据真正落盘后返回 |

- `mode`：仅当 `flags` 包含 `O_CREAT` 时需要，指定新文件的权限位，如 `0644`。实际生效权限还会受进程 `umask` 影响：最终权限 = `mode & ~umask`。

返回值：

- 成功时返回一个新的文件描述符（非负整数）。
- 失败时返回 `-1`，并设置 `errno` 表示错误原因。

示例：

```c
int fd = open("./test.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
if (fd == -1) {
    perror("open");
    return 1;
}
```

## 3. read —— 读取数据

```c
#include <unistd.h>

ssize_t read(int fd, void *buf, size_t count);
```

参数说明：

- `fd`：已打开的文件描述符。
- `buf`：存放读取数据的缓冲区指针。
- `count`：期望读取的最大字节数。

返回值（`ssize_t` 为有符号类型）：

| 返回值 | 含义 |
|:---|:---|
| `> 0` | 实际读取到的字节数 |
| `0` | 已到达文件末尾（EOF） |
| `-1` | 读取失败，`errno` 被设置 |

关键行为：

- `read` 从文件当前偏移量开始读取，读取完成后偏移量自动增加实际读取的字节数。
- **返回值可能小于 `count`**：即使文件尚未到达末尾，也可能返回少于请求的字节数（例如从管道或网络 socket 读取）。读取普通文件时通常能一次返回请求的数量，但编写健壮代码时应始终用返回值判断实际读取量。

以下是一个简单的 `read` 示例：

```c
char buf[128];
ssize_t n = read(fd, buf, sizeof(buf) - 1);
if (n == -1) {
    perror("read");
    return 1;
}
buf[n] = '\0';
printf("read %zd bytes: %s\n", n, buf);
```

## 4. write —— 写入数据

```c
#include <unistd.h>

ssize_t write(int fd, const void *buf, size_t count);
```

参数说明：

- `fd`：已打开的文件描述符。
- `buf`：待写入数据的缓冲区指针。
- `count`：期望写入的字节数。

返回值：

| 返回值 | 含义 |
|:---|:---|
| `> 0` | 实际写入的字节数 |
| `0` | 未写入任何数据（极少发生，通常视为异常） |
| `-1` | 写入失败，`errno` 被设置 |

关键行为：

- `write` 从文件当前偏移量开始写入，写入完成后偏移量增加实际写入的字节数。
- **返回值可能小于 `count`**：例如磁盘空间不足、信号中断等原因可能导致部分写入。健壮代码应检查返回值，必要时循环写入剩余数据。

循环写入示例：

```c
ssize_t write_all(int fd, const void *buf, size_t count)
{
    const char *p = buf;
    size_t remaining = count;

    while (remaining > 0) {
        ssize_t n = write(fd, p, remaining);
        if (n == -1) {
            if (errno == EINTR)
                continue;   // 被信号中断，重试
            return -1;
        }
        p += n;
        remaining -= n;
    }
    return count;
}
```

## 5. close —— 关闭文件

```c
#include <unistd.h>

int close(int fd);
```

参数说明：

- `fd`：需要关闭的文件描述符。

返回值：

- 成功返回 `0`。
- 失败返回 `-1`，并设置 `errno`。

关键行为：

- 关闭后，该文件描述符被释放，可被后续 `open` 复用。
- 关闭文件时，内核会将缓冲区中尚未写入的数据刷入磁盘，并释放与该描述符关联的内核资源（如文件表项、i-node 引用等）。
- 错误地 `close` 之后继续使用该描述符（如 `read`/`write`）会导致 `EBADF` 错误。
- 进程结束时，内核会自动关闭该进程所有打开的文件描述符，但仍建议显式调用 `close` 以尽早释放资源。

示例：

```c
if (close(fd) == -1) {
    perror("close");
}
```

## 6. lseek —— 移动文件偏移量

```c
#include <sys/types.h>
#include <unistd.h>

off_t lseek(int fd, off_t offset, int whence);
```

参数说明：

- `fd`：已打开的文件描述符。
- `offset`：相对偏移量，可以为负数（取决于 `whence`）。
- `whence`：基准位置，取以下三个宏之一：

| 宏 | 含义 |
|:---|:---|
| `SEEK_SET` | 从文件开头算起，`offset` 成为新的绝对偏移量 |
| `SEEK_CUR` | 从当前偏移量算起，新偏移量 = 当前偏移量 + `offset` |
| `SEEK_END` | 从文件末尾算起，新偏移量 = 文件大小 + `offset` |

返回值：

- 成功时返回新的文件偏移量（从文件开头计算的字节数）。
- 失败时返回 `-1`，并设置 `errno`。

常见用法：

```c
// 获取当前偏移量
off_t cur = lseek(fd, 0, SEEK_CUR);

// 获取文件大小
off_t size = lseek(fd, 0, SEEK_END);

// 移动到文件开头
lseek(fd, 0, SEEK_SET);

// 从当前位置向后移动 10 字节
lseek(fd, 10, SEEK_CUR);
```

`lseek` 仅修改内核中记录的偏移量，不产生实际的 IO 操作。它不适用于管道、socket 等不可寻址的文件类型，对这类文件调用 `lseek` 会返回 `ESPIPE` 错误。

## 7. 完整示例

以下程序打开（或创建）一个文件，写入一行文本，再将文件偏移量移回开头并读取打印：

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main(void)
{
    const char *path = "./demo.txt";
    const char *msg  = "Hello, Linux file IO!\n";

    // 1. 打开文件
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    // 2. 写入数据
    ssize_t nw = write(fd, msg, strlen(msg));
    if (nw == -1) {
        perror("write");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("wrote %zd bytes\n", nw);

    // 3. 移动偏移量到文件开头
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        return EXIT_FAILURE;
    }

    // 4. 读取数据
    char buf[128];
    ssize_t nr = read(fd, buf, sizeof(buf) - 1);
    if (nr == -1) {
        perror("read");
        close(fd);
        return EXIT_FAILURE;
    }
    buf[nr] = '\0';
    printf("read  %zd bytes: %s", nr, buf);

    // 5. 关闭文件
    if (close(fd) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

编译并运行：

```bash
gcc -Wall -o demo demo.c
./demo
```

预期输出：

```
wrote 22 bytes
read  22 bytes: Hello, Linux file IO!
```

## 8. 要点总结

- Linux 通过文件描述符统一管理文件和设备 IO，`0`/`1`/`2` 分别对应标准输入、输出和错误。
- `open` 的 `flags` 参数控制访问模式和行为，`O_CREAT` 时需提供 `mode` 创建权限。
- `read` / `write` 返回值可能小于请求的字节数，应检查返回值并循环处理以确保完整性。
- `close` 释放文件描述符和内核资源，进程退出时内核也会自动关闭，但显式调用是良好实践。
- `lseek` 用于移动文件偏移量，不产生实际 IO，不适用于管道和 socket。
- 每个系统调用失败时返回 `-1` 并设置 `errno`，使用 `perror` 或 `strerror` 可输出错误信息。
- 写代码的时候不知道需要包含哪个头文件的时候可以`man`就可以查看需要的函数,例如`man 2 open`就能看到
``` txt
       #include <fcntl.h>
       int open(const char *pathname, int flags);
```