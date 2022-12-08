#include <os/string.h>
#include <os/fs.h>
#include <common.h>     // for [p6-task1]
#include <pgtable.h>    // for [p6-task1]

static superblock_t superblock;
static fdesc_t fdesc_array[NUM_FDESCS];

// for [p6-task1]
int alloc_datablock(){
    static char blockmap_buff[SECTOR_SIZE];
    int ret=0;
    for(int i=0;i<superblock.blockmap_sector_num;i++){
        sd_read(kva2pa(blockmap_buff), 1, superblock.start_sector_id + superblock.blockmap_sector_offset+i);
        for(int j=0;j<SECTOR_SIZE;j++){
            if(blockmap_buff[j]!=(char)-1){
                char tmp=blockmap_buff[j];
                while(tmp&1){
                    tmp>>=1;
                    ret+=1;
                }
                blockmap_buff[j]|=1<<(ret%8);
                sd_write(kva2pa(blockmap_buff), 1, superblock.start_sector_id+superblock.blockmap_sector_offset+i);
                printl("[alloc_datablock] (%d)\n",ret);
                return ret;
            }
            ret+=8;
        }
    }
    return -1;
}

// for [p6-task1]
int alloc_inode(){
    static char inodemap_buff[SECTOR_SIZE];
    int ret=0;
    for(int i=0;i<superblock.inodemap_sector_num;i++){
        sd_read(kva2pa(inodemap_buff), 1, superblock.start_sector_id + superblock.inodemap_sector_offset+i);
        for(int j=0;j<SECTOR_SIZE;j++){
            if(inodemap_buff[j]!=(char)-1){
                char tmp=inodemap_buff[j];
                while(tmp&1){
                    tmp>>=1;
                    ret+=1;
                }
                inodemap_buff[j]|=1<<(ret%8);
                sd_write(kva2pa(inodemap_buff), 1, superblock.start_sector_id+superblock.inodemap_sector_offset+i);
                printl("[alloc_inode] (%d)\n",ret);
                return ret;
            }
            ret+=8;
        }
    }
    return -1;
}

// for [p6-task1]
int map_dentry(dir_t *dirptr, char *name, int inode_id){
    for(int i=0;i<FS_DENTRY_NUM;i++){
        if(!dirptr->dentries[i].valid){
            dirptr->cnt++;
            dirptr->dentries[i].valid=1;
            strcpy(dirptr->dentries[i].name, name);
            dirptr->dentries[i].inode_id=inode_id;
            printl("[map_dentry] (%s -> %d)\n", name, inode_id);
            return i;
        }
    }
    return -1;
}

// for [p6-task1]
int parse_path(char *path, char **result){
    int cnt=0;
    assert(path[0]!='/');
    result[cnt++]=path;
    for(int i=0;path[i];i++){
        if(path[i]=='/'){
            path[i]='\0';
            if(path[i+1]){  // path not end like ".../.../"
                result[cnt++]=path+i+1;
                assert(cnt<=FS_DIR_MAX_LEVEL);  // path is too long
            }
        }
    }
    return cnt;
}

// for [p6-task1]
// return inode id
int create_new_dir(int is_root, int father_inode_id){
    static inode_t tmp_inode;
    memset(&tmp_inode, 0, sizeof(inode_t));
    assert((tmp_inode.id=alloc_inode())!=-1);
    tmp_inode.mode=O_RDWR;
    tmp_inode.size=SECTOR_SIZE;
    tmp_inode.nlinks=1;
    tmp_inode.type=DIR;
    tmp_inode.direct[0].valid=1;
    tmp_inode.direct[0].datablock_id=alloc_datablock();
    sd_write(kva2pa(&tmp_inode), 1, 
        superblock.start_sector_id + superblock.inode_sector_offset + tmp_inode.id);
    
    static dir_t tmp_dir;
    memset(&tmp_dir, 0, sizeof(dir_t));
    map_dentry(&tmp_dir, ".", tmp_inode.id);
    if(is_root){
        map_dentry(&tmp_dir, "..", tmp_inode.id);
    }else{
        map_dentry(&tmp_dir, "..", father_inode_id);
    }
    sd_write(kva2pa(&tmp_dir), FS_DATABLOCK_SIZE_COUNT_IN_SECTORS, 
        superblock.start_sector_id + superblock.data_sector_offset + 
        FS_DATABLOCK_SIZE_COUNT_IN_SECTORS*tmp_inode.direct[0].datablock_id);

    printl("[create_new_dir] (%d . %d)\n", father_inode_id, tmp_inode.id);
    return tmp_inode.id;
}

// for [p6-task1]
void init_fs(void){
    int superblock_sector_id = swap_end_sector_id;
    assert(sizeof(superblock_t)==SECTOR_SIZE);
    sd_read(kva2pa(&superblock), 1, superblock_sector_id);

    do_mkfs();
}

int do_mkfs(void)
{
    // TODO [P6-task1]: Implement do_mkfs
    if(superblock.magic==SUPERBLOCK_MAGIC){
        printk("[FS] Filesystem already exists!\n");
        return -1;
    }

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
        sd_write(kva2pa(blank), 1, superblock.start_sector_id + superblock.inodemap_sector_offset+i);
    }

    printk("[FS] Setting block-map...\n");
    for(int i=0;i<superblock.blockmap_sector_num;i++){
        sd_write(kva2pa(blank), 1, superblock.start_sector_id + superblock.blockmap_sector_offset+i);
    }

    printk("[FS] Setting inode...\n");
    cwd_inode_id=create_new_dir(1,-1);
    
    printk("[FS] Initialize filesystem finished!\n");
    return 0;  // do_mkfs succeeds
}

int do_statfs(void)
{
    // TODO [P6-task1]: Implement do_statfs
    if(superblock.magic!=SUPERBLOCK_MAGIC){
        printk("> [FS] Filesystem not exists!");
        return -1;
    }

    printk("magic 0x%x\n", superblock.magic);
    printk("sector num %d   start sector id %d\n",superblock.sector_num, superblock.start_sector_id);
    printk("inode entry size %dB   dir entry size %dB\n",superblock.inode_entry_size, superblock.dir_entry_size);
    printk("[Sector Info]\n");
    printk("CONTENT    OFFSET NUM\n");
    printk("superblock %d      %d\n", superblock.superblock_sector_offset, superblock.superblock_sector_num);
    printk("blockmap   %d      %d\n", superblock.blockmap_sector_offset, superblock.blockmap_sector_num);
    printk("inodemap   %d     %d\n", superblock.inodemap_sector_offset, superblock.inodemap_sector_num);
    printk("inode      %d     %d\n", superblock.inode_sector_offset, superblock.inode_sector_num);
    printk("data       %d   %d\n", superblock.data_sector_offset, superblock.data_sector_num);
    
    return 0;  // do_statfs succeeds
}

int do_cd(char *path)
{
    // TODO [P6-task1]: Implement do_cd

    return 0;  // do_cd succeeds
}

int do_mkdir(char *path)
{
    // TODO [P6-task1]: Implement do_mkdir

    // parse path
    if(!path){
        path=".";
    }
    char *parsed_path[FS_DIR_MAX_LEVEL];  // note: this implies that the level of dir < 10
    int cnt=parse_path(path, parsed_path);
    printk("Path (");
    for(int i=0;i<cnt;i++){
        printk("%s/",parsed_path[i]);
    }
    printk(") is parsed!\n");


    int curr_inode_id=cwd_inode_id;
    int curr_parsed_path_idx=0;
    static inode_t tmp_inode;
    static dir_t tmp_dir;

    while(curr_parsed_path_idx<cnt){
        sd_read(kva2pa(&tmp_inode), 1,
            superblock.start_sector_id + superblock.inode_sector_offset + curr_inode_id);
        if(tmp_inode.type!=DIR){
            printk("Error: %s not a dir!\n", parsed_path[curr_parsed_path_idx-1]);
            return -1;
        }

        // search in the direct dentry
        for(int i=0;i<FS_DIRECT_INODE_ENTRY_NUM;i++){
            if(tmp_inode.direct[i].valid){
                sd_read(kva2pa(&tmp_dir), FS_DATABLOCK_SIZE_COUNT_IN_SECTORS,
                    superblock.start_sector_id + superblock.data_sector_offset +
                    FS_DATABLOCK_SIZE_COUNT_IN_SECTORS*tmp_inode.direct[i].datablock_id);
                int tmp_dir_cnt = tmp_dir.cnt;
                for(int i=0;i<FS_DENTRY_NUM&&tmp_dir_cnt>0;i++){
                    if(tmp_dir.dentries[i].valid){
                        tmp_dir_cnt--;
                        if(!strcmp(tmp_dir.dentries[i].name, parsed_path[curr_parsed_path_idx])){
                            curr_inode_id=tmp_dir.dentries[i].inode_id;
                            curr_parsed_path_idx++;
                            goto done;
                        }
                    }
                }
            }else{
                break;
            }
        }

        // no such dir, create one
        int newly_created_inode_id = create_new_dir(0, curr_inode_id);

        // record dentry in last dir
        for(int i=0;i<FS_DIRECT_INODE_ENTRY_NUM;i++){
            if(tmp_inode.direct[i].valid){
                sd_read(kva2pa(&tmp_dir), FS_DATABLOCK_SIZE_COUNT_IN_SECTORS,
                    superblock.start_sector_id + superblock.data_sector_offset +
                    FS_DATABLOCK_SIZE_COUNT_IN_SECTORS*tmp_inode.direct[i].datablock_id);
                
                if(map_dentry(&tmp_dir, parsed_path[curr_parsed_path_idx], newly_created_inode_id)==-1){
                    // this dir already full
                    continue;
                }

                sd_write(kva2pa(&tmp_dir), FS_DATABLOCK_SIZE_COUNT_IN_SECTORS,
                    superblock.start_sector_id + superblock.data_sector_offset +
                    FS_DATABLOCK_SIZE_COUNT_IN_SECTORS*tmp_inode.direct[i].datablock_id);
                
                curr_inode_id = newly_created_inode_id;
                curr_parsed_path_idx++;
                goto done;
            }else{
                tmp_inode.direct[i].valid=1;
                tmp_inode.direct[i].datablock_id=alloc_datablock();
                sd_write(kva2pa(&tmp_inode), 1,
                    superblock.start_sector_id + superblock.inode_sector_offset + tmp_inode.id);
                
                static dir_t new_dir;
                memset(&new_dir, 0, sizeof(dir_t));
                map_dentry(&new_dir, parsed_path[curr_parsed_path_idx], newly_created_inode_id);
                sd_write(kva2pa(&new_dir), FS_DATABLOCK_SIZE_COUNT_IN_SECTORS,
                    superblock.start_sector_id + superblock.data_sector_offset +
                    FS_DATABLOCK_SIZE_COUNT_IN_SECTORS*tmp_inode.direct[i].datablock_id);
                
                curr_inode_id = newly_created_inode_id;
                curr_parsed_path_idx++;
                goto done;
            }
        }

        assert(0);

        done:;
    }
    return 0;  // do_mkdir succeeds
}

int do_rmdir(char *path)
{
    // TODO [P6-task1]: Implement do_rmdir

    return 0;  // do_rmdir succeeds
}

int do_ls(char *path, int option)
{
    // TODO [P6-task1]: Implement do_ls
    // Note: argument 'option' serves for 'ls -l' in A-core
    if(!path){
        path=".";
    }
    char *parsed_path[FS_DIR_MAX_LEVEL];  // note: this implies that the level of dir < 10
    int cnt=parse_path(path, parsed_path);
    printk("Path (");
    for(int i=0;i<cnt;i++){
        printk("%s/",parsed_path[i]);
    }
    printk(") is parsed!\n");

    
    int curr_inode_id=cwd_inode_id;
    int curr_parsed_path_idx=0;
    static inode_t tmp_inode;
    static dir_t tmp_dir;

    while(curr_parsed_path_idx<cnt){
        sd_read(kva2pa(&tmp_inode), 1,
            superblock.start_sector_id + superblock.inode_sector_offset + curr_inode_id);
        if(tmp_inode.type!=DIR){
            printk("Error: %s not a dir!\n", parsed_path[curr_parsed_path_idx-1]);
            return -1;
        }

        // search in the direct dentry
        for(int i=0;i<FS_DIRECT_INODE_ENTRY_NUM;i++){
            if(tmp_inode.direct[i].valid){
                sd_read(kva2pa(&tmp_dir), FS_DATABLOCK_SIZE_COUNT_IN_SECTORS,
                    superblock.start_sector_id + superblock.data_sector_offset +
                    FS_DATABLOCK_SIZE_COUNT_IN_SECTORS*tmp_inode.direct[i].datablock_id);
                int tmp_dir_cnt = tmp_dir.cnt;
                for(int i=0;i<FS_DENTRY_NUM&&tmp_dir_cnt>0;i++){
                    if(tmp_dir.dentries[i].valid){
                        tmp_dir_cnt--;
                        if(!strcmp(tmp_dir.dentries[i].name, parsed_path[curr_parsed_path_idx])){
                            curr_inode_id=tmp_dir.dentries[i].inode_id;
                            curr_parsed_path_idx++;
                            goto done;
                        }
                    }
                }
            }else{
                break;
            }
        }

        printk("Error: no such dir!\n");
        return -1;

        done:;
    }

    sd_read(kva2pa(&tmp_inode), 1,
        superblock.start_sector_id + superblock.inode_sector_offset + curr_inode_id);
    if(tmp_inode.type!=DIR){
        printk("Error: %s not a dir!\n", parsed_path[curr_parsed_path_idx-1]);
        return -1;
    }

    // search in the direct dentry
    for(int i=0;i<FS_DIRECT_INODE_ENTRY_NUM;i++){
        if(tmp_inode.direct[i].valid){
            sd_read(kva2pa(&tmp_dir), FS_DATABLOCK_SIZE_COUNT_IN_SECTORS,
                superblock.start_sector_id + superblock.data_sector_offset +
                FS_DATABLOCK_SIZE_COUNT_IN_SECTORS*tmp_inode.direct[i].datablock_id);
            int tmp_dir_cnt = tmp_dir.cnt;
            for(int i=0;i<FS_DENTRY_NUM&&tmp_dir_cnt>0;i++){
                if(tmp_dir.dentries[i].valid){
                    tmp_dir_cnt--;
                    printk("%s ",tmp_dir.dentries[i].name);
                }
            }
        }else{
            break;
        }
    }
    printk("\n");

    return 0;  // do_ls succeeds
}

int do_touch(char *path)
{
    // TODO [P6-task2]: Implement do_touch

    return 0;  // do_touch succeeds
}

int do_cat(char *path)
{
    // TODO [P6-task2]: Implement do_cat

    return 0;  // do_cat succeeds
}

int do_fopen(char *path, int mode)
{
    // TODO [P6-task2]: Implement do_fopen

    return 0;  // return the id of file descriptor
}

int do_fread(int fd, char *buff, int length)
{
    // TODO [P6-task2]: Implement do_fread

    return 0;  // return the length of trully read data
}

int do_fwrite(int fd, char *buff, int length)
{
    // TODO [P6-task2]: Implement do_fwrite

    return 0;  // return the length of trully written data
}

int do_fclose(int fd)
{
    // TODO [P6-task2]: Implement do_fclose

    return 0;  // do_fclose succeeds
}

int do_ln(char *src_path, char *dst_path)
{
    // TODO [P6-task2]: Implement do_ln

    return 0;  // do_ln succeeds 
}

int do_rm(char *path)
{
    // TODO [P6-task2]: Implement do_rm

    return 0;  // do_rm succeeds 
}

int do_lseek(int fd, int offset, int whence)
{
    // TODO [P6-task2]: Implement do_lseek

    return 0;  // the resulting offset location from the beginning of the file
}
