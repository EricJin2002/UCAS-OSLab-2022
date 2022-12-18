#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char buff[64];

int main(void)
{
    sys_move_cursor(0,0);

    int fd = sys_fopen("2.txt", O_RDWR);
    sys_fwrite(fd, "hello world!\n", 14);

    sys_lseek(fd, 1024, SEEK_SET);
    sys_fwrite(fd, "seek KB\n", 9);
    
    sys_lseek(fd, 1024*1024, SEEK_SET);
    sys_fwrite(fd, "seek MB\n", 9);

    sys_lseek(fd, 1024*1024*1024, SEEK_SET);
    sys_fwrite(fd, "seek GB\n", 9);
    
    // sys_lseek(fd, 1024*1024*1024*1024, SEEK_SET);
    // sys_fwrite(fd, "seek TB\n", 9);


    sys_lseek(fd, 0, SEEK_SET);
    sys_fread(fd, buff, 13);
    printf("%s", buff);
    
    sys_lseek(fd, 1024, SEEK_SET);
    sys_fread(fd, buff, 9);
    printf("%s", buff);
    
    sys_lseek(fd, 1024*1024, SEEK_SET);
    sys_fread(fd, buff, 9);
    printf("%s", buff);
    
    sys_lseek(fd, 1024*1024*1024, SEEK_SET);
    sys_fread(fd, buff, 9);
    printf("%s", buff);

    // sys_lseek(fd, 1024*1024*1024*1024, SEEK_SET);
    // sys_fread(fd, buff, 9);
    // printf("%s", buff);

    sys_fclose(fd);

    return 0;
}