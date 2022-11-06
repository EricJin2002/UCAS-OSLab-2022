# Project 3 进程管理、通信与多核执行

## 实验任务

本实验包括四个任务。

具体如下：

- 任务1：终端和终端命令的实现（C,A,S-core）
- 任务1续：exec、kill、exit、waitpid方法的实现（C,A,S-core）
- 任务2：实现同步原语：barriers、condition variables（C,A,S-core）
- 任务2续：进程间的通信——mailbox实现（C,A-core）
- 任务3：开启双核并行运行（C,A-core）
- 任务4：shell命令taskset——将进程绑定在指定的核上（C-core）

## 运行说明

```sh
make all        # 编译，制作镜像
make run        # 启动QEMU，模拟运行
make run-smp    # 启动QEMU，模拟双核运行
# 进一步上板测试
make floppy     # 将镜像写入SD卡
make minicom    # 监视串口
```

进入BBL界面后，输入`loadbootd`命令来启动单核。输入`loadbootm`命令来启动双核。

### `shell`界面
将进入如下所示的`shell`界面。
```
> [INIT] SCREEN initialization succeeded.                                       
                                                                                
                                                                                
                                                                                
                                                                                
Successfully aroused (sub core)! [core1]                                        
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
------------------- COMMAND -------------------                                 
> root@UCAS_OS:                                                                 
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                 
```

### `taskinfo`命令
输入`taskinfo`命令查看可执行的task列表。

```
------------------- COMMAND -------------------                                 
> root@UCAS_OS: taskinfo                                                        
[Task Table]                                                                    
 IDX TYPE LOADED NAME                                                           
[00]  APP LOADED shell                                                          
[01]  APP LOADED mbox_client                                                    
[02]  APP LOADED multicore                                                      
[03]  APP LOADED test_barrier                                                   
[04]  APP LOADED consumer                                                       
[05]  APP LOADED affinity                                                       
[06]  APP LOADED waitpid                                                        
[07]  APP LOADED ready_to_exit                                                  
[08]  APP LOADED mbox_server                                                    
[09]  APP LOADED condition                                                      
[10]  APP LOADED add                                                            
[11]  APP LOADED wait_locks                                                     
[12]  APP LOADED producer                                                       
[13]  APP LOADED barrier                                                        
[14]  APP LOADED test_affinity                                                  
[15]  BAT NOTYET p1_example                                                     
[16]  BAT NOTYET p1_cycle                                                       
[17]  BAT NOTYET p2                                                             
[18]  BAT NOTYET p3_mbox                                                        
[19]  BAT NOTYET p3                                                             
Note: due to the bug that sub core cannot read sd,                              
      there is possibility of malfunctioning if sub core calls BAT              
> root@UCAS_OS:                                 
```

由于QEMU上从核调用`sd_read`存在bug，因此在初始化时我将所有`APP`类型的task预加载到了内存。这也是上述TYPE为APP的task的LOADED项一进`shell`便显示为LOADED的原因。

### `APP`相关

`exec`，`kill`，`waitpid`，`exit`，`taskset`，`ps`等命令的使用方法与讲义一致，此处不再赘述。

### `BAT`相关

除此之外，我还更新了对BAT类task的支持。所有BAT文件以`.txt`结尾，位于`bat/`文件夹下。这里以`p3`为例（`p1`以及`p2`开头的`BAT`需要用到前几个实验`test`文件夹下内容，因此本实验不支持）。该文件（`bat/p3.txt`）内容如下所示：

```
waitpid 1 &
barrier 4 &
condition 7 &
p3_mbox
```

包含了三个`APP`类型的task，以及一个`BAT`类型的task（`p3_mbox`）。

（是的，`BAT`内甚至可以调用其他`BAT`！）

被调用的`p3_mbox.txt`文件内容如下所示：

```
mbox_server 11 &
mbox_client &
mbox_client &
mbox_client &
mbox_client &
```

这里每行末尾的`&`表示非阻塞地运行着一系列任务。如果想要某个`APP`（以及之后的所有`APP`）在前一个`APP`执行退出后才被唤醒执行，可以把对应的`&`去掉。

和`APP`一样，使用`exec`命令运行`BAT`。上述`BAT`文件的执行效果如下所示：

```
> [INIT] SCREEN initialization succeeded.                                       
> [TASK] I want to wait task (pid=12) to exit.                                  
> [TASK] I am task with pid 12, I have acquired two mutex locks. (3257)         
> [TASK] I want to acquire mutex lock1 (handle=0).                              
> [TASK] Exited barrier (7).(sleep 3 s)                                         
> [TASK] Exited barrier (7).(sleep 1 s)                                         
> [TASK] Exited barrier (7).(sleep 3 s)                                         
> [TASK] Total produced 21 products. (next in 3 seconds)                        
> [TASK] Total consumed 7 products. (Sleep 1 seconds)                           
> [TASK] Total consumed 7 products. (Sleep 1 seconds)                           
> [TASK] Total consumed 7 products. (Sleep 2 seconds)                           
[Server]: recved msg from 8 (blocked: 38, correctBytes: 234, errorBytes: 0)     
[Client] send bytes: 94, blocked: 9                                             
[Client] send bytes: 42, blocked: 3                                             
[Client] send bytes: 32, blocked: 2                                             
[Client] send bytes: 44, blocked: 3                                             
                                                                                
                                                                                
                                                                                
                                                                                
------------------- COMMAND -------------------                                 
> root@UCAS_OS: exec p3                                                         
                                                                                
===Reading batch from image...===                                               
waitpid 1 &                                                                     
barrier 4 &                                                                     
condition 7 &                                                                   
p3_mbox                                                                         
===Finish reading!===                                                           
                                                                                
                                                                                
===Now excute batch!===                                                         
                                                                                
===Reading batch from image...===                                               
mbox_server 11 &                                                                
mbox_client &                                                                   
mbox_client &                                                                   
mbox_client &                                                                   
mbox_client &                                                                   
===Finish reading!===                                                           
                                                                                
                                                                                
===Now excute batch!===                                                         
===All tasks in batch are excuted!===                                           
                                                                                
===All tasks in batch are excuted!===                                           
                                                                                
Error: Running a BAT / No such APP / No available PCB!                          
> root@UCAS_OS:                                                      
```

值得注意的是，因为`BAT`类task本身不会孵化自身为一个进程，因此`exec`函数的返回值为`0`。这也是为什么所有`BAT`类task最后一定会输出一条`Error`信息的原因。

### `history`命令

**shell支持使用上下键快速恢复历史命令。**

这通过在`test/hell.c`这个用户程序内增加历史记录数组来实现。目前支持存储最近十条命令，并循环更新覆盖旧历史记录。

如果想要存储更多的历史指令，可以修改`test/shell.c`文件内的`HISTORY_SIZE`宏定义。

如果想要查看最近若干条的历史命令，可以使用`history`命令。执行示例如下：

```
------------------- COMMAND -------------------                             
> root@UCAS_OS: getpid                                                      
1                                                                           
> root@UCAS_OS: ps                                                          
[Process Table]                                                             
 IDX PID STATUS MASK TASK_NAME                                              
[01]  01 RUNNING 0x3 shell @ running on core0                               
current ready_queue:                                                        
> root@UCAS_OS: history                                                     
[History Table]                                                             
 IDX COMMAND                                                                
[00] getpid                                                                 
[01] ps                                                                     
[02] history                                                                
[03]                                                                        
[04]                                                                        
[05]                                                                        
[06]                                                                        
[07]                                                                        
[08]                                                                        
[09]                                                                        
> root@UCAS_OS:              
```

### 屏幕更新相关

使用`clear`命令来清屏。不过，由于我魔改了`drivers/screen.c`文件，使得实现的**shell具有“到达屏幕底端时，自动向上滚一行”的特性**，我们无需经常使用该命令。

（如果没有自动滚屏，可能是因为当前终端窗口太小了。默认的终端高度有50行，见`drivers/screen.c`文件中的`SCREEN_HEIGHT`宏定义。可适当调小这个值。）

## 运作流程



## 实验过程

**这里仅记录实验过程中遇到的一小部分问题与思考。**

**讲义上已包括的内容以及前文提及的设计方案不再赘述。**

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

但这样会带来不同进程对锁的竞争，无法保证“先到先得”。在最终实现中，我延续了先前“传递锁”而非“归还锁再借用锁”的逻辑。我将锁主人的更替操作挪到了原主人释放锁的时刻，从而彻底避免了锁在传递过程中丢失的情况。

### 使用迭代代替递归

在`do_scheduler`函数中，我们需要`check_sleeping`，并从`ready_queue`中选择待调度的任务。此时有可能会遇到`ready_queue`为空的情况。

在最初的实现中，当`ready_queue`为空时，将会递归地执行`ready_queue`。

```c
void do_scheduler(void){
  check_sleeping();
  ...
  if(list_is_empty(&ready_queue)){
    return do_scheduler();
  }
  ...
}
```

注意到`sleep`输入的时间量级一般为秒级，由此将导致内核在这秒级的时间内不断循环递归，进而造成过栈溢出。

这个问题通过在shell中执行`exit`命令或`exce barrier`（后边不加`&`）得以暴露。

通过将递归改为迭代解决了问题。

```c
while(list_is_empty(&ready_queue)) {
  check_sleeping();
}
```

不过在运行多核时，上述逻辑又遇到了新的问题。因此，最终的实现中没有采用上述逻辑。

### 为什么需要两个`current_running`变量？

讲义中曾提到：只有一个`current_running`变量无法管理多个正在运行中的CPU。然而事实上，每个正在运行的CPU也将自身的`current_running`值保存在了`tp`寄存器；如果我们每次进入内核态时从核自身的`tp`寄存器加载`current_running`值，在内核大锁保证同时仅有一个CPU位于内核态的前提下，仅通过一个变量理论上是可以实现预期功能的。

~~然而，这样实践起来还是遇到了问题。这是因为，两个核中的一个可能会长期处于内核态（例如等待就绪队列非空时）。如果只有一个`current_running`变量，则另一个核将无法进入内核态。由此造成了整个系统的阻塞。~~

~~由此可见，这里的双核操作系统**需要满足两个核上的应用能同时处于内核态**。我们不能简单粗暴地使用大锁并共用所有内核态变量。~~

（在最新版本中，通过引入“`ready_queue`为空时`switch_to`到`kernel`进程”的逻辑，使得两个核均不能长期处于内核态等待就绪队列非空，从而仅用一把内核大锁足以完成所需。具体动机与实现如下一部分所述。由于历史原因以及为了后续小锁的实现，我还是保留了两个`current_running`变量。）

### 多核运行时遇到的部分问题

在设计多核逻辑时，因为不清楚`send_ipi`函数的参数与功能，实验进度停滞了很久。期间也曾尝试通过屏蔽主核的核间中断来避免主核多次收到自身发出的核间中断问题。在老师的提示下，我最终改用了“发核间中断-清除`SIP`-打开中断使能”的逻辑来唤醒从核，从而解决了主核被自身的核间中断信号中断的问题。

此外，在实现了多核运行逻辑后，我发现在主核与从核均等待正`sleep`的进程的情况下，当进程结束睡眠后被调度时，将出错。具体描述如下所示：

```
[core 0] before scheduler: 
[core 0] enter with pcb [status RUNNING]
[core 0] switching from [pid 3 name test_barrier] to [pid 3 name test_barrier]
[core 0] after  scheduler: 

[core 1] before scheduler: 
[core 1] enter with pcb [status RUNNING]
[core 1] switching from [pid 4 name test_barrier] to [pid 4 name test_barrier]
[core 1] after  scheduler: 

[core 0] before scheduler: 
[core 0] enter with pcb [status BLOCKED]
[core 1] before scheduler: 
[core 1] enter with pcb [status BLOCKED]
do_unblock pid 3
3 
[core 1] switching from [pid 4 name test_barrier] to [pid 3 name test_barrier]
[core 1] after  scheduler: 
```

如上所示，主核进入`do_scheduler`时，需要完成从`ready_queue`中选择任务替换当前被`blocked`的进程3——但此时`ready_queue`队列为空。根据我先前的逻辑，它将处于`unlock_kernel-lock_kernel-check_sleeping`的循环中，直至`ready_queue`非空。

此时，从核进入了`do_scheduler`，也同样需要完成从`ready_queue`中选择任务替换当前被`blocked`的进程4——此时`ready_queue`队列同样为空。

最终，进程3被`check_sleeping`操作唤醒，进而被从核调度。然而，由于此时主核尚未完成`switch_to`的操作，进程3的`switch_to context`并未保存在其内核栈空间上，从而导致从核`switch_to`时出错。

解决错误的思路有以下两种：

- 通过保留初始kernel进程等方法来确保`ready_queue`始终非空。
- 在`unlock_kernel`前保存`switch_to context`。

这里采用第二种方法。当`ready_queue`为空时，将`kernel`作为换出的进程。`switch_to`时将保存之前进程的上下文信息，并返回`kernel`内的低功耗循环，直至接下来的时钟中断时通过`check_sleeping`发现`ready_queue`非空。

为了提升调度的效率，`kernel`（`pid`为0的进程）永远不会进入`ready_queue`，因此也不会被再次调度。

### shell运行过程中栈溢出

当shell调用BAT任务时，由于最初设置的sd_read缓冲区有1KB，当BAT循环嵌套时容易导致栈溢出。这里采取为shell进程分配更多内核栈空间的方式解决。