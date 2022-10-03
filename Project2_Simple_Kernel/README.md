# Project 2 简易内核实现

## 实验任务

本实验包括五个任务，其中前两个为S-core，前四个为A-core，前五个为C-core.

前两个为PART-Ⅰ内容，后三个为PART-Ⅱ内容。

具体如下：

- 任务1：任务启动与非抢占式调度（C,A,S-core）
- 任务2：互斥锁的实现（C,A,S-core）
- 任务3：系统调用（C,A-core）
- 任务4：时钟中断、抢占式调度（C,A-core）
- 任务5：实现线程的创建 thread_create（C-core）

## 运行说明

```sh
make all        # 编译，制作镜像
make run        # 启动QEMU，模拟运行
# 进一步上板测试
make floppy     # 将镜像写入SD卡
make minicom    # 监视串口
```

修改```init/main.c```中```init_pcb```函数内的```needed_task_name```变量来选择```test/test_project2/```目录下待执行的应用程序。

进入BBL界面后，输入```loadbootd```命令来启动bootloader。运行时的界面如下：

```
> [TASK] This task is to test scheduler. (35)                                   
> [TASK] This task is to test scheduler. (33)                                   
> [TASK] Applying for a lock.                                                   
> [TASK] Has acquired lock and running.(4)                                      
> [TASK] This task is to test sleep. (1)                                        
> [TASK] This is a thread to timing! (5/58454045 seconds).                      
                                                                                
                                                                                
                                                                                
                                                                                
                     _                                                          
                   -=\`\                                                        
               |\ ____\_\__                                                     
             -=\`""""""" "`)                                                    
               `~~~~~/ /~~`                                                     
                 -==/ /                                                         
                   '-'                                           
```

## 运作流程

### 初始化内核

内核的初始化包括以下操作：

- `init_jmptab`用于将kernel与bios提供的函数记录在跳转表中
- `init_task_info`用于保存loadboot时预存的task-info信息
- `init_pcb`具体包括下述功能：
  - 加载用户程序到内存
  - 为用户程序分配用户栈与内核栈空间
  - 修改栈内数据及栈顶指针来模拟“刚受到中断、将从`switch_to`返回”
  - 在PCB中记录用户程序的pid，栈指针等信息
  - 将用户程序的状态标记为ready，并添加到ready队列
  - 令第零号PCB为`pid0_pcb`，并设置`current_running`
  - 设置`tp`寄存器的值为`current_running`（这是为了当发生第一次抢占时，`SAVE_CONTEXT`能找到PCB地址）
- `bios_read_fdt`用于读取CPU频率，设置`time_base`以供`kernel/sched/time.c`使用
- `init_locks`用于将所有锁标记为未使用
- `init_exception`用于初始化例外表（及中断表），并设置中断处理入口地址`stvec`，打开中断
- `init_syscall`用于初始化系统调用表
- `init_screen`用于初始化屏幕
- `bios_set_timer`，`enable_interrupt`，`enable_preempt`等用于开启抢占式调度

### 例外的触发与处理

当发生例外时（例如当触发时钟中断时），内核依次完成下述操作：
- `ecall`指令进入内核态
- 进入`exception_handler_entry`
- 进入`SAVE_CONTEXT`，保存上下文到内核栈，并令`sp`指向内核栈
- 进入`interrupt_helper`，根据`scause`调用相应例外处理函数

对于例外，目前仅支持处理系统调用与时钟中断：
- 对于系统调用，将进入`handle_syscall`，根据`a7`寄存器内存储的值调用系统调用表内函数，随后令`sepc`加4
- 对于时钟中断，将进入`handle_irq_timer`，更新抢占时刻，并进行下一次调度

### 任务的调度与切换

当例外类型为时钟中断或系统调用`yield`函数时，将进入`do_scheduler`进行下一次调度：

- `check_sleeping`用于从sleep队列中唤醒正处于blocked状态的任务（修改其状态为ready，将其添加至ready队列）
- 若当前任务尚处于running状态，同样修改其状态为ready，将其添加至ready队列
- 让ready队列中处于队头的进程出队
- 调用`switch_to`，完成进程间的切换

### 例外的返回

- 从`interrupt_helper`返回，进入`ret_from_exception`
- 进入`RESTORE_CONTEXT`，恢复先前保存的上下文变量，并令`sp`指向用户栈
- `sret`返回用户态

## 部分PART-Ⅰ设计细节（摘自 PART-Ⅰ REVIEW 时制作的PPT）

- When a process is blocked, how does the kernel handle the blocked process?
- When someone acquire a lock . . .
  - If the lock LOCKED, 
    - Do_block
      - Push pcb_node to block_queue & Set its status to TASK_BLOCKED
      - Do_scheduler
  - Else if the lock UNLOCKED, 
    - Set lock status to LOCKED
- When someone release a lock . . . 
  - If block_queue not empty,
    - Pop a pcb_node from block_queue
    - Do_unblock
      - Push pcb_node to ready_queue & Set its status to TASK_READY
  - Else if block_queue empty,
    - Set lock status to UNLOCKED
- Where to place the PCB when the process is blocked/unblocked?
  - Place those blocked PCBs in the block_queue of mutex locks
  - Place those unblocked PCBs in the ready_queue or current_running

## 实验过程

### 区分内核栈与用户栈

在p2-task1 & p2-task2最初的实现中，我将```switch_to```上下文变量中的```sp```（亦即```init/main.c```中```init_pcb_stack```函数内的```pt_switchto->regs[1]```变量）理解成了用户栈栈顶指针。因此，每次切换应用程序时，保存与恢复的栈指针也均为用户栈指针。这样能正确地实现功能。然而，```switch_to```函数的调用发生在内核态，原本应为内核态的操作使用了用户态的栈——这显然是不太合理的。

PART-Ⅰ REVIEW 时，在助教的帮助下，我意识到start code中```switch_to```函数内原先被我注释的第一行是发生在内核栈上的。

```
addi sp, sp, -(SWITCH_TO_SIZE)
```

我进而修改了内核栈空间的初始化方式来模拟应用切换时的上下文逻辑。这样的设计能实现和先前一样的逻辑，且保证内核态操作能运行在内核栈上。（但值得注意的是，此时用户态操作也运行在了内核栈上）

内核态与用户态在p2-task3中得到彻底区分。这是通过系统调用来实现的，内核栈指针与用户栈指针在```SAVE_CONTEXT```与```RESTORE_CONTEXT```的过程中发生切换。

### 掌握新的调试工具使用技巧

在调试系统调用功能时，我发现gdb的```si```命令在遇到```sret```指令时，无法随程序进入用户态。这为调试带来了极大的麻烦，我的实验进程也因此停滞很久。在此过程中，我曾尝试使用```printl```来打印内核态信息，使用```while(1)```在用户态阻塞程序运行，来推算程序运行状况——但是效果均不是很好。

我也曾和同学讨论更加方便的调试方法，并尝试配置了```.vscode/*```调试环境。最终，在同学的分享下，我学会了用gdb的```b * 0x...```命令为用户态程序设置断点。结合汇编文件，可以掌握程序在用户态的行为，进而彻底解决了调试难题。在此表示感谢。