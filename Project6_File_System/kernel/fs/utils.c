#include <os/string.h>
#include <os/fs.h>

// for [p6-task1]
int alloc_datablock(){
    static char blockmap_buff[SECTOR_SIZE];
    int ret=0;
    for(int i=0;i<superblock.blockmap_sector_num;i++){
        sd_read(kva2pa(blockmap_buff), 1, superblock.start_sector_id + superblock.blockmap_sector_offset + i);
        for(int j=0;j<SECTOR_SIZE;j++){
            if(blockmap_buff[j]!=(char)-1){
                char tmp=blockmap_buff[j];
                while(tmp&1){
                    tmp>>=1;
                    ret+=1;
                }
                blockmap_buff[j]|=1<<(ret%8);
                sd_write(kva2pa(blockmap_buff), 1, superblock.start_sector_id + superblock.blockmap_sector_offset + i);
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
        sd_read(kva2pa(inodemap_buff), 1, superblock.start_sector_id + superblock.inodemap_sector_offset + i);
        for(int j=0;j<SECTOR_SIZE;j++){
            if(inodemap_buff[j]!=(char)-1){
                char tmp=inodemap_buff[j];
                while(tmp&1){
                    tmp>>=1;
                    ret+=1;
                }
                inodemap_buff[j]|=1<<(ret%8);
                sd_write(kva2pa(inodemap_buff), 1, superblock.start_sector_id + superblock.inodemap_sector_offset + i);
                printl("[alloc_inode] (%d)\n",ret);
                return ret;
            }
            ret+=8;
        }
    }
    return -1;
}

// for [p6-task1]
void free_inode(int id){
    int inodemap_sector_idx = id/8/SECTOR_SIZE;
    int inodemap_buff_idx = (id%(8*SECTOR_SIZE))/8;
    int inodemap_byte_idx = id%8;
    
    static char inodemap_buff[SECTOR_SIZE];
    sd_read(kva2pa(inodemap_buff), 1, 
        superblock.start_sector_id + superblock.inodemap_sector_offset + inodemap_sector_idx);
    inodemap_buff[inodemap_buff_idx] &=~(1<<inodemap_byte_idx);
    sd_write(kva2pa(inodemap_buff), 1,
        superblock.start_sector_id + superblock.inodemap_sector_offset + inodemap_sector_idx);
}

// for [p6-task1]
void free_datablock(int id){
    int blockmap_sector_idx = id/8/SECTOR_SIZE;
    int blockmap_buff_idx = (id%(8*SECTOR_SIZE))/8;
    int blockmap_byte_idx = id%8;
    
    static char blockmap_buff[SECTOR_SIZE];
    sd_read(kva2pa(blockmap_buff), 1, 
        superblock.start_sector_id + superblock.blockmap_sector_offset + blockmap_sector_idx);
    blockmap_buff[blockmap_buff_idx] &=~(1<<blockmap_byte_idx);
    sd_write(kva2pa(blockmap_buff), 1,
        superblock.start_sector_id + superblock.blockmap_sector_offset + blockmap_sector_idx);
}

// for [p6-task1]
void free_all_datablocks_in_inode(inode_t *inodeptr){
    for(int i=0;i<FS_DIRECT_INODE_ENTRY_NUM;i++){
        if(inodeptr->direct[i].valid){
            inodeptr->direct[i].valid=0;
            free_datablock(inodeptr->direct[i].datablock_id);
        }
    }

    // todo: free indirect datablocks
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
    // fixme: path will be modified in being parsed
    if(!path){
        path=".";
    }
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
// on success, return inode id; else, return -1
int grope_path(char **parsed_path, int grope_level){
    int curr_inode_id=cwd_inode_id;
    int curr_parsed_path_idx=0;
    static inode_t tmp_inode;

    while(curr_parsed_path_idx<grope_level){
        // read father inode
        sd_read_inode(&tmp_inode, curr_inode_id);
        if(tmp_inode.type!=DIR){
            printk("Error: %s not a dir!\n", parsed_path[curr_parsed_path_idx-1]);
            return -1;
        }

        // search in the direct dentry
        int tmp_curr_inode_id=search_inode_from_inode(&tmp_inode, parsed_path[curr_parsed_path_idx]);
        if(tmp_curr_inode_id!=-1){
            // son inode exists
            curr_inode_id=tmp_curr_inode_id;
            curr_parsed_path_idx++;
        }else{
            // no such dir
            printk("Error: No dir named %s!\n", parsed_path[curr_parsed_path_idx]);
            return -1;
        }
    }
    return curr_inode_id;
}

// for [p6-task1]
// search in the direct dentries
// if exists, return inode_id; else, return -1
int search_inode_from_inode(inode_t *father_inodeptr, char *name){
    static dir_t tmp_dir;
    for(int i=0;i<FS_DIRECT_INODE_ENTRY_NUM;i++){
        if(father_inodeptr->direct[i].valid){
            sd_read_data(&tmp_dir, father_inodeptr->direct[i].datablock_id);
            int tmp_dir_cnt = tmp_dir.cnt;
            for(int j=0;j<FS_DENTRY_NUM&&tmp_dir_cnt>0;j++){
                if(tmp_dir.dentries[j].valid){
                    tmp_dir_cnt--;
                    if(!strcmp(tmp_dir.dentries[j].name, name)){
                        return tmp_dir.dentries[j].inode_id;
                    }
                }
            }
        }else{
            break;
        }
    }
    return -1;
}

// for [p6-task1]
// only for direct dentries
void delete_dentry_from_inode(inode_t *father_inodeptr, char *name){
    static dir_t tmp_dir;
    for(int i=0;i<FS_DIRECT_INODE_ENTRY_NUM;i++){
        if(father_inodeptr->direct[i].valid){
            sd_read_data(&tmp_dir, father_inodeptr->direct[i].datablock_id);
            int tmp_dir_cnt = tmp_dir.cnt;
            for(int j=0;j<FS_DENTRY_NUM&&tmp_dir_cnt>0;j++){
                if(tmp_dir.dentries[j].valid){
                    tmp_dir_cnt--;
                    if(!strcmp(tmp_dir.dentries[j].name, name)){
                        tmp_dir.dentries[j].valid=0;
                        tmp_dir.cnt--;
                    }
                }
            }
            sd_write_data(&tmp_dir, father_inodeptr->direct[i].datablock_id);
        }else{
            break;
        }
    }
}

// for [p6-task1]
// only for direct dentries
void record_dentry_to_inode(inode_t *father_inodeptr, char *name, int inode_id){
    static dir_t tmp_dir;
    for(int i=0;i<FS_DIRECT_INODE_ENTRY_NUM;i++){
        if(father_inodeptr->direct[i].valid){
            sd_read_data(&tmp_dir, father_inodeptr->direct[i].datablock_id);
            
            if(map_dentry(&tmp_dir, name, inode_id)==-1){
                // this dir already full
                continue;
            }

            sd_write_data(&tmp_dir, father_inodeptr->direct[i].datablock_id);
            return;   // do_mkdir succeeds
        }else{
            father_inodeptr->size+=FS_DATABLOCK_SIZE;
            father_inodeptr->direct[i].valid=1;
            father_inodeptr->direct[i].datablock_id=alloc_datablock();
            sd_write_inode(father_inodeptr, father_inodeptr->id);

            static dir_t new_dir;
            memset(&new_dir, 0, sizeof(dir_t));
            map_dentry(&new_dir, name, inode_id);
            sd_write_data(&new_dir, father_inodeptr->direct[i].datablock_id);
            return;   // do_mkdir succeeds
        }
    }
    assert(0);
}

// for [p6-task1]
// return inode id
int create_new_dir(int is_root, int father_inode_id){
    static inode_t tmp_inode;
    memset(&tmp_inode, 0, sizeof(inode_t));
    assert((tmp_inode.id=alloc_inode())!=-1);
    tmp_inode.mode=O_RDWR;
    tmp_inode.size=FS_DATABLOCK_SIZE;
    tmp_inode.nlinks=1;
    tmp_inode.type=DIR;
    tmp_inode.direct[0].valid=1;
    tmp_inode.direct[0].datablock_id=alloc_datablock();
    sd_write_inode(&tmp_inode, tmp_inode.id);
    
    static dir_t tmp_dir;
    memset(&tmp_dir, 0, sizeof(dir_t));
    map_dentry(&tmp_dir, ".", tmp_inode.id);
    if(is_root){
        map_dentry(&tmp_dir, "..", tmp_inode.id);
    }else{
        map_dentry(&tmp_dir, "..", father_inode_id);
    }
    sd_write_data(&tmp_dir, tmp_inode.direct[0].datablock_id);

    printl("[create_new_dir] (%d . %d)\n", father_inode_id, tmp_inode.id);
    return tmp_inode.id;
}

// for [p6-task1]
int create_new_file(int father_inode_id){
    static inode_t tmp_inode;
    memset(&tmp_inode, 0, sizeof(inode_t));
    assert((tmp_inode.id=alloc_inode())!=-1);
    tmp_inode.mode=O_RDWR;
    tmp_inode.size=0;
    tmp_inode.nlinks=1;
    tmp_inode.type=FILE;
    sd_write_inode(&tmp_inode, tmp_inode.id);

    printl("[create_new_file] (%d . %d)\n", father_inode_id, tmp_inode.id);
    return tmp_inode.id;
}

static int pow(int x, int a){
    int ret = 1;
    for(int i=0;i<a;i++){
        ret*=x;
    }
    return ret;
}

// for [p6-task1]
// find and return datablock entry from the input entry
// if entry invalid, create new datablock to make it valid
// the caller MUST write back the input entry to the disk!
inode_entry_t *find_datablock(int indirect_level, int datablock_no, inode_entry_t *entry){
    if(!entry->valid){
        entry->valid=1;
        assert((entry->datablock_id=alloc_datablock())!=-1);
        static char blank[FS_DATABLOCK_SIZE];
        memset(blank, 0, FS_DATABLOCK_SIZE);
        sd_write_data(blank, entry->datablock_id);
    }

    if(!indirect_level){
        return entry;
    }else{
        int datablock_this_no, datablock_next_no;
        datablock_this_no = datablock_no / pow(FS_DBTABLE_ENTRY_NUM, indirect_level-1);
        datablock_next_no = datablock_no % pow(FS_DBTABLE_ENTRY_NUM, indirect_level-1);

        // create at least 3 buff for nested calling
        static dbtable_t datablock_table_ptr[4];
        sd_read_data(datablock_table_ptr[indirect_level], entry->datablock_id);
        inode_entry_t *ret_entry = find_datablock(
            indirect_level-1, 
            datablock_next_no, 
            &datablock_table_ptr[indirect_level][datablock_this_no]
        );
        sd_write_data(datablock_table_ptr[indirect_level], entry->datablock_id);

        return ret_entry;
    }
}

// for [p6-task1]
void print_datablock(int indirect_level, inode_entry_t *entry){
    if(!entry->valid){
        return;
    }

    static dbtable_t datablock_table_ptr[4];
    sd_read_data(datablock_table_ptr[indirect_level], entry->datablock_id);
    
    if(!indirect_level){
        // buf is for a better print speed
        static char print_buf[FS_DATABLOCK_SIZE];
        int head=0;
        for(int i=0;i<FS_DATABLOCK_SIZE;i++){
            // printk("%c",((char *)datablock_table_ptr[0])[i]);
            if(((char *)datablock_table_ptr[0])[i]){
                print_buf[head++]=((char *)datablock_table_ptr[0])[i];
            }
        }
        print_buf[head]='\0';
        printk("%s",print_buf);
    }else{
        for(int i=0;i<FS_DBTABLE_ENTRY_NUM;i++){
            print_datablock(indirect_level-1, &datablock_table_ptr[indirect_level][i]);
        }
    }
}

// for [p6-task3]
void print_datablock_to_buf(int indirect_level, inode_entry_t *entry, char **bufptr){
    // copied from print_datablock

    if(!entry->valid){
        return;
    }

    static dbtable_t datablock_table_ptr[4];
    sd_read_data(datablock_table_ptr[indirect_level], entry->datablock_id);
    
    if(!indirect_level){
        for(int i=0;i<FS_DATABLOCK_SIZE;i++){
            if(((char *)datablock_table_ptr[0])[i]){
                *((*bufptr)++)=((char *)datablock_table_ptr[0])[i];
            }
        }
    }else{
        for(int i=0;i<FS_DBTABLE_ENTRY_NUM;i++){
            print_datablock_to_buf(indirect_level-1, &datablock_table_ptr[indirect_level][i], bufptr);
        }
    }
}