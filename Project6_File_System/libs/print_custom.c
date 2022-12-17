#include <printk.h>

void print_pos_num_with_blanks_leading(int num, int len){
    int num_len=1;
    int num_tmp=num/10;
    while(num_tmp>0){
        num_len++;
        num_tmp/=10;
    }
    for(int i=0;i<len-num_len;i++){
        printk(" ");
    }
    printk("%d",num);
}