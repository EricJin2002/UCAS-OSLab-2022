#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <dirent.h> // for [p1-task5] to read batch file under certain dir

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512
#define BOOT_LOADER_SIG_OFFSET 0x1fe
#define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
#define BOOT_LOADER_SIG_1 0x55
#define BOOT_LOADER_SIG_2 0xaa

#define NBYTES2SEC(nbytes) (((nbytes) / SECTOR_SIZE) + ((nbytes) % SECTOR_SIZE != 0))

// for [p1-task5]
typedef enum{
    app, bat
} TYPE;

/* TODO: [p1-task4] design your own task_info_t */
typedef struct {
    char name[32];
    int offset;
    int size;
    uint64_t entrypoint;
    TYPE type; // used to judge app or bat for [p1-task5]
} task_info_t;

#define TASK_MAXNUM 30
static task_info_t taskinfo[TASK_MAXNUM];

/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr *ehdr, FILE *fp);
static void read_phdr(Elf64_Phdr *phdr, FILE *fp, int ph, Elf64_Ehdr ehdr);
static uint64_t get_entrypoint(Elf64_Ehdr ehdr);
static uint32_t get_filesz(Elf64_Phdr phdr);
static uint32_t get_memsz(Elf64_Phdr phdr);
static void write_segment(Elf64_Phdr phdr, FILE *fp, FILE *img, int *phyaddr);
static void write_padding(FILE *img, int *phyaddr, int new_phyaddr);
static void write_img_info(int nbytes_kernel, task_info_t *taskinfo,
                           short tasknum, FILE *img);

int main(int argc, char **argv)
{
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
        char *option = &argv[1][2];

        if (strcmp(option, "vm") == 0) {
            options.vm = 1;
        } else if (strcmp(option, "extended") == 0) {
            options.extended = 1;
        } else {
            error("%s: invalid option\nusage: %s %s\n", progname,
                  progname, ARGS);
        }
        argc--;
        argv++;
    }
    if (options.vm == 1) {
        error("%s: option --vm not implemented\n", progname);
    }
    if (argc < 3) {
        /* at least 3 args (createimage bootblock main) */
        error("usage: %s %s\n", progname, ARGS);
    }
    create_image(argc - 1, argv + 1);
    return 0;
}

/* TODO: [p1-task4] assign your task_info_t somewhere in 'create_image' */
static void create_image(int nfiles, char *files[])
{
    int tasknum = nfiles - 2;
    int nbytes_kernel = 0;
    int phyaddr = 0;
    FILE *fp = NULL, *img = NULL;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;

    /* open the image file */
    img = fopen(IMAGE_FILE, "w");
    assert(img != NULL);

    /* for each input file */
    for (int fidx = 0; fidx < nfiles; ++fidx) {

        int taskidx = fidx - 2;

        /* open input file */
        fp = fopen(*files, "r");
        assert(fp != NULL);

        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("0x%04lx: %s\n", ehdr.e_entry, *files);

        // for [p1-task4]
        // record task offset and name
        if(taskidx >= 0){
            taskinfo[taskidx].offset = phyaddr;
            strcpy(taskinfo[taskidx].name, *files);
            taskinfo[taskidx].entrypoint = get_entrypoint(ehdr);
            taskinfo[taskidx].type = app;
        }

        /* for each program header */
        for (int ph = 0; ph < ehdr.e_phnum; ph++) {

            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);

            /* write segment to the image */
            write_segment(phdr, fp, img, &phyaddr);

            /* update nbytes_kernel */
            if (strcmp(*files, "main") == 0) {
                nbytes_kernel += get_filesz(phdr);
            }
        }

        // for [p1-task4]
        // record task size
        if(taskidx >= 0){
            taskinfo[taskidx].size = phyaddr - taskinfo[taskidx].offset;
            printf("taskidx %d\tname %s\toffset %#x\tsize %#x\n", taskidx, taskinfo[taskidx].name, taskinfo[taskidx].offset, taskinfo[taskidx].size);
        }

        /* write padding bytes */
        /**
         * TODO:
         * 1. [p1-task3] do padding so that the kernel and every app program
         *  occupies the same number of sectors
         * 2. [p1-task4] only padding bootblock is allowed!
         */
        if (strcmp(*files, "bootblock") == 0) {
            write_padding(img, &phyaddr, SECTOR_SIZE);
        } 

        // for [p1-task3]
        // pad for kernel and apps
        /* else{
         *     int program_size = 15 * SECTOR_SIZE;
         *     int new_phyaddr =  (
         *         (phyaddr - SECTOR_SIZE) / program_size 
         *         + ((phyaddr - SECTOR_SIZE) % program_size != 0)
         *     ) * program_size + SECTOR_SIZE;
         *     write_padding(img, &phyaddr, new_phyaddr);
         * }
         */

        fclose(fp);
        files++;
    }
    write_img_info(nbytes_kernel, taskinfo, tasknum, img);

    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    int ret;

    ret = fread(ehdr, sizeof(*ehdr), 1, fp);
    assert(ret == 1);
    assert(ehdr->e_ident[EI_MAG1] == 'E');
    assert(ehdr->e_ident[EI_MAG2] == 'L');
    assert(ehdr->e_ident[EI_MAG3] == 'F');
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    int ret;

    fseek(fp, ehdr.e_phoff + ph * ehdr.e_phentsize, SEEK_SET);
    ret = fread(phdr, sizeof(*phdr), 1, fp);
    assert(ret == 1);
    if (options.extended == 1) {
        printf("\tsegment %d\n", ph);
        printf("\t\toffset 0x%04lx", phdr->p_offset);
        printf("\t\tvaddr 0x%04lx\n", phdr->p_vaddr);
        printf("\t\tfilesz 0x%04lx", phdr->p_filesz);
        printf("\t\tmemsz 0x%04lx\n", phdr->p_memsz);
    }
}

static uint64_t get_entrypoint(Elf64_Ehdr ehdr)
{
    return ehdr.e_entry;
}

static uint32_t get_filesz(Elf64_Phdr phdr)
{
    return phdr.p_filesz;
}

static uint32_t get_memsz(Elf64_Phdr phdr)
{
    return phdr.p_memsz;
}

static void write_segment(Elf64_Phdr phdr, FILE *fp, FILE *img, int *phyaddr)
{
    if (phdr.p_memsz != 0 && phdr.p_type == PT_LOAD) {
        /* write the segment itself */
        /* NOTE: expansion of .bss should be done by kernel or runtime env! */
        if (options.extended == 1) {
            printf("\t\twriting 0x%04lx bytes\n", phdr.p_filesz);
        }
        fseek(fp, phdr.p_offset, SEEK_SET);
        while (phdr.p_filesz-- > 0) {
            fputc(fgetc(fp), img);
            (*phyaddr)++;
        }
    }
}

static void write_padding(FILE *img, int *phyaddr, int new_phyaddr)
{
    if (options.extended == 1 && *phyaddr < new_phyaddr) {
        printf("\t\twrite 0x%04x bytes for padding\n", new_phyaddr - *phyaddr);
    }

    while (*phyaddr < new_phyaddr) {
        fputc(0, img);
        (*phyaddr)++;
    }
}

static void write_img_info(int nbytes_kern, task_info_t *taskinfo,
                           short tasknum, FILE * img)
{
    // TODO: [p1-task3] & [p1-task4] write image info to some certain places
    // NOTE: os size, infomation about app-info sector(s) ...

    // for [p1-task5]
    // record batch info and save batch content
    DIR *dir = opendir("../bat");
    if(!dir){
        printf("Failed to open bat dir!\n");
    }else{
        struct dirent *dirp;
        while((dirp=readdir(dir))){
            int file_ext_off = strlen(dirp->d_name) - 4;
            if(file_ext_off>=0 && !strcmp(dirp->d_name + file_ext_off, ".txt")){
                // record batch name
                printf("Reading %s ",dirp->d_name);
                strncpy(taskinfo[tasknum].name, dirp->d_name, file_ext_off);
                taskinfo[tasknum].name[file_ext_off] = '\0';
                printf("as %s ", taskinfo[tasknum].name);
                char dir_name[40] = "../bat/";
                strcat(dir_name, dirp->d_name);
                printf("from %s\n", dir_name);

                // save bat/*.txt in image
                uint32_t bat_off = ftell(img);
                uint32_t bat_size = 0;
                FILE *bat_fp = fopen(dir_name, "r");
                char ch_b;
                while((ch_b=fgetc(bat_fp))!=EOF){
                    bat_size++;
                    fputc(ch_b,img);
                }
                fputc('\0',img);
                bat_size++;
                fclose(bat_fp);

                // record other batch info
                taskinfo[tasknum].size = bat_size;
                taskinfo[tasknum].offset = bat_off;
                taskinfo[tasknum].type = bat;
                printf("taskidx %d\tname %s\toffset %#x\tsize %#x\n", tasknum, taskinfo[tasknum].name, taskinfo[tasknum].offset, taskinfo[tasknum].size);

                tasknum++;
            }
        }
        closedir(dir);
    }

    // save task-info for [p1-task4]
    fwrite(taskinfo, sizeof(task_info_t), TASK_MAXNUM, img);
    
    // abandoned implementation
    // save bat.txt in image for [p1-task5]
    // first record batch size, then save content
    /* uint32_t bat_size_off = ftell(img);
     * uint32_t bat_size = 0;
     * fwrite(&bat_size, 4, 1, img); // place holder
     * FILE *bat = fopen("../bat.txt", "r");
     * char ch_b;
     * while((ch_b=fgetc(bat))!=EOF){
     *     bat_size++;
     *     fputc(ch_b,img);
     * }
     * fputc('\0',img);
     * bat_size++;
     * fseek(img, bat_size_off, SEEK_SET);
     * fwrite(&bat_size, 4, 1, img);
     */

    // save kernel sector num and task num for [p1-task3] & [p1-task4]
    uint16_t kernel_sector_num = NBYTES2SEC(nbytes_kern);
    fseek(img, OS_SIZE_LOC, SEEK_SET);
    fwrite(&kernel_sector_num, 2, 1, img);
    fwrite(&tasknum, 2, 1, img);
    
    // save task-info offset and block id and block num for [p1-task4]
    uint32_t task_info_off = taskinfo[tasknum-1].offset + taskinfo[tasknum-1].size;
    uint16_t task_info_block_id = task_info_off/SECTOR_SIZE;
    uint16_t task_info_block_num = NBYTES2SEC(task_info_off%SECTOR_SIZE + sizeof(task_info_t)*TASK_MAXNUM);
    printf("task-info offset %#x\n", task_info_off);
    printf("task-info size %#lx\n", sizeof(task_info_t)*TASK_MAXNUM);
    printf("task-info block id %d\n", task_info_block_id);
    printf("task-info blcok num %d\n", task_info_block_num);
    fseek(img, OS_SIZE_LOC-8, SEEK_SET);
    fwrite(&task_info_off, 4, 1, img);
    fwrite(&task_info_block_id, 2, 1, img);
    fwrite(&task_info_block_num, 2, 1, img);
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}
