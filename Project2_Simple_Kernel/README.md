# Project 1 引导、镜像文件和ELF文件

## 实验任务

本实验包括五个任务，其中前三个为S-core，前四个为A-core，前五个为C-core.

具体如下：

- 任务1：第一个引导块的制作（C,A,S-core）
- 任务2：开始制作镜像文件（C,A,S-core）
- 任务3：加载并选择启动多个用户程序之一（C,A,S-core）
- 任务4：镜像文件压缩（C,A-core）
- 任务5：批处理运行多个用户程序（C-core）

## 运行说明

```sh
make all        # 编译，制作镜像
make run        # 启动QEMU，模拟运行
# 进一步上板测试
make floppy     # 将镜像写入SD卡
make minicom    # 监视串口
```

进入BBL界面后，输入```loadboot```命令来启动bootloader。将进入以下界面：

```
=> loadboot
It's a bootloader...
Hello OS!
bss check: t version: 2

Type help to show help infomation
$ 
```

此时，操作系统已成功进入kernel，等待用户输入待执行的**应用（Application, 以下简称app）**或**批处理文件（Batch File, 以下简称bat）**。

输入help来查看帮助。

```
Type help to show help infomation
$ help
Type app name to launch an application
or bat name (without ".txt") to launch a batch file
$ 
```

输入应用程序名来执行对应应用。（其实上面的help也是一个应用）

应用程序源代码为```.c```文件，存放在```test/```目录下。

例如执行应用程序```2048```：

```
$ 2048
 2048 - Game
 * Use h-j-k-l / w-a-s-d keys to move the tiles.
 * When two tiles with the same number touch, they merge into one.

              ^                       ^
              k                       w
        < h       l >           < a       d >
              j                       s
              v                       v

 * Commands: 
         n - New game
         q - Exit
Press 'Enter' key to continue.. 
```

输入批处理文件名来执行对应的批处理程序。

批处理程序为```.txt```文件，存放在```bat/```目录下。

例如执行批处理文件```example.txt```：

```
$ example

===Reading batch from image...===
auipc
bss
auipc
bss
bss

auipc
data
qwerty
data
===Finish reading!===


===Now excute batch!===
[auipc] Info: testpc = 0x520102f6
[bss] Info: passed bss check!
[auipc] Info: testpc = 0x520102f6
[bss] Info: passed bss check!
[bss] Info: passed bss check!
Task name empty!
[auipc] Info: testpc = 0x520102f6
[data] Info: checking data!
[data] Info: checking data --- [OK]!
No task named qwerty!
[data] Info: checking data!
[data] Info: checking data --- [OK]!
===All tasks in batch are excuted!===

$ 
```

所有往命令行输入的指令，或批处理文件中的每一行，都被视作一个**任务（task）**。任务可以是一个**app**，也可以是一个**bat**

P.S. 这意味着你甚至可以在批处理文件中调用批处理文件！

P.P.S. 甚至可以让批处理文件自己调用自己！详见```bat/cycle.txt```

（虽然这样只会死循环... 为了好玩加了个名为pause的用户程序，来为每次循环暂停一下...

如果任务名为空，将提示```Task name empty!```

如果不存在该任务，将提示```No task named xxx!```
（例如```example```执行结果中的```No task named qwerty!```）

## 运作流程

### 引导块

```arch/riscv/boot/bootblock.S```完成下述操作：

- 打印```"It's bootblock!"```
- 将sd卡中的kernel所在块加载到内存
- **将sd卡中的task-info所在块加载到内存（暂时存放在应用程序入口地址处）**
- 跳转到kernel入口地址

### 内核起点

```arch/riscv/kernel/head.S```完成下述操作：

- 清空bss段
- 设置内核栈指针

```init/main.c```完成下述操作：

- 检验bss段已清空
- 初始化bios
- **读取暂存在应用程序入口地址的task-info，保存到位于bss段的全局变量中，以防被后续应用覆盖**
- 读取用户输入，调用各应用/批处理程序

### 用户任务的加载与执行

```kernel/loader/loader.c```共提供了两个函数，以供```init/main.c```调用：

```h
uint64_t load_task_img(int taskid);
void excute_task_img_via_name(char *taskname);
```

其中，

```load_task_img```将从sd卡中读取指定```taskid```下的task。若task类型为app，则返回该应用程序的入口地址。若task类型为bat，则直接依次执行该bat内所有task，结束后返回0。该函数的调用者进而根据函数返回值来判断是否要跳转到应用程序。

```excute_task_img_via_name```将在```task-info```信息中逐个比对输入的task名。若输入的任务存在，则调用```load_task_img```函数执行该任务；否则，输出反馈信息。

### 测试程序入口

```crt0.S```完成下述操作：

- 清空bss段
- 保存返回地址到栈空间
- 调用main
- 恢复栈中的返回地址
- 返回内核

## 设计细节

### 镜像文件结构

镜像文件由```createimage.c```合成，具体依次存放了以下信息：

- bootblock (padding到512B)
- kernel
- app 1, app 2, ... , app n
- bat 1, bat 2, ... , bat m
- task-info

其中，

在任务三中，kernel和各个app均被padding到15个扇区（512B）。

在任务四中，kernel，各个app以及app-info依次紧密存储。

在任务五中，kernel，各个app，各个bat以及task-info依次紧密存储。

### task-info结构

task-info主要包括以下信息：

```h
// include/os/task.h

// for [p1-task5]
typedef enum{
    app, bat
} TYPE;

typedef struct {
    char name[32];
    int offset;
    int size;
    uint64_t entrypoint;
    TYPE type; // for [p1-task5]
} task_info_t;

extern task_info_t tasks[TASK_MAXNUM];

// for [p1-task4]
extern uint16_t task_num;
```

其中，
- ```name```属性用来标识task的调用名（对于bat来说不含“.txt”后缀）
- ```offset```属性用来记录该task在image文件中距离文件开始位置的偏移量
- ```size```属性用来记录该task在image文件中的字节数
- ```entrypoint```属性来自ELF文件中记载的程序入口地址

在实验五中，bat-info和app-info以相同的格式一并保存在task-info中，因此增设了```type```属性来指示当前task的类型。```task_num```为所有app和bat的总数。

### 辅助信息的存储

bios提供的```bios_sdread```函数需要提供块id，块数信息来一块一块地（512B）读取sd卡。这意味着需要存储kernel块数，task-info块id、块数信息，以辅助sd上数据块的加载。

注意到bootblock块末尾有大端空白，因此将这些信息存储在了bootblock块的最后的12B中。依次为：

- task-info信息在image文件中的偏移量（用于判断块中有效信息位置），4B
- task-info块id，2B
- task-info块数，2B
- kernel块数，2B
- task_num，2B

这些信息随bootblock块在一开始就被加载到内存。

## 实验心得

### 善于使用工具

在开始调试代码时，我因为不擅长使用gdb，导致代码调试陷入了瓶颈。随着我逐渐熟识了gdb工具，代码编写也不再困难。我还掌握了```vim```中的```:%!xxd```指令和```hexdump```指令，来查看image的二进制编码。我发现，```hexdump```指令和```vim```中的```:%!xxd```指令输出不尽相同。前者输出时会自动调整相邻字节的输出顺序，而后者不会。

### 严格按位宽处理数据

在代码调试的过程中，我一开始使用了short, int, long等变量定义数据。由于事先未关注各数据的位宽大小，在编译时，不同位宽数据在转换的过程中收到了不少warning。最后的解决方案是用uintxx_t类型取代位宽模糊的short, int, long, long long类型。

此外，在上板测试的过程中，我和另外两个同学被2048程序无法正常相应回车键的问题困扰很久。最后在老师和同学们的帮助下，发现是因为我们在栈中保存和恢复返回地址时错误地使用了lw,sw指令（而非ld,sd指令），并且栈空间也仅开了4字节（而非8字节）。这是因为我惯性地顺从了体系结构研讨课的思维，误以为该机器是32位。但实际上该机器是64位的，栈指针的非对齐访问导致了最终的出错。

在此感谢所有耐心指导的老师和同学们！

## 目录结构

```
Project1_BootLoader/
├── arch
│   └── riscv
│       ├── bios
│       │   └── common.c
│       ├── boot
│       │   └── bootblock.S
│       ├── crt0
│       │   └── crt0.S
│       ├── include
│       │   ├── asm
│       │   │   └── biosdef.h
│       │   ├── asm.h
│       │   ├── common.h
│       │   └── csr.h
│       └── kernel
│           └── head.S
├── bat
│   ├── cycle.txt
│   └── example.txt
├── build
├── createimage
├── include
│   ├── os
│   │   ├── bios.h
│   │   ├── loader.h
│   │   ├── string.h
│   │   └── task.h
│   └── type.h
├── init
│   └── main.c
├── kernel
│   └── loader
│       └── loader.c
├── libs
│   └── string.c
├── Makefile
├── README.md
├── riscv.lds
├── task2.sh
├── test
│   └── test_project1
│       ├── 2048.c
│       ├── auipc.c
│       ├── bss.c
│       ├── data.c
│       └── pause.c
├── tiny_libc
│   └── include
│       └── bios.h
└── tools
    └── createimage.c
```