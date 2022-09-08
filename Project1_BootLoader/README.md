# Project 1 引导、镜像文件和ELF文件

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