/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>

#define SHELL_BEGIN 20

#define TASK_NAME_MAXLEN    50
#define HISTORY_SIZE        10

int main(void)
{
    sys_move_cursor(0, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");

    char history[HISTORY_SIZE][TASK_NAME_MAXLEN+1];
    int hist_curr=0;
    int hist_head=0;
    memset(history, 0, sizeof(history));

    while (1)
    {
        printf("> root@UCAS_OS: ");
        // printf("%d ",hist_curr);

        // TODO [P3-task1]: call syscall to read UART port
        char cache[TASK_NAME_MAXLEN+1];
        int tail=0;
        int ch;
        while((ch=sys_getchar())){if(ch!=-1){
            if(ch==27){
                //printf("111");
                while((ch=sys_getchar())){if(ch!=-1){
                    if(ch=='['){
                        //printf("222");
                        while((ch=sys_getchar())){if(ch!=-1){
                            //printf("333");
                            if(ch=='A'){
                                hist_head--;
                                hist_head=(hist_head+HISTORY_SIZE)%HISTORY_SIZE;
                            }else if(ch=='B'){
                                hist_head++;
                                hist_head%=HISTORY_SIZE;
                            }else{
                                break;
                            }
                            while(tail){
                                sys_backspace();
                                tail--;
                            }
                            // printf("%d\n",hist_head);
                            assert(tail==0);
                            for(;history[hist_head][tail];tail++){
                                cache[tail]=history[hist_head][tail];
                                printf("%c",history[hist_head][tail]);
                            }
                            break;
                        }}
                    }
                    break;  
                }}
                continue;
            }

            if(ch=='\r'){// \r for Carriage Return and \n for Line Feed
                printf("\n");
                cache[tail]='\0';
                break;
            }if(ch==127||ch=='\b'){
                if(tail){
                    sys_backspace();
                    tail--;
                }
            }else{
                if(tail<TASK_NAME_MAXLEN){
                    printf("%c",ch);
                    cache[tail++]=ch;
                }else{
                    printf("\nError: Maximal input reached!\n> root@UCAS_OS: ");
                    tail=0;
                }
            }
        }}

        strcpy(history[hist_curr], cache);
        hist_curr++;
        hist_curr%=HISTORY_SIZE;
        hist_head=hist_curr;

        // print input
        // printf("%s\n",cache);

        // TODO [P3-task1]: parse input
        // note: backspace maybe 8('\b') or 127(delete)
        int argc=0;
        char *argv[TASK_NAME_MAXLEN+1];
        int parsing=0;
        int err=0;
        for(int head=0;head<tail;head++){
            if(isspace(cache[head])){
                parsing=0;
                cache[head]='\0';
                continue;
            }
            
            // if(isalnum(cache[head])){
                if(!parsing){
                    argv[argc++]=cache+head;
                    parsing=1;
                }
            // }else{
            //     printf("Error: Unknown symbol %c!\n",cache[head]);
            //     err=1;
            //     break;
            // }
        }
        if(err){
            continue;
        }
        argv[argc]=(char *)0;

        // print parsed input
        // printf("argc: %d \nargv: ",argc);
        // for(int i=0;argv[i];i++){
        //     printf("%s ",argv[i]);
        // }
        // printf("\n");

        // TODO [P3-task1]: ps, exec, kill, clear        
        if(!argc){
            continue;
        }

        int wait_end = 1;
        if(!strcmp(argv[argc-1],"&")){
            wait_end = 0;
        }

        if(!strcmp(argv[0],"exec")){
#ifdef S_CORE
            pid_t pid=sys_exec(atol(argv[1]), argc-2, atol(argv[2]), atol(argv[3]), atol(argv[4]));
#else
            pid_t pid=sys_exec(argv[1], argc-1, argv+1);
#endif

            if(!pid){
                printf("Error: No such APP! (Perhaps you've run a BAT)\n");
                err=1;
            }else{
                printf("Info: Process pid %d is launched.\n", pid);
            }

            if(!err && wait_end){
                sys_waitpid(pid);
            }
        }else if(!strcmp(argv[0],"exit")){
            return 0;
        }else if(!strcmp(argv[0],"kill")){
            if(!sys_kill(atol(argv[1]))){
                printf("Error: No such pid!\n");
                err=1;
            }else{
                printf("Info: Process pid %d is killed.\n", atol(argv[1]));
            }
        }else if(!strcmp(argv[0],"waitpid")){
            if(!sys_waitpid(atol(argv[1]))){
                printf("Error: No such pid!\n");
                err=1;
            }
        }else if(!strcmp(argv[0],"ps")){
            sys_ps();
        }else if(!strcmp(argv[0],"getpid")){
            printf("%d\n", sys_getpid());
        }else if(!strcmp(argv[0],"clear")){
            sys_clear();
            sys_move_cursor(0, SHELL_BEGIN);
            printf("------------------- COMMAND -------------------\n");
        }else if(!strcmp(argv[0],"taskinfo")){
            sys_show_task();
        }else if(!strcmp(argv[0],"history")){
            printf("[History Table]\n");
            printf("IDX COMMAND\n");
            for(int i=0;i<HISTORY_SIZE;i++){
                printf("[%d] %s\n",i,history[i]);
            }
        }else{
            printf("Error: Unknown command %s!\n",argv[0]);
            err=1;
        }
    }

    return 0;
}
