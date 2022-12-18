#include <os/string.h>
#include <os/fs.h>

// for [p6-task1]
void init_fs(void){
    int superblock_sector_id = swap_end_sector_id;
    assert(sizeof(superblock_t)==SECTOR_SIZE);
    sd_read(kva2pa(&superblock), 1, superblock_sector_id);

    if(superblock.magic==SUPERBLOCK_MAGIC){
        cwd_inode_id=superblock.root_inode_id;
        printk("[FS] Filesystem already exists!\n");
    }else{
        do_mkfs();
    }

    // init file descriptors
    for(int i=0;i<NUM_FDESCS;i++){
        fdesc_array[i].valid=0;
    }
}

int do_mkfs(void)
{
    // TODO [P6-task1]: Implement do_mkfs
    printk("[FS] Start initialize filesystem!\n");
    printk("[FS] Setting superblock...\n");

    superblock.magic = SUPERBLOCK_MAGIC;
    superblock.start_sector_id = swap_end_sector_id;

    superblock.superblock_sector_num = 1;
    superblock.blockmap_sector_num = FS_DATABLOCK_NUM/SECTOR_SIZE/8;
    superblock.inodemap_sector_num = FS_INODE_NUM/SECTOR_SIZE/8;
    superblock.inode_sector_num = FS_INODE_NUM;
    superblock.data_sector_num = FS_DATA_TOT_SIZE/SECTOR_SIZE;

    superblock.superblock_sector_offset = 0;
    superblock.blockmap_sector_offset = superblock.superblock_sector_offset + superblock.superblock_sector_num;
    superblock.inodemap_sector_offset = superblock.blockmap_sector_offset + superblock.blockmap_sector_num;
    superblock.inode_sector_offset = superblock.inodemap_sector_offset + superblock.inodemap_sector_num;
    superblock.data_sector_offset = superblock.inode_sector_offset + superblock.inode_sector_num;
    superblock.sector_num = superblock.data_sector_offset + superblock.data_sector_num;

    superblock.inode_entry_size = sizeof(inode_entry_t);
    superblock.dir_entry_size = sizeof(dentry_t);

    do_statfs();
    sd_write(kva2pa(&superblock), 1, superblock.start_sector_id);

    
    static char blank[SECTOR_SIZE];
    memset(blank, 0, SECTOR_SIZE);

    printk("[FS] Setting inode-map...\n");
    for(int i=0;i<superblock.inodemap_sector_num;i++){
        sd_write(kva2pa(blank), 1, superblock.start_sector_id + superblock.inodemap_sector_offset + i);
    }

    printk("[FS] Setting block-map...\n");
    for(int i=0;i<superblock.blockmap_sector_num;i++){
        sd_write(kva2pa(blank), 1, superblock.start_sector_id + superblock.blockmap_sector_offset + i);
    }

    printk("[FS] Setting inode...\n");
    cwd_inode_id=create_new_dir(1,-1);
    superblock.root_inode_id=cwd_inode_id;
    
    printk("[FS] Initialize filesystem finished!\n");
    return 0;  // do_mkfs succeeds
}

int do_statfs(void)
{
    // TODO [P6-task1]: Implement do_statfs
    if(superblock.magic!=SUPERBLOCK_MAGIC){
        printk("[FS] Filesystem not exists!");
        return -1;
    }

    printk("magic 0x%x\n", superblock.magic);
    printk("sector num %d   start sector id %d\n",superblock.sector_num, superblock.start_sector_id);
    printk("inode entry size %dB   dir entry size %dB\n",superblock.inode_entry_size, superblock.dir_entry_size);
    
    printk("[Sector Info]\n");
    printk("CONTENT    OFFSET     NUM\n");
    // printk("superblock %d      %d\n", superblock.superblock_sector_offset, superblock.superblock_sector_num);
    // printk("blockmap   %d      %d\n", superblock.blockmap_sector_offset, superblock.blockmap_sector_num);
    // printk("inodemap   %d     %d\n", superblock.inodemap_sector_offset, superblock.inodemap_sector_num);
    // printk("inode      %d     %d\n", superblock.inode_sector_offset, superblock.inode_sector_num);
    // printk("data       %d   %d\n", superblock.data_sector_offset, superblock.data_sector_num);
    
    printk("superblock ");
    print_pos_num_with_blanks_leading(superblock.superblock_sector_offset, 6);
    printk(" ");
    print_pos_num_with_blanks_leading(superblock.superblock_sector_num, 7);
    printk("\n");

    printk("blockmap   ");
    print_pos_num_with_blanks_leading(superblock.blockmap_sector_offset, 6);
    printk(" ");
    print_pos_num_with_blanks_leading(superblock.blockmap_sector_num, 7);
    printk("\n");

    printk("inodemap   ");
    print_pos_num_with_blanks_leading(superblock.inodemap_sector_offset, 6);
    printk(" ");
    print_pos_num_with_blanks_leading(superblock.inodemap_sector_num, 7);
    printk("\n");

    printk("inode      ");
    print_pos_num_with_blanks_leading(superblock.inode_sector_offset, 6);
    printk(" ");
    print_pos_num_with_blanks_leading(superblock.inode_sector_num, 7);
    printk("\n");

    printk("data       ");
    print_pos_num_with_blanks_leading(superblock.data_sector_offset, 6);
    printk(" ");
    print_pos_num_with_blanks_leading(superblock.data_sector_num, 7);
    printk("\n");

    return 0;  // do_statfs succeeds
}

int do_cd(char *path)
{
    // TODO [P6-task1]: Implement do_cd
    // parse path
    char *parsed_path[FS_DIR_MAX_LEVEL];  // note: this implies that the level of dir < 10
    int cnt=parse_path(path, parsed_path);

    // grope path
    int dest_inode_id = grope_path(parsed_path, cnt);
    if(dest_inode_id==-1){
        return -1;
    }

    static inode_t tmp_inode;
    sd_read_inode(&tmp_inode, dest_inode_id);
    if(tmp_inode.type!=DIR){
        printk("Error: %s not a dir!\n", parsed_path[cnt-1]);
        return -1;
    }

    cwd_inode_id=dest_inode_id;

    return 0;  // do_cd succeeds
}

int do_mkdir(char *path)
{
    // TODO [P6-task1]: Implement do_mkdir
    // parse path
    char *parsed_path[FS_DIR_MAX_LEVEL];  // note: this implies that the level of dir < 10
    int cnt=parse_path(path, parsed_path);

    // grope path
    int father_inode_id = grope_path(parsed_path, cnt-1);
    if(father_inode_id==-1){
        return -1;
    }

    // read father inode
    static inode_t father_inode;
    sd_read_inode(&father_inode, father_inode_id);
    if(father_inode.type!=DIR){
        printk("Error: %s not a dir!\n", parsed_path[cnt-2]);
        return -1;
    }

    // search in the direct dentry
    if(search_inode_from_inode(&father_inode, parsed_path[cnt-1])!=-1){
        // son inode already exists
        printk("Error: %s already exists!\n", parsed_path[cnt-1]);
        return -1;
    }

    // no such dir, create one
    int newly_created_inode_id = create_new_dir(0, father_inode_id);

    // record dentry in last dir
    record_dentry_to_inode(&father_inode, parsed_path[cnt-1], newly_created_inode_id);
    
    return 0;  // do_mkdir succeeds
}

int do_rmdir(char *path)
{
    // TODO [P6-task1]: Implement do_rmdir
    // parse path
    char *parsed_path[FS_DIR_MAX_LEVEL];  // note: this implies that the level of dir < 10
    int cnt=parse_path(path, parsed_path);

    // grope path
    int father_inode_id = grope_path(parsed_path, cnt-1);
    if(father_inode_id==-1){
        return -1;
    }

    // read father inode
    static inode_t father_inode;
    sd_read_inode(&father_inode, father_inode_id);
    if(father_inode.type!=DIR){
        printk("Error: %s not a dir!\n", parsed_path[cnt-2]);
        return -1;
    }

    // search in the direct dentry
    int son_inode_id=search_inode_from_inode(&father_inode, parsed_path[cnt-1]);
    if(son_inode_id==-1){
        // no such dir
        printk("Error: No dir named %s!\n", parsed_path[cnt-1]);
        return -1;
    }

    // son inode exists
    static inode_t son_inode;
    sd_read_inode(&son_inode, son_inode_id);
    if(son_inode.type!=DIR){
        printk("Error: %s not a dir!\n", parsed_path[cnt-1]);
        return -1;
    }

    // judge if son dir is empty
    static dir_t son_dir;
    for(int i=0;i<FS_DIRECT_INODE_ENTRY_NUM;i++){
        if(son_inode.direct[i].valid){
            sd_read_data(&son_dir, son_inode.direct[i].datablock_id);
            int son_dir_cnt = son_dir.cnt;
            for(int j=0;j<FS_DENTRY_NUM&&son_dir_cnt>0;j++){
                if(son_dir.dentries[j].valid){
                    son_dir_cnt--;
                    if(strcmp(son_dir.dentries[j].name, ".")&&strcmp(son_dir.dentries[j].name, "..")){
                        printk("Error: %s not empty!\n", parsed_path[cnt-1]);
                        return -1;
                    }
                }
            }
        }else{
            break;
        }
    }

    // judge if removing itself
    if(!strcmp(parsed_path[cnt-1],".") || !strcmp(parsed_path[cnt-1],"..")){
        printk("Error: Illegal dir name!\n");
        return -1;
    }

    // delete son inode in father dir
    delete_dentry_from_inode(&father_inode, parsed_path[cnt-1]);

    // decrease inode nlinks
    son_inode.nlinks--;
    sd_write_inode(&son_inode, son_inode_id);

    // delete son inode if nlinks comes to 0
    if(!son_inode.nlinks){
        free_all_datablocks_in_inode(&son_inode);
        free_inode(son_inode_id);
    }

    return 0;  // do_rmdir succeeds
}

int do_ls(char *path, int option)
{
    // TODO [P6-task1]: Implement do_ls
    // Note: argument 'option' serves for 'ls -l' in A-core
    char *parsed_path[FS_DIR_MAX_LEVEL];  // note: this implies that the level of dir < 10
    int cnt=parse_path(path, parsed_path);
    
    // grope path
    int dest_inode_id=grope_path(parsed_path, cnt);
    if(dest_inode_id==-1){
        return -1;
    }

    // read inode
    static inode_t tmp_inode;
    sd_read_inode(&tmp_inode, dest_inode_id);
    if(tmp_inode.type!=DIR){
        printk("Error: %s not a dir!\n", parsed_path[cnt-1]);
        return -1;
    }

    // print file/dir name
    if(option==1){
        printk("TYPE ID NLINKS SIZE NAME\n");
    }

    static dir_t tmp_dir;
    for(int i=0;i<FS_DIRECT_INODE_ENTRY_NUM;i++){
        if(tmp_inode.direct[i].valid){
            sd_read_data(&tmp_dir, tmp_inode.direct[i].datablock_id);
            int tmp_dir_cnt = tmp_dir.cnt;
            for(int j=0;j<FS_DENTRY_NUM&&tmp_dir_cnt>0;j++){
                if(tmp_dir.dentries[j].valid){
                    tmp_dir_cnt--;
                    if(option==1){
                        static inode_t son_inode;
                        sd_read_inode(&son_inode, tmp_dir.dentries[j].inode_id);

                        printk("%s ",son_inode.type==DIR?" DIR":"FILE");
                        print_pos_num_with_blanks_leading(son_inode.id, 2);
                        printk(" ");
                        print_pos_num_with_blanks_leading(son_inode.nlinks, 6);
                        printk(" ");
                        print_pos_num_with_blanks_leading(son_inode.size, 4);
                        printk(" ");

                        // print name
                        printk("%s",tmp_dir.dentries[j].name);
                        for(int k=0;k<32-strlen(tmp_dir.dentries[j].name);k++){
                            printk(" ");
                        }

                        printk("\n");
                    }else{
                        printk("%s ",tmp_dir.dentries[j].name);
                    }
                }
            }
        }else{
            break;
        }
    }

    if(option==0){
        printk("\n");
    }

    return 0;  // do_ls succeeds
}

int do_touch(char *path)
{
    // TODO [P6-task2]: Implement do_touch
    // parse path
    char *parsed_path[FS_DIR_MAX_LEVEL];  // note: this implies that the level of file < 10
    int cnt=parse_path(path, parsed_path);

    // grope path
    int father_inode_id = grope_path(parsed_path, cnt-1);
    if(father_inode_id==-1){
        return -1;
    }

    // read father inode
    static inode_t father_inode;
    sd_read_inode(&father_inode, father_inode_id);
    if(father_inode.type!=DIR){
        printk("Error: %s not a dir!\n", parsed_path[cnt-2]);
        return -1;
    }

    // search in the direct dentry
    if(search_inode_from_inode(&father_inode, parsed_path[cnt-1])!=-1){
        // file already exists
        printk("Error: %s already exists!\n", parsed_path[cnt-1]);
        return -1;
    }

    // no such file, create one
    int newly_created_inode_id = create_new_file(father_inode_id);
    
    // record dentry in last dir
    record_dentry_to_inode(&father_inode, parsed_path[cnt-1], newly_created_inode_id);
    
    return 0;  // do_touch succeeds
}

int do_cat(char *path)
{
    // TODO [P6-task2]: Implement do_cat
    // parse path
    char *parsed_path[FS_DIR_MAX_LEVEL];
    int cnt=parse_path(path, parsed_path);

    // grope path
    int father_inode_id = grope_path(parsed_path, cnt-1);
    if(father_inode_id==-1){
        return -1;
    }

    // read father inode
    static inode_t father_inode;
    sd_read_inode(&father_inode, father_inode_id);
    if(father_inode.type!=DIR){
        printk("Error: %s not a dir!\n", parsed_path[cnt-2]);
        return -1;
    }

    // search in the direct dentry
    int son_inode_id=search_inode_from_inode(&father_inode, parsed_path[cnt-1]);
    if(son_inode_id==-1){
        // no such file
        printk("Error: No file named %s!\n", parsed_path[cnt-1]);
        return -1;
    }

    // son inode exists
    static inode_t son_inode;
    sd_read_inode(&son_inode, son_inode_id);
    if(son_inode.type!=FILE){
        printk("Error: %s not a file!\n", parsed_path[cnt-1]);
        return -1;
    }

    // read file
    for(int i=0;i<FS_DIRECT_INODE_ENTRY_NUM;i++){
        print_datablock(0, &son_inode.direct[i]);
    }
    print_datablock(1, &son_inode.indirect);
    print_datablock(2, &son_inode.double_indirect);
    print_datablock(3, &son_inode.triple_indirect);

    return 0;  // do_rm succeeds 
}

// use fd to cat, abandoned
int do_cat__old_version(char *path)
{
    // TODO [P6-task2]: Implement do_cat
    int fd=do_fopen(path, O_RDONLY);
    if(fd==-1){
        return -1;
    }

    static char buff[FS_DATABLOCK_SIZE];
    int len;
    do{
        len=do_fread(fd, buff, FS_DATABLOCK_SIZE);
        for(int i=0;i<len;i++){
            printk("%c",buff[i]);
        }
    }while(len>0);

    do_fclose(fd);

    return 0;  // do_cat succeeds
}

int do_fopen(char *path, int mode)
{
    // TODO [P6-task2]: Implement do_fopen
    for(int i=0;i<NUM_FDESCS;i++){
        if(!fdesc_array[i].valid){
            // parse path
            char *parsed_path[FS_DIR_MAX_LEVEL];  // note: this implies that the level of file < 10
            int cnt=parse_path(path, parsed_path);
            
            // grope path
            int father_inode_id = grope_path(parsed_path, cnt-1);
            if(father_inode_id==-1){
                return -1;
            }

            // read father inode
            static inode_t father_inode;
            sd_read_inode(&father_inode, father_inode_id);
            if(father_inode.type!=DIR){
                printk("Error: %s not a dir!\n", parsed_path[cnt-2]);
                return -1;
            }

            // search in the direct dentry
            int tmp_inode_id=search_inode_from_inode(&father_inode, parsed_path[cnt-1]);
            if(tmp_inode_id!=-1){
                // same name already exists
                static inode_t tmp_inode;
                sd_read_inode(&tmp_inode, tmp_inode_id);
                if(tmp_inode.type!=FILE){
                    printk("Error: %s not a file!\n", parsed_path[cnt-1]);
                    return -1;
                }
            }else{
                if(mode==O_RDONLY){
                    printk("Error: Reading a file not exists!\n");
                    return -1;
                }

                // no such file, create one
                tmp_inode_id = create_new_file(father_inode_id);
                
                // record dentry in last dir
                record_dentry_to_inode(&father_inode, parsed_path[cnt-1], tmp_inode_id);
            }

            // update file descriptor
            fdesc_array[i].valid=1;
            fdesc_array[i].mode=mode;
            fdesc_array[i].rd_off=0;
            fdesc_array[i].wr_off=0;
            fdesc_array[i].inode_id=tmp_inode_id;
            return i;   // return the id of file descriptor
        }
    }

    printk("Error: No available fd!\n");
    return -1;
}

int do_fread(int fd, char *buff, int length)
{
    // TODO [P6-task2]: Implement do_fread
    if(fd<0 || fd>=NUM_FDESCS || !fdesc_array[fd].valid){
        printk("Error: fd not valid!\n");
        return -1;
    }

    if(fdesc_array[fd].mode==O_WRONLY){
        printk("Error: Not readable!\n");
        return -1;
    }

    // read inode
    static inode_t tmp_inode;
    sd_read_inode(&tmp_inode, fdesc_array[fd].inode_id);

    // read off and len
    int off=fdesc_array[fd].rd_off;
    int len=length;
    if(len+off>tmp_inode.size){
        len=tmp_inode.size-off;
    }
    if(len<0){
        // the read off is already beyond the end of file
        // do nothing
        len=0;
    }

    // read
    int tmp_len=len;
    while(tmp_len>0){
        int datablock_no = off/FS_DATABLOCK_SIZE;
        // assert(datablock_no<FS_DIRECT_INODE_ENTRY_NUM);

        inode_entry_t *target_entry;
        if(datablock_no<FS_DIRECT_INODE_ENTRY_NUM){
            // direct
            target_entry = find_datablock(0, 0, tmp_inode.direct+datablock_no);
            sd_write_inode(&tmp_inode, tmp_inode.id);

            // the same as followings:
            // target_entry = tmp_inode.direct+datablock_no;
            // if(!target_entry->valid){
            //     target_entry->valid=1;
            //     target_entry->datablock_id=alloc_datablock();
            //     sd_write_inode(&tmp_inode, tmp_inode.id);
            // 
            //     static char blank[FS_DATABLOCK_SIZE];
            //     memset(blank, 0, FS_DATABLOCK_SIZE);
            //     sd_write_data(blank, target_entry->datablock_id);
            // }

            goto find;
        }
        
        datablock_no-=FS_DIRECT_INODE_ENTRY_NUM;
        if(datablock_no<FS_DBTABLE_ENTRY_NUM){
            // indirect
            target_entry = find_datablock(1, datablock_no, &tmp_inode.indirect);
            sd_write_inode(&tmp_inode, tmp_inode.id);

            goto find;
        }

        datablock_no-=FS_DBTABLE_ENTRY_NUM;
        if(datablock_no<FS_DBTABLE_ENTRY_NUM*FS_DBTABLE_ENTRY_NUM){
            // double indirect
            target_entry = find_datablock(2, datablock_no, &tmp_inode.double_indirect);
            sd_write_inode(&tmp_inode, tmp_inode.id);

            goto find;
        }

        datablock_no-=FS_DBTABLE_ENTRY_NUM*FS_DBTABLE_ENTRY_NUM;
        if(datablock_no<FS_DBTABLE_ENTRY_NUM*FS_DBTABLE_ENTRY_NUM*FS_DBTABLE_ENTRY_NUM){
            // triple indirect
            target_entry = find_datablock(3, datablock_no, &tmp_inode.triple_indirect);
            sd_write_inode(&tmp_inode, tmp_inode.id);

            goto find;
        }

        // file exceeds maximal size
        assert(0);

        static char tmp_datablock[FS_DATABLOCK_SIZE];
    find:    
        sd_read_data(tmp_datablock, target_entry->datablock_id);

        for(int i=off%FS_DATABLOCK_SIZE;i<FS_DATA_TOT_SIZE&&tmp_len>0;i++){
            *(buff++)=tmp_datablock[i];
            tmp_len--;
            off++;
        }
    }

    fdesc_array[fd].rd_off+=len;
    return len;  // return the length of trully read data
}

int do_fwrite(int fd, char *buff, int length)
{
    // TODO [P6-task2]: Implement do_fwrite
    if(fd<0 || fd>=NUM_FDESCS || !fdesc_array[fd].valid){
        printk("Error: fd not valid!\n");
        return -1;
    }

    if(fdesc_array[fd].mode==O_RDONLY){
        printk("Error: Not writable!\n");
        return -1;
    }

    // read inode
    static inode_t tmp_inode;
    sd_read_inode(&tmp_inode, fdesc_array[fd].inode_id);

    // read off and len
    int off=fdesc_array[fd].wr_off;
    int len=length;
    // assert(off+len<=FS_DIRECT_INODE_ENTRY_NUM*FS_DATABLOCK_SIZE);

    // write
    int tmp_len=len;
    while(tmp_len>0){
        int datablock_no = off/FS_DATABLOCK_SIZE;
        // assert(datablock_no<FS_DIRECT_INODE_ENTRY_NUM);

        inode_entry_t *target_entry;
        if(datablock_no<FS_DIRECT_INODE_ENTRY_NUM){
            // direct
            target_entry = find_datablock(0, 0, tmp_inode.direct+datablock_no);
            sd_write_inode(&tmp_inode, tmp_inode.id);

            // the same as followings:
            // target_entry = tmp_inode.direct+datablock_no;
            // if(!target_entry->valid){
            //     target_entry->valid=1;
            //     target_entry->datablock_id=alloc_datablock();
            //     sd_write_inode(&tmp_inode, tmp_inode.id);
            // 
            //     static char blank[FS_DATABLOCK_SIZE];
            //     memset(blank, 0, FS_DATABLOCK_SIZE);
            //     sd_write_data(blank, target_entry->datablock_id);
            // }

            goto find;
        }
        
        datablock_no-=FS_DIRECT_INODE_ENTRY_NUM;
        if(datablock_no<FS_DBTABLE_ENTRY_NUM){
            // indirect
            target_entry = find_datablock(1, datablock_no, &tmp_inode.indirect);
            sd_write_inode(&tmp_inode, tmp_inode.id);

            goto find;
        }

        datablock_no-=FS_DBTABLE_ENTRY_NUM;
        if(datablock_no<FS_DBTABLE_ENTRY_NUM*FS_DBTABLE_ENTRY_NUM){
            // double indirect
            target_entry = find_datablock(2, datablock_no, &tmp_inode.double_indirect);
            sd_write_inode(&tmp_inode, tmp_inode.id);

            goto find;
        }

        datablock_no-=FS_DBTABLE_ENTRY_NUM*FS_DBTABLE_ENTRY_NUM;
        if(datablock_no<FS_DBTABLE_ENTRY_NUM*FS_DBTABLE_ENTRY_NUM*FS_DBTABLE_ENTRY_NUM){
            // triple indirect
            target_entry = find_datablock(3, datablock_no, &tmp_inode.triple_indirect);
            sd_write_inode(&tmp_inode, tmp_inode.id);

            goto find;
        }

        // file exceeds maximal size
        assert(0);

        static char tmp_datablock[FS_DATABLOCK_SIZE];
    find:    
        sd_read_data(tmp_datablock, target_entry->datablock_id);

        for(int i=off%FS_DATABLOCK_SIZE;i<FS_DATA_TOT_SIZE&&tmp_len>0;i++){
            tmp_datablock[i]=*(buff++);
            tmp_len--;
            off++;
        }

        sd_write_data(tmp_datablock, target_entry->datablock_id);
    }

    fdesc_array[fd].wr_off+=len;
    if(fdesc_array[fd].wr_off>tmp_inode.size){
        tmp_inode.size=fdesc_array[fd].wr_off;
    }

    sd_write_inode(&tmp_inode, tmp_inode.id);

    return len;  // return the length of trully written data
}

int do_fclose(int fd)
{
    // TODO [P6-task2]: Implement do_fclose
    if(fd>=0 && fd<NUM_FDESCS){
        fdesc_array[fd].valid=0;
        return 0;  // do_fclose succeeds
    }else{
        printk("Error: No such fd!\n");
        return -1;
    }
}

int do_ln(char *src_path, char *dst_path)
{
    // TODO [P6-task2]: Implement do_ln
    // parse path
    char *parsed_src_path[FS_DIR_MAX_LEVEL];
    int src_cnt=parse_path(src_path, parsed_src_path);
    char *parsed_dst_path[FS_DIR_MAX_LEVEL];
    int dst_cnt=parse_path(dst_path, parsed_dst_path);

    // grope path
    int src_inode_id = grope_path(parsed_src_path, src_cnt);
    if(src_inode_id==-1){
        return -1;
    }
    int dst_father_inode_id = grope_path(parsed_dst_path, dst_cnt-1);
    if(dst_father_inode_id==-1){
        return -1;
    }

    // read father inode
    static inode_t father_inode;
    sd_read_inode(&father_inode, dst_father_inode_id);
    if(father_inode.type!=DIR){
        printk("Error: %s not a dir!\n", parsed_dst_path[dst_cnt-2]);
        return -1;
    }

    // search in the dentries
    if(search_inode_from_inode(&father_inode, parsed_dst_path[dst_cnt-1])!=-1){
        // file/dir already exists
        printk("Error: %s already exists!\n", parsed_dst_path[dst_cnt-1]);
        return -1;
    }

    // no such file, start link
    static inode_t tmp_inode;
    sd_read_inode(&tmp_inode, src_inode_id);
    tmp_inode.nlinks++;
    sd_write_inode(&tmp_inode, src_inode_id);
    record_dentry_to_inode(&father_inode, parsed_dst_path[dst_cnt-1], src_inode_id);

    return 0;  // do_ln succeeds 
}

int do_rm(char *path)
{
    // TODO [P6-task2]: Implement do_rm
    // parse path
    char *parsed_path[FS_DIR_MAX_LEVEL];
    int cnt=parse_path(path, parsed_path);

    // grope path
    int father_inode_id = grope_path(parsed_path, cnt-1);
    if(father_inode_id==-1){
        return -1;
    }

    // read father inode
    static inode_t father_inode;
    sd_read_inode(&father_inode, father_inode_id);
    if(father_inode.type!=DIR){
        printk("Error: %s not a dir!\n", parsed_path[cnt-2]);
        return -1;
    }

    // search in the direct dentry
    int son_inode_id=search_inode_from_inode(&father_inode, parsed_path[cnt-1]);
    if(son_inode_id==-1){
        // no such file
        printk("Error: No file named %s!\n", parsed_path[cnt-1]);
        return -1;
    }

    // son inode exists
    static inode_t son_inode;
    sd_read_inode(&son_inode, son_inode_id);
    if(son_inode.type!=FILE){
        printk("Error: %s not a file!\n", parsed_path[cnt-1]);
        return -1;
    }

    // delete son inode in father dir
    delete_dentry_from_inode(&father_inode, parsed_path[cnt-1]);

    // decrease inode nlinks
    son_inode.nlinks--;
    sd_write_inode(&son_inode, son_inode_id);

    // delete son inode if nlinks comes to 0
    if(!son_inode.nlinks){
        free_all_datablocks_in_inode(&son_inode);
        free_inode(son_inode_id);
    }

    return 0;  // do_rm succeeds 
}

int do_lseek(int fd, int offset, int whence)
{
    // TODO [P6-task2]: Implement do_lseek
    if(fd<0 || fd>=NUM_FDESCS || !fdesc_array[fd].valid){
        printk("Error: fd not valid!\n");
        return -1;
    }

    if(whence==SEEK_SET){
        fdesc_array[fd].rd_off=offset;
        fdesc_array[fd].wr_off=offset;
    }else if(whence==SEEK_CUR){
        fdesc_array[fd].rd_off+=offset;
        fdesc_array[fd].wr_off+=offset;
    }else if(whence==SEEK_END){
        static inode_t tmp_inode;
        sd_read_inode(&tmp_inode, fdesc_array[fd].inode_id);
        fdesc_array[fd].rd_off=tmp_inode.size+offset;
        fdesc_array[fd].wr_off=tmp_inode.size+offset;
    }

    // the resulting offset location from the beginning of the file
    return fdesc_array[fd].wr_off;
}
