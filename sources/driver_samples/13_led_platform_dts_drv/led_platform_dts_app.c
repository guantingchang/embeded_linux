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

#define LED_ON 1
#define LED_OFF 0

static char usrdata[] = {"usr data!"};

int main(int argc, char *argv[])
{
    int fd, ret;

    char *file_name = NULL;
    unsigned char data_buf[1];


    char readbuf[100], writebuf[100];

    if (argc != 3) {
        printf("Error Usage!\r\n");
        return -1;
    }

    file_name = argv[1];
    fd = open(file_name, O_RDWR);
    if (fd < 0) {
        printf("Can't open file %s\r\n", file_name);
        return -1;
    }
    data_buf[0] = atoi(argv[2]);

    ret = write(fd, data_buf, sizeof(data_buf));
    if(ret < 0){
        printf("led control failed!\r\n");
        close(fd);
        return -1;
    }

    ret = close(fd);
    if (ret < 0) {
        printf("close file %s failed!\r\n", file_name);
        return -1;
    }

    return 0;
}