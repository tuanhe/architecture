#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define DEV_PIFACE "/dev/piface0.0"
#define SYS_PI_FACE "/sys/bus/spi/devices/spi0.0/sys_piface"
#define BUF_SIZE        16

int main(int argc, char * argv[])
{
        int fd;
        int i ;
        int err ;
        char buff[BUF_SIZE];
        char *p = "hunanweishi13579";
        int len;
        fd = open(SYS_PI_FACE , O_RDWR);
        if(fd < 0){
                printf("open file %s error\n",DEV_PIFACE);
        }
        
        len = strlen(p);
        write(fd , p , len);
        printf("the write string is %s\n", p);
        printf("*****************************************************************\n", p);
        err = read(fd , buff , len );
        if(err<0){
                printf("error\n");
                return 1 ;
        }
        printf("the read value is %s\n", buff);
        close(fd);
        return 0;
}



