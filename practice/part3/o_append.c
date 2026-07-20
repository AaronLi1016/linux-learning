#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char *argv[]){
    int fd;
    char buffer[32];
    int ret;

    fd = open("append.txt", O_RDWR | O_APPEND | O_CREAT, 0644);
    if(fd == -1){
        perror("open");
        exit(-1);
    }

    memset(buffer, 0x55, sizeof(buffer));
    ret = write(fd, buffer, strlen(buffer));
    if(ret == -1){
        perror("write");
        goto err;
    }

    memset(buffer, 0x00, sizeof(buffer));//清空 buffer，准备读取刚写入的尾部数据
    //因为O_APPEND只保证每次write之前先把偏移设置到文件末尾，然后写入。它不会“锁定偏移”，lseek仍然按普通offset工作。
    ret = lseek(fd, -4, SEEK_END);
    if(ret == -1){
        perror("lseek");
        goto err;
    }

    ret = read(fd, buffer, 4);
    if(ret == -1){
        perror("read");
        goto err;
    }
    printf("0x%x 0x%x 0x%x 0x%x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
    ret = 0;

    close(fd);
    exit(0);

err:
    close(fd);
    exit(ret);
}
