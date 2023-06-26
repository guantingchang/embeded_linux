/*
int open(const char *pathname, int flags);
ssize_t read(int fd, void*buf, size_t count);
ssize_t write(int fd,const void*buf, size_t count);
int close(int fd);
*/
#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <signal.h>

#define KEY0_VALUE    0xF0
#define INVALID_KEY      0x00

static char usrdata[] = {"usr data!"};


static int fd;

static void sigio_cb(int sig_num)
{
    unsigned int key_value = 0;

    read(fd, &key_value, sizeof(key_value));
    if(key_value == 0){
        printf("key press\n");
    }else if(key_value == 1){
        printf("key release\n");
    }
}

int main(int argc, char *argv[])
{
    int fd, ret;
    fd_set read_fds;
    int flags;

    char *file_name = NULL;
    int key_value;

    if (argc != 2) {
        printf("Usage:\n" "\t./key_irq_noblock_app /dev/key\n");
        return -1;
    }

    file_name = argv[1];
    fd = open(file_name, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        printf("Can't open file %s\r\n", file_name);
        return -1;
    }
    signal(SIGIO, sigio_cb);
    fcntl(fd, F_SETOWN, getpid());
    flags = fcntl(fd, F_GETFD);
    fcntl(fd, F_SETFL, flags | FASYNC);
    while(1){
        sleep(2);
    }

    ret = close(fd);
    if (ret < 0) {
        printf("close file %s failed!\r\n", file_name);
        return -1;
    }

    return 0;
}