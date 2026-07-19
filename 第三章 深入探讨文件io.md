# 深入探讨文件IO
主要是针对linux文件下函数返回的错误的处理
退出程序exit() _Exit() _exit()
空洞文件的概念
open的 O_APPEND 和 O_TRUNC 标志
复制文件描述符
文件共享介绍
原子操作和竞争冒险
系统调用fcntl() ioctl()
截断文件

### 返回错误处理和error
例如在open函数里面的man描述里面
``` txt
RETURN VALUE
       open(),  openat(),  and creat() return the new file descriptor (a nonnegative integer), or -1 if an
       error occurred (in which case, errno is set appropriately).
```

错误码是通过errno这个变量来传递的
- 我们可以通过sterror对错误码进行解析,直接输出开发者友好log
- perror(使用最多)

最后就是exit退出进程

### 空洞文件（hole file / sparse file）

空洞文件是指文件中存在未被写入的区域，这些区域不占用实际磁盘块，但读操作会返回全零。这种文件也叫稀疏文件（sparse file）。

#### 创建方式

使用 `lseek` 将文件偏移移动到超过文件末尾的位置，然后执行 `write`，中间的未写入区域自动成为空洞：

```c
fd = open("./hole_file", O_WRONLY | O_CREAT | O_EXCL,
          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

// 偏移到 4096，中间 0~4095 字节成为空洞
lseek(fd, 4096, SEEK_SET);

// 写入 4KB 数据，文件逻辑大小为 8KB，实际磁盘占用仅 4KB
write(fd, buf, 4096);
```

#### 如何观察空洞

| 命令 | 显示内容 |
|---|---|
| `ls -lh` | 文件逻辑大小（包含空洞） |
| `du -h` | 实际磁盘占用（空洞不占块） |

二者差值即为空洞大小。例如 `ls` 显示 8K、`du` 显示 4K，说明有 4K 的空洞。

#### 空洞文件存在的意义

**1. 节省磁盘空间**

这是最直接的价值。一个逻辑大小 100GB 的文件，如果大部分区域是空洞，实际磁盘占用可能只有几百 MB。

**2. 高效支持随机写入场景**

如果没有空洞机制，每次在文件末尾之前的位置写入数据时，文件系统必须将中间所有未写入区域填零，造成大量无意义的 IO 和磁盘占用。空洞文件让应用直接定位写入，不需要关心中间区域的初始化。

**3. 简化应用层语义**

开发者不需要手动管理"未写入区域读为零"的逻辑——文件系统天然保证空洞区域读操作返回 `\0`。

#### 典型应用场景

- **数据库文件**：SQLite、Berkeley DB 等按页管理数据，文件创建时预分配逻辑大小，按需写入各页，未写入的页为空洞。
- **虚拟机磁盘镜像**：qcow2、raw 等格式利用空洞（或类似机制）实现精简置备（thin provisioning），逻辑容量远大于物理占用。
- **核心转储文件（core dump）**：进程地址空间中未映射的区域在 core 文件中对应空洞。
- **BitTorrent 下载**：下载器预分配文件大小，按块下载写入，未完成的块保持空洞。

#### 注意事项

- **空洞的持久性取决于文件系统**：大多数 Unix/Linux 文件系统（ext4、XFS、btrfs）完整支持空洞文件；NTFS 本身支持稀疏文件，但 **WSL 的 DrvFs 挂载层不传递此能力**——在 `/mnt/d/` 等 Windows 盘路径下，空洞会被实际填零并分配磁盘块。如需验证，应将代码放在 WSL 原生 ext4 路径（如 `/home/`）编译运行。
- **备份工具**：部分备份软件（如 `tar`、`rsync`）默认会展开空洞（将空洞填零后复制），需要加对应选项（如 `tar --sparse`）才能保留空洞。
- **文件传输**：`scp`、`cp` 等操作文件时，可用 `cp --sparse=always` 保留空洞，或直接用 `rsync --sparse`。


