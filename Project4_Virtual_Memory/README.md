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

使用`clear`命令来清屏。不过，由于我魔改了`drivers/screen.c`文件，使得实现的**shell支持“到达屏幕底端时，自动向上滚一行”**，我们无需经常使用该命令。

（如果没有自动滚屏，可能是因为当前终端窗口太小了。默认的终端高度有50行，见`drivers/screen.c`文件中的`SCREEN_HEIGHT`宏定义。可适当调小这个值。）

## 运作流程

这里仅简单介绍双核是如何启动的。

对于主核：
- 像前几个实验一样正常地进入`init/main.c`
- 完成各逻辑的初始化
- 通过发核间中断唤醒从核
- 清除自身SIP寄存器
- 开中断

对于从核：
- `bootblock.S`中令中断入口地址指向kernel
- 只开启软件中断（核间中断）
- 低功耗等待来自主核的唤醒
- `head.S`中跳过bss段清空（以避免之前由主核初始化的jump table等变量被清空）
- `head.S`中分配与主核不同的栈空间，用于执行kernel
- 进入`init/main.c`
- 为自己再分配一块内核栈空间，用于处理中断（保存上下文等）
- 在新的内核栈空间内创建一个新的PCB
- 初始化该PCB，并保存其地址到自身tp寄存器
- 重新设置中断入口地址，设置时钟中断
- 开中断

## 设计细节

### drivers/screen.c 内自动滚屏的实现

该文件内我实现了一个滚屏函数，用于将第`shell_begin`行下的所有行向上移动一行。

```c
void screen_scroll(int shell_begin);
```

除此之外，我魔改了`screen_write_ch`函数。每次输出`\n`时，将判断当前行是否为最后一行；若是，将调用滚屏函数。

由此，我实现了前文中“shell到达屏幕底端时，自动向上滚一行”的功能。

### mutex lock / barrier / condition / mailbox

这四类操作的实现使用了相似的逻辑。各自对应的结构体内大都包含了
- `key/name`：用于区分不同的对象。
- `handle_num`：用于记录曾对当前`key/name`调用过`init`并分配的`handle`数目。
- `block_queue`：用于记录正因当前对象未满足条件而被阻塞的进程。
- `lock`：一个自旋锁，用于保护操作的原子性。


每次调用`init`时，会先遍历当前结构体数组，查找是否有`key`值相同且`handle_num`非空（标记已被分配）的对象。如果有，则返回相应的`handle`值，并令`handle_num`自增；如果无，则再次遍历结构体数组，分配一个`handle_num`为空的对象，并令`handle_num`自增。

每次调用`destroy`时，会令`handle_num`自减，并释放`block_queue`内被阻塞的所有进程。

### taskset

为了实现绑核相关逻辑，我在PCB结构体内增加一个`mask`域。对于初始时运行的进程，其值为`0x3`，表示可以在两个核中的任意一个上运行。通过`taskset`命令为进程绑核。

两个核具有独立的`current_running`变量。（实际上由一个`current_running_of[2]`数组实现，用`current_running_of[get_current_cpu_id()]`代替`current_running`）

两个核共享同个`ready_queue`。每次调度时，将通过`include/os/list.h`文件内的下述函数从`ready_queue`中寻找一个满足`filter`函数返回值条件的进程节点。这里的`filter`即要求PCB域内的`mask`值允许进程在当前核上运行。

```c
static inline list_node_t *list_find_and_pop(list_head *queue, void *filter)；
```

如果未找到符合条件的进程，将调度回kernel，而非留在内核态忙等待。其原因见下述“实验过程”中“多核运行时遇到的部分问题”。

## 实验过程

**这里仅记录实验过程中遇到的一小部分问题与思考。**

**讲义上已包括的内容以及前文提及的设计方案不再赘述。**

### `exec`指令传递参数时内核态出现缺页

默认增加的缺页处理机制只能应对用户态出现缺页，但如果使用了`get_kva_of`函数，在内核态是有缺页风险的。这一点在内核调用`exec`函数往用户栈中赋值时得到体现。具体如下：

`exec`函数需要创建一个新的进程。这个过程包含多次对页的分配。但是此时如果剩余页资源紧张，内核需要将已被占用的页换到sd卡来分配新的页。在这一过程中，刚分配的用户栈空间也有可能被换出内存。如果对内核栈的赋值操作在换出内存之后，将导致`get_kva_of`返回值无效。此时需要先将这个用户栈页换回内存，再继续完成后续的修改操作。

除此之外，调用`exec`指令的进程也保存了参数（`argv`指针数组内元素指向的字符串）在它的用户栈空间上。这个用户栈页也有可能被换出内存。这里的解决方法是：先将该部分参数存入kernel的静态内存空间，然后再为新进程分配空间，最后初始化新进程用户栈时从kernel的静态内存空间中搬运参数。

### `load_app_img`时内核态出现缺页

`load_app_img`函数用于完成从sd卡加载image到内存的操作。这个操作往往涉及对多个页的初始化，这些页是一个接着一个分配的。根据我最初设计的逻辑，每个页将首先一次性从sd卡读取8（=4096/512）个块，然后利用`strcpy`函数移动页内数据。（需要移动数据是因为sd卡中image文件内部各个task所在的段并非按块对齐的。）每个页最终的数据来自初始时自身页的页尾与下一个页的页首。

这在task3中遇到了bug。经过漫长的debug（该bug查了好久好久~），我发现当内存空闲页空间不多时，这些页有可能未正常初始化！

这是因为，如果`load_app_img`时内存空闲页资源紧张，将会出现先前分配的页尚未完成初始化，便被换出了内存的情况。我们必须一次性读两个页的块数，来确保申请下一个页时，前一个页已经完成了初始化。
