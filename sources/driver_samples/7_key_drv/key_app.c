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

#define KEY0_VALUE    0xF0
#define INVALID_KEY      0x00

static char usrdata[] = {"usr data!"};

int main(int argc, char *argv[])
{
    int fd, ret;

    char *file_name = NULL;
    int key_value;

    if (argc != 2) {
        printf("Error Usage!\r\n");
        return -1;
    }

    file_name = argv[1];
    fd = open(file_name, O_RDWR);
    if (fd < 0) {
        printf("Can't open file %s\r\n", file_name);
        return -1;
    }

    while(1){
        read(fd,&key_value, sizeof(key_value));
        if(key_value == KEY0_VALUE){
            printf("key0 press,value=%x\r\n",key_value);
        }
    }

    ret = close(fd);
    if (ret < 0) {
        printf("close file %s failed!\r\n", file_name);
        return -1;
    }

    return 0;
}