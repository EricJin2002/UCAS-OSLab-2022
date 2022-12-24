# Project 6 文件系统

## 实验任务

本实验包括三个任务。

具体如下：

- 任务1：物理文件系统的实现（C,A,S-core）
- 任务2：文件操作（C,A,S-core）
- 任务3：操作系统综合测试（C-core）

## 运行说明

```sh
make all        # 编译，制作镜像
make run        # 启动基础QEMU，模拟运行
make run-net    # 启动带有网卡功能的QEMU，模拟运行
# 进一步上板测试
make floppy     # 将镜像写入SD卡
make minicom    # 监视串口
```

进入BBL界面后，输入`loadboot`命令来启动单核。输入`loadbootm`命令来启动双核。

注意到执行`make run-net`需要花费较长时间，不方便调试。因此可以在注释掉`init/main.c`中`#define NET`语句后，采用`make run`命令来加速对非网卡相关功能部分的调试。

使用`pktRxTx_elf/pktRxTx`程序来测试收发包。

### 增加的shell命令

支持以下shell命令，具体功能详见讲义，不再赘述。
- `mkfs`
- `mkdir`
- `rmdir`
- `ls`
- `statfs`
- `cd`
- `touch`
- `cat`
- `ln`
- `ls -l`
- `rm`
- `batch`

### `batch`命令

这里具体介绍一下`batch`命令。

该命令支持执行文件系统内部的`batch`文件。`batch`文件内的每一行应为一句对用户程序的调用，支持参数的传递，支持在语句末尾条件`&`来非阻塞地运行各程序。

一个调用例子如下：

我们先用`exec bat1`生成一个`batch`文件`1.bat`。随后用`batch 1.bat`来执行这个`batch`文件。执行结果如下：

```
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
> [TASK] Thread created: tid1=1 tid2=2                                          
add1 thread is calculating *ans1+=data[313] (sum 48828)                         
add2 thread is calculating *ans2+=data[932] (sum 254146)                        
                                                                                
                          _                                                     
                        -=\`\                                                   
                    |\ ____\_\__                                                
                  -=\c`""""""" "`)                                              
                     `~~~~~/ /~~`                                               
                      -==/ /                                                    
                        '-'                                                     
                                                                                
                                                                                
                                                                                
------------------- COMMAND -------------------                                 
> root@UCAS_OS: exec bat1                                                       
Info: Process pid 4 is launched.                                                
> root@UCAS_OS: ls                                                              
Parsing ls command as [ls path]...                                              
. .. 1.bat                                                                      
> root@UCAS_OS: cat 1.bat                                                       
fly &                                                                           
add2                                                                            
> root@UCAS_OS: batch 1.bat                                                     
Executing batch:                                                                
fly &                                                                           
add2                                                                            
> root@UCAS_OS:                                                                 
                                                                                
```

## 设计细节

### 新增文件

文件系统相关函数的实现位于`kernel/fs/fs.c`和`kernel/fs/utils.c`这两个文件。

为了让数字打印对齐（方便制表），我将`printk`函数进行了包装。这部分实现位于`libs/print_custom.c`这个文件。

### 文件系统的存储位置

文件系统在磁盘上的存储位置紧接着`swap`部分（中间没有任何空白填充）。依次存储了超级块、数据块的bitmap、inode的bitmap、inode、数据块。各个块的位置与大小可以通过`statfs`命令获得，也可以参考`include/os/fs.h`文件中各个宏的定义。

### recv功能

C-core要求的recv功能实现在`test/test_project6/recv.c`文件。该用户程序会接收来自网卡的文件并写入文件系统。

可以在QEMU上使用`pktRxTx_elf/pktRxTx`小程序测试这一功能。执行的命令为：

```
sudo pktRxTx -m 1
```

进入小程序界面后输入`send`来发送文件。这里建议发送位于同一目录下的`net.bat`文件。运行该`batch`文件将执行多次`test`测试程序。

### test测试

C-core要求的综合测试实现在`test/test_project6/test.c`文件。该文件主要调用了这个实验以及之前几个实验中实现的相关函数。测试结果会反馈在屏幕以及输入参数指定的log文件中。

## 实验过程

由于临近期末考试周，这个实验我在编程的时候没有记录实验过程。