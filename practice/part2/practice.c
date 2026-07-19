#include "stdio.h"      // <stdio.h>   —— 标准输入输出
#include <stdlib.h>     // <stdlib.h>  —— 标准库工具
#include <unistd.h>     // <unistd.h>  —— Unix 标准函数
#include <fcntl.h>      // <fcntl.h>   —— 文件控制选项
#include <string.h>     // <string.h>  —— 字符串处理
#include <errno.h>      // <errno.h>   —— 错误码

int main(void)
{
    printf("Hello, Linux file IO!\n");

    const char *path = "./practice.txt";
    const char *msg  = "Hello, linux, today is 20260718\n";

    int fd = open (path, O_RDWR | O_CREAT | O_TRUNC, 0644);//这里的0644是文件权限，表示所有者有读写权限，组用户和其他用户只有读权限
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    int nw = write(fd, msg, strlen(msg));
    if (nw == -1) {
        perror("write");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("wrote %d bytes\n", nw);

    int ret = lseek(fd, 0, SEEK_SET);
    if (ret == -1) {
        perror("lseek");
        close(fd);
        return EXIT_FAILURE;
    }

    char buf[128];
    int nr = read(fd, buf, sizeof(buf) - 1);
    if (nr == -1) {
        perror("read");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("read %d bytes: %s", nr, buf);

    ret = close(fd);
    if (ret == -1) {
        perror("close");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}