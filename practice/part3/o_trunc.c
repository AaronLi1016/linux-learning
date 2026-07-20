#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
int main(int argc, char *argv[]){
    int fd;
    fd = open("test.txt", O_WRONLY | O_TRUNC);
    if(fd == -1){
        perror("open");
        exit(-1);
    }
    close(fd);
    exit(0);
    return 0;
}
