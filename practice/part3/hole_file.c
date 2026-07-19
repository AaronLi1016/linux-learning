#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"
#include "fcntl.h"  //这个是文件控制的头文件，里面定义了很多文件操作的函数和宏
#include "sys/types.h"
#include "sys/stat.h"

int main(int argc, char *argv[]){
    int fd,ret;
    char buf[1024];
    int i;
    
    fd = open("./hole_file",O_WRONLY|O_CREAT|O_EXCL,
            S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(-1 == fd){
        perror("open");
        exit(1);
    }

    ret = lseek(fd,4096,SEEK_SET);
    if(-1 == ret){
        perror("lseek");
        exit(1);
    }

    memset(buf,0xff,sizeof(buf));

    for(i=0;i<4;i++){
        ret = write(fd,buf,sizeof(buf));
        if(-1 == ret){
            perror("write");
            goto err;
        }
    }
    close(fd);
    ret = 0;
    return 0;
err:
    close(fd);
    exit(ret);
}