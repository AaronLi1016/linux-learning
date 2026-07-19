#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "fcntl.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "stdlib.h"

int main(void)
{
    // int fd;
    // fd = open("./error_test_file",O_RDONLY);
    // if(fd == -1){
    //     printf("testing!!! open error: %s\n",strerror(errno));
    //     return -1;
    // }

    int fd;
    fd = open("./error_test_file",O_RDONLY);
    if(fd == -1){
        perror("perror testing");
        return -1;
        _exit(-1);
    }

    close(fd);
    exit(0);
    return 0;
}