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
#include <linux/input.h>

#define KEY0_VALUE    0xF0
#define INVALID_KEY      0x00

static char usrdata[] = {"usr data!"};

int main(int argc, char *argv[])
{
    int fd, ret;
    struct input_event key_event;

    char *file_name = NULL;
    int key_value;

    if (argc != 2) {
        printf("Error Usage!\r\n");
        return -1;
    }

    file_name = argv[1];
    fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        printf("Can't open file %s\r\n", file_name);
        return -1;
    }

    while(1){
        ret = read(fd,&key_event, sizeof(key_event));
        if(ret != 0){
            switch(key_event.type){
                case EV_KEY:
                    if(KEY_0 == key_event.code){
                        if(key_event.value !=0){
                            printf("key0 press,value=%x\r\n",key_event.value);
                        }else{
                            printf("key0 release,value=%x\r\n",key_event.value);
                        }
                    }
                    break;
                case EV_REL:
                case EV_ABS:
                case EV_MSC:
                case EV_SW:
                default:
                    printf("key event type:%x \r\n",key_event.type);
                    break;
            }            
        }else {
            printf("Error:file %s read failed\r\n",argv[1]);
            goto out;
        }
    }
out:
    ret = close(fd);
    if (ret < 0) {
        printf("close file %s failed!\r\n", file_name);
        return -1;
    }

    return 0;
}