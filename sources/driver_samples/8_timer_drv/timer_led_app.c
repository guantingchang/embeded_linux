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
#include <sys/ioctl.h>

#define LED_ON 1
#define LED_OFF 0
#define CLOSE_CMD (_IO(0xEF, 0x1))
#define OPEN_CMD (_IO(0xEF, 0x2))
#define SETPERIOD_CMD (_IO(0xEF, 0x3))

static char usrdata[] = {"usr data!"};

int main(int argc, char *argv[])
{
    int fd, ret;
    int cmd_err = 0;
    char *file_name = NULL;
    unsigned int cmd;
    unsigned int arg;
    unsigned char str[100];

    unsigned char data_buf[1];


    char readbuf[100], writebuf[100];

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
        printf("Input cmd:");
        ret = scanf("%d", &cmd);
        if(ret !=1){
            fgets(str, sizeof(str), stdin);
        }
        switch(cmd){
            case 1:
                cmd = CLOSE_CMD;
                break;
            case 2:
                cmd = OPEN_CMD;
                break;
            case 3:
                cmd = SETPERIOD_CMD;
                printf("Input timer period:");
                ret = scanf("%d", &arg);
                if(ret !=1){
                    fgets(str, sizeof(str), stdin);
                }
                break;
            default:
               cmd_err = 1;
                break;
            
        }
        if(cmd_err == 0){
            ioctl(fd, cmd, arg);
        }else{
            break;
        }
 
    }
    close(fd);
    return 0;
}