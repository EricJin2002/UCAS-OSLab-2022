# Project 3 进程管理、通信与多核执行

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

## 设计细节

### 部分PART-Ⅰ设计细节（摘自 PART-Ⅰ REVIEW 时制作的PPT）

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

### 线程的创建、退出与回收

### 线程的创建（thread_create）

thread_create函数接收五个参数，其中：
- 第一个参数为一个tid指针，用于返回创建的新线程tid
- 第二个参数为新线程的函数入口地址
- 后三个参数用于向新线程传递函数调用参数

在内核态，线程的创建包含以下操作：
- 为新线程分配内核栈与用户栈空间
- 在新分配的内核栈上创建tcb
- 为新线程分配tid
- 初始化tcb
- 初始化内核栈
- 将新线程加入ready队列以待调度

其中，这里的tcb复用了pcb的逻辑，并在原来的pcb结构体中增添了以下内容：
- tid，该项用于记录线程的tid；对于进程而言，此项为0（子线程的pid与父进程的pid一致）
- tcb_list，该项用于连接pid相同的所有线程（进程）
- retval，该项用于存储线程的返回值

在为新线程分配tid时，将遍历tcb_list，以避免同一tid已经被分配给拥有相同pid的线程。

### 线程的退出（thread_exit）

thread_exit函数接收一个参数，为线程的返回值。

在内核态，线程的退出包含以下操作：
- 将线程的状态设置为TASK_EXITED
- 在tcb中记录线程的返回值，以待回收
- 运行新调度

### 线程的回收（thread_join）

thread_join函数接收两个参数，其中：
- 第一个参数为待回收的线程tid
- 第二个参数为该线程退出时的返回值

若成功回收线程，thread_join函数将返回0；反之，将返回-1。

在内核态，线程的回收包含以下操作：
- 等待线程退出
- 传递线程退出时的返回值
- 将线程从对应的tcb_list中删去，以避免重复回收同一线程

## 实验过程

### shell的实现

#### 命令历史记录

#### 屏幕自动滚动

### 重新使用自旋锁实现互斥锁

在Project 2中，我曾实现了基于阻塞队列的简单互斥锁。这个锁足以应对Project 2的所有测试文件，然而在Project 3的多核任务中将会出错。这是因为，我先前实现的互斥锁仅能保证同时最多执行一个任务时不会出错。

在单核下，由于进入内核态后默认处于关中断状态，`acquire`操作和`release`操作的原子性自然得到了保证。然而在多核下，`acquire`操作和`release`操作需要对其他核也是原子的，因此无法简单地通过控制中断来实现。

我其实很早就意识到了这个问题。之所以迟迟没有纠正，是因为“阻塞队列的入队与出队逻辑”无法通过简单的原子指令和“锁的占用判断逻辑”一并打包成原子操作。我也曾被这个问题困扰了几周（理论课上PPT上利用原子指令实现无忙等待锁的部分感觉也存在问题），直到受到了xv6代码的启发。

xv6中的sleeplock近似实现了互斥锁。它是借助自旋锁来实现多核间同步的。这启发了我可以利用自旋锁来保护`init`，`acquire`和`release`操作，从而实现其原子性。

在实现进程结束后锁的回收时，我还遇到了一个困扰我很久的问题。

我创建了三个占用同把锁的进程（进程号分别为5,6,10），并尝试`kill`其中正占用锁的一个进程（进程号为10）。理论上，这个进程需要将锁释放并唤醒一个正阻塞的进程；但实际上，从下述调试信息中我们可以发现，进程10并未释放锁。

```
...
mlock_idx 0 owner changed from 0 to 10
mlock_idx 0 owner changed from 10 to 0
unblock 5 due to lock release
mlock_idx 0 owner changed from 0 to 5
mlock_idx 0 owner changed from 5 to 0
unblock 6 due to lock release
mlock_idx 0 owner changed from 0 to 6
mlock_idx 0 owner changed from 6 to 0
unblock 10 due to lock release
mlock_idx 0 owner changed from 0 to 10
mlock_idx 0 owner changed from 10 to 0
unblock 5 due to lock release
mlock_idx 0 owner changed from 0 to 5
mlock_idx 0 owner changed from 5 to 0
unblock 6 due to lock release
mlock_idx 0 owner changed from 0 to 6
mlock_idx 0 owner changed from 6 to 0
unblock 10 due to lock release
```

经过分析后，我发现进程10根本没有拿到锁！这是因为，在进程6释放锁并唤醒进程10后，进程10还未被调度，就被我通过shell`kill`掉了。从而导致进程5始终处于锁的阻塞队列中，无法被唤醒。由于在我的实现中，进程间传递锁时，锁的状态始终为LOCKED；因此，进程6也无法再次拿到锁，从而也被阻塞在锁的等待队列中。

这里先给出一版解决方案，也是一般的解决方案。该版解决方案有以下两个要点：
- 每当进程释放锁时，它将修改锁的状态为UNLOCKED，直到有进程再次占用。
- 每当进程释放锁时，它将尝试唤醒所有正处于同一个锁中阻塞队列内的进程。

第一点是为了防止唤醒阻塞进程时，原阻塞的进程一被唤醒就被`kill`，从而锁的状态时钟处于LOCKED。

第二点是为了防止阻塞队列队首进程被`kill`后，该队列其他进程始终不被唤醒。

但这样会带来不同进程对锁的竞争，无法保证“先到先得”。在最终实现中，我延续了先前“传递锁”而非“归还锁再借用锁”的逻辑。我将锁主人的更换挪到了原主人释放锁的时刻，从而彻底避免了锁在传递过程中丢失的情况。