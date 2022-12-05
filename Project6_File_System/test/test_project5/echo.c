#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

int main(){
    sys_exec("echo_send",0,0);
    sys_exec("echo_recv",0,0);
}