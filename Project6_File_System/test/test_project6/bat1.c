#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(){
    sys_move_cursor(0,0);
    int fd = sys_fopen("./1.bat", O_RDWR);

    sys_fwrite(fd, "fly &\n", 6);
    sys_fwrite(fd, "add2\n", 5);

    sys_fclose(fd);
}