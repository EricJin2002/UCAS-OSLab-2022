#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(){
    sys_move_cursor(0,0);
    int fd = sys_fopen("./2.bat", O_RDWR);

    // sys_fwrite(fd, "recv\n", 5);
    sys_fwrite(fd, "test 1.log 10\n", 14);
    sys_fwrite(fd, "test 2.log 20\n", 14);
    sys_fwrite(fd, "test 3.log 30\n", 14);
    sys_fwrite(fd, "test 4.log 40\n", 14);
    sys_fwrite(fd, "test 5.log 50\n", 14);

    sys_fclose(fd);
}