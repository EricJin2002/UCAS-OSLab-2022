#ifndef __INCLUDE_OS_FS_H__
#define __INCLUDE_OS_FS_H__

#include <type.h>
#include <os/mm.h>      // for [p6-task1]
#include <os/loader.h>  // for [p6-task1]
#include <common.h>     // for [p6-task1]
#include <pgtable.h>    // for [p6-task1]
#include <print_custom.h>

/* macros of file system */
#define SUPERBLOCK_MAGIC 0x20221205
#define NUM_FDESCS 16

/* data structures of file system */
typedef struct superblock_t{
    // TODO [P6-task1]: Implement the data structure of superblock
    uint32_t magic;
    int start_sector_id;
    int sector_num;

    int superblock_sector_offset;
    int superblock_sector_num;
    int blockmap_sector_offset;
    int blockmap_sector_num;
    int inodemap_sector_offset;
    int inodemap_sector_num;
    int inode_sector_offset;
    int inode_sector_num;
    int data_sector_offset;
    int data_sector_num;

    // count in bytes
    int inode_entry_size;
    int dir_entry_size;

    int root_inode_id;
} __attribute__((aligned(SECTOR_SIZE))) superblock_t;

// for [p6-task1]
#define FS_DATABLOCK_SIZE 4096
#define FS_DATABLOCK_SIZE_COUNT_IN_SECTORS (FS_DATABLOCK_SIZE/SECTOR_SIZE)
#define FS_DATA_TOT_SIZE (1<<30)
#define FS_DATABLOCK_NUM (FS_DATA_TOT_SIZE/FS_DATABLOCK_SIZE)
#define FS_INODE_NUM 4096   // must be times of SECTOR_SIZE*8

typedef struct dentry_t{
    // TODO [P6-task1]: Implement the data structure of directory entry
    int valid;
    char name[32];
    int inode_id;
} dentry_t;

// for [p6-task1]
#define FS_DENTRY_NUM 100
typedef struct dir_t{
    int cnt;
    dentry_t dentries[FS_DENTRY_NUM]
} __attribute__((aligned(FS_DATABLOCK_SIZE))) dir_t;

// for [p6-task1]
typedef struct inode_entry_t{
    int valid;
    int datablock_id;
} inode_entry_t;
#define FS_DIRECT_INODE_ENTRY_NUM 6

typedef struct inode_t{ 
    // TODO [P6-task1]: Implement the data structure of inode
    int id;
    int mode;
    int size;   // in bytes
    int nlinks;
    enum {FILE, DIR} type;

    // datablocks dir
    inode_entry_t direct[FS_DIRECT_INODE_ENTRY_NUM];
    inode_entry_t indirect;
    inode_entry_t double_indirect;
    inode_entry_t triple_indirect;
} __attribute__((aligned(SECTOR_SIZE))) inode_t;

// for [p6-task1]
int cwd_inode_id;
#define FS_DIR_MAX_LEVEL 10

typedef struct fdesc_t{
    // TODO [P6-task2]: Implement the data structure of file descriptor
    int valid;
    int mode;
    int inode_id;
    int rd_off;
    int wr_off;
} fdesc_t;

/* modes of do_fopen */
#define O_RDONLY 1  /* read only open */
#define O_WRONLY 2  /* write only open */
#define O_RDWR   3  /* read/write open */

/* whence of do_lseek */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// for [p6-task1]
extern void init_fs(void);

/* fs function declarations */
extern int do_mkfs(void);
extern int do_statfs(void);
extern int do_cd(char *path);
extern int do_mkdir(char *path);
extern int do_rmdir(char *path);
extern int do_ls(char *path, int option);
extern int do_touch(char *path);
extern int do_cat(char *path);
extern int do_fopen(char *path, int mode);
extern int do_fread(int fd, char *buff, int length);
extern int do_fwrite(int fd, char *buff, int length);
extern int do_fclose(int fd);
extern int do_ln(char *src_path, char *dst_path);
extern int do_rm(char *path);
extern int do_lseek(int fd, int offset, int whence);

// for [p6-task1]
superblock_t superblock;
fdesc_t fdesc_array[NUM_FDESCS];

extern int alloc_datablock();
extern int alloc_inode();
extern void free_inode(int id);
extern void free_datablock(int id);
extern void free_all_datablocks_in_inode(inode_t *inodeptr);
extern int map_dentry(dir_t *dirptr, char *name, int inode_id);
extern int parse_path(char *path, char **result);
extern int grope_path(char **parsed_path, int grope_level);
static inline void sd_read_inode(inode_t *buff, int inode_id){
    sd_read(kva2pa(buff), 1, 
        superblock.start_sector_id + superblock.inode_sector_offset + inode_id);
}
static inline void sd_write_inode(inode_t *buff, int inode_id){
    sd_write(kva2pa(buff), 1,
        superblock.start_sector_id + superblock.inode_sector_offset + inode_id);
}
static inline void sd_read_data(dir_t *buff, int datablock_id){
    sd_read(kva2pa(buff), FS_DATABLOCK_SIZE_COUNT_IN_SECTORS,
        superblock.start_sector_id + superblock.data_sector_offset +
        FS_DATABLOCK_SIZE_COUNT_IN_SECTORS * datablock_id);
}
static inline void sd_write_data(dir_t *buff, int datablock_id){
    sd_write(kva2pa(buff), FS_DATABLOCK_SIZE_COUNT_IN_SECTORS,
        superblock.start_sector_id + superblock.data_sector_offset +
        FS_DATABLOCK_SIZE_COUNT_IN_SECTORS * datablock_id);
}
extern int search_inode_from_inode(inode_t *father_inodeptr, char *name);
extern void delete_dentry_from_inode(inode_t *father_inodeptr, char *name);
extern void record_dentry_to_inode(inode_t *father_inodeptr, char *name, int inode_id);
extern int create_new_dir(int is_root, int father_inode_id);
extern int create_new_file(int father_inode_id);


#endif