# Project 5 设备驱动

## 实验任务

本实验包括五个任务。

具体如下：

- 任务1：实现网卡初始化与轮询发送数据帧的功能（C,A,S-core）
- 任务2：实现网卡初始化与轮询接收数据帧的功能（C,A,S-core）
- 任务3：利用时钟中断实现有阻塞的网卡收发包功能（C,A-core）
- 任务4：有网卡中断的收发包（C-core）
- 任务5：网卡同时收发（C-core）

## 运行说明

```sh
make all        # 编译，制作镜像
make run-net    # 启动QEMU，模拟运行
# 进一步上板测试
make floppy     # 将镜像写入SD卡
make minicom    # 监视串口
```

进入BBL界面后，输入`loadboot`命令来启动单核。输入`loadbootm`命令来启动双核。

使用下述命令来抓取qemu上发送的包：

```sh
sudo tcpdump -i tap0 -XX -vvv -nn
```

使用`pktRxTx`程序来测试收发包。

## 设计细节

在“利用中断实现有阻塞的网卡收发包功能”时，我设计了两个函数（见`drivers/e1000.c`），分别用于判断当前是否允许发包/收包。这通过读取`TDT/RDT`寄存器以及相应文件描述符的DD状态位来实现。

```c
int td_sendable(int *tailptr);
int rd_recvable(int *tailptr);
```

借助这两个函数，我又实现了两个函数（见`kernel/net/net.c`），用于从`send_block_queue`以及`recv_block_queue`中唤醒满足条件的进程。

```c
void check_net_send(void);
void check_net_recv(void);
```

在task3中，每次`do_scheduler`时，将调用上述两个函数；
在task4中，当网卡发送特定的中断信号时，将调用相应函数。

## 实验过程

**这里仅记录实验过程中遇到的一小部分问题与思考。**

**讲义上已包括的内容以及前文提及的设计方案不再赘述。**

本实验的主要难点在于环境的配置以及代码的调试。

### 运行qemu时ssh出现断连

这个问题在VMware和VirtualBox上均出现了。在助教的相助下，最终依靠下面两个办法解决了问题：
- 方法一是修改qemu/etc/qemu-ifup和qemu/etc/qemu-ifdown，把开关$IFNAME的语句注释掉。
- 方法二是为虚拟机添加新的网络地址转换（NAT），并将$IFNAME改成新的网卡名。

### VMware上qemu启动较慢，换用VirtualBox

我一开始使用的是VMware虚拟机。在该虚拟机上，每次qemu启动网卡功能时均需要等待较长的时间（大约数分钟）。在听说VirtualBox启动更快时，我尝试配置了在VirtualBox虚拟机上的环境，遇到了以下几个问题：

#### VirtualBox与WSL2不兼容

解决方案是将VirtualBox升级为最新版本，并按照提示升级VirtualBox功能扩展包。（第一节课上提供的VirtualBox版本与WSL2不兼容）

#### 环境迁移

由于我是新建了一个环境，需要将原环境的代码数据与qemu迁移到新环境上。为了避免未知的错误，我按历史所有更新包升级了qemu与uboot（需要注释掉板卡部分的更新代码），并通过传递`.zip`文件的方式拷贝了代码到新的环境上。

在使用VirtualBox虚拟机后，qemu启动速度得到了显著提升。

### 硬件不自动更新TDH寄存器、上板崩溃

在task2中，我遇到了TDH寄存器始终无法自动更新的问题。我一开始以为是qemu的问题，结果上板时触发了未知错误进入了oslab_petalinux...

在经历漫长的debug后，我发现主要的问题在于我修改了TXDESCS的值，新的值没有按照标准要求的那样对齐。（目前看来应该是这个原因，也有可能是因为我之前没有在do_scheduler时flush icache）

### task4中板卡接收描述符不够时始终无法自动触发RXDMT0中断

在发现这个问题后，我第一时间检查了中断使能/掩码/claim ID相关逻辑，也曾试过手动设置ICS寄存器的相应位，是能正确触发中断的，从而确定bug不发生在这些地方。

在经历漫长的debug后，我发现问题在于我错误地设置了RCTL：在rx configure函数里，我在设置RCTL的时候用了| ~E1000_RCTL_BSEX，这样写会导致除了BSEX外的其他位都为1。这里应该用与非才行。

RCTL.RXDMT0等于11是一个保留值，当它为其它三个值时才会使能中断，从而导致了这个bug。