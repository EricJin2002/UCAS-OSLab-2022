#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef struct{
    char meaningless_byts[54];
    char pack_num[16];
    char elf_name[512-16];
}__attribute__((packed)) head_pack_t;
head_pack_t head_pack_buf;

typedef struct{
    char meaningless_bytes[54];
    char data[512];
}__attribute__((packed)) data_pack_t;
data_pack_t data_pack_buff;

int pack_len;

int main(void){
    sys_move_cursor(0, 0);
    sys_net_recv(&head_pack_buf, 1, &pack_len);

    printf("head pack len: %d\n",pack_len);

    // there are 54 meaningless bytes ahead...
    for(int i=54;i<pack_len;i++){
        printf("%c", i[(char *)&head_pack_buf]);
    }
    printf("\n\n");

    int pack_num=atoi(head_pack_buf.pack_num);
    printf("pack num: %d\n",pack_num);
    printf("elf name: %s\n\n",head_pack_buf.elf_name);

    int fd=sys_fopen(head_pack_buf.elf_name, O_RDWR);

    for(int i=0;i<pack_num;i++){
        sys_net_recv(&data_pack_buff, 1, &pack_len);
        printf("%s",data_pack_buff.data);
        sys_fwrite(fd, data_pack_buff.data, 512);
    }

    sys_fclose(fd);

    return 0;
}