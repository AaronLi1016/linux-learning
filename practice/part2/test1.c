#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main(void)
{
    const char *path = "./test1.txt";
    const char *msg  = "Hello, Linux file IO!\n";

    // 1. 打开文件
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);//打开文件,如果不存在则创建
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;//打开失败返回
    }

    // 2. 写入数据
    ssize_t nw = write(fd, msg, strlen(msg));//写入数据
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
