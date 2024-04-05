#include "system_call.h"
#include "lib.h"

//TODO CP2

int32_t halt(uint8_t status){
    // int i;

    pcb_t* pcb = current_pcb();
    
    /* clear file desc */
    
    pcb->present = 0;

    page_directory[USER_ENTRY].MB.present = 0;
    page_directory[USER_ENTRY].MB.user_supervisor = 0;
    page_directory[USER_ENTRY].MB.page_base_address = USER_ENTRY;

    return -1;
}

int32_t execute(const uint8_t* command){
    int i, pid;
    uint8_t filename[READBUF_SIZE] = {0};   /* file name */
    uint8_t args[READBUF_SIZE] = {0};       /* arguments */
    dentry_t exec_dentry;           /* location of file name */
    uint32_t magic_check;           /* exec format check */
    uint32_t eip;                   /* instruction ptr */
    pcb_t* pcb;

    /* **************************************************
     * *                 Parse Argument                 *
     * **************************************************/
    /* filters spaces before file name */
    for (i = 0; i < READBUF_SIZE; i++)
        if (!command[i])
            return -1;
        else if (command[i] != ' ')
            break;

    command += i;

    /* parses the file name */
    for (i = 0; i < READBUF_SIZE; i++)
        if (!command[i] || command[i] == ' ')
            break;
        else
            filename[i] = command[i];

    command += i;
    
    /* filters the space before arguments */
    for (i = 0; i < READBUF_SIZE; i++)
        if (!command[i] ||command[i] != ' ')
            break;

    command += i;

    /* parses the arguments */
    for (i = 0; i < READBUF_SIZE; i++)
        if (!command[i])
            break;
        else
            args[i] = command[i];
    
    /* **************************************************
     * *              Check File Validity               *
     * **************************************************/
    /*
     * checks the
     *   - existence of the file
     *   - size of the file
     *   - magic header of executable
     */
    if (read_dentry_by_name(filename, &exec_dentry) == -1
        || read_data(exec_dentry.inode_num, 0, (uint8_t*)&magic_check, MAGIC_SIZE) == -1
        || magic_check != MAGIC_NUM)
        return -1;

    /* gets the index of the new process */
    for (pid = 0; pid < MAX_TASKS; pid++)
        if (!(GET_PCB(pid)->present))
            break;

    if (pid == MAX_TASKS) /* cannot handle it */
        return -1;


    /* **************************************************
     * *                  Setup Paging                  *
     * **************************************************/
    page_directory[USER_ENTRY].MB.present = 1;              /* 128MB -> user program */
    page_directory[USER_ENTRY].MB.user_supervisor = 1;      /* user can access */
    page_directory[USER_ENTRY].val |= 0x400000 + pid * 0x400000; /* physical address: 0x400000 & 0x800000 */

    asm volatile (                  /* rewrite cr3 (pde[]) to empty tlb */
        "movl %%cr3, %%eax"
        "movl %%eax, %%cr3"
        :
        :
        : "%eax"
    );

    /* **************************************************
     * *             Load File into Memory              *
     * **************************************************/
    /* loads the program image */
    read_data(exec_dentry.inode_num, 0, (uint8_t*)PROGRAM_IMAGE_ADDR, PROGRAM_IMAGE_LIMIT);

    /* **************************************************
     * *              Create PCB & File OP              *
     * **************************************************/
    /* creates the pcb */
    pcb = GET_PCB(pid);
    if (pid) /* if the created pid is not shell, then we assign parent */
        pcb->parent_pcb = current_pcb();
    pcb->present = 1;
    pcb->pid = pid;
    
    /* setup stdin and stdout*/
    pcb->fd[0].file_ops = &stdin_op;
    pcb->fd[0].flags = 1;
    pcb->fd[1].file_ops = &stdout_op;
    pcb->fd[1].flags = 1;

    /* setup remaining fileop */
    for (i = 2; i < MAX_FILES; i++)
        pcb->fd[i].file_ops = &null_op;

    memcpy(pcb->args, args, READBUF_SIZE); /* assign pcb->args */
    
    tss.esp0 = KSTACK_START - KSTACK_SIZE * pid;
    read_data(exec_dentry.inode_num, 24, (uint8_t*)&eip, 4);


    /* **************************************************
     * *           Prepare for Context Switch           *
     * **************************************************/
    /* Create its own context switch stack */
    asm volatile (
        "movw %0, %%ds"  /* ds = USER_DS */
        "pushl %0"      /* USER_DS */
        "pushl %1"      /* USER_STACK */
        "pushfl"        /* eflags */
        "pushl %2"      /* USER_CS */
        "pushl %3"      /* eip */
        "iret"
        :
        :"r"((uint32_t)USER_DS), "r"((uint32_t)USER_STACK), "r"((uint32_t)USER_CS), "r"(eip)
        :"memory"
    );
    return 0;
}

int32_t read(int32_t fd, void* buf, int32_t nbytes){
    pcb_t* curr_pcb = current_pcb();
    uint32_t result;
    if(fd < 0 || fd >= MAX_FILES || curr_pcb->fd[fd].flags==0) return -1;
    result = curr_pcb->fd[fd].file_ops->read(fd, buf, nbytes);
    curr_pcb->fd[fd].file_position += result;
    return result;
}

int32_t write(int32_t fd, const void* buf, int32_t nbytes){
    pcb_t* curr_pcb = current_pcb();
    if(fd < 0 || fd >= MAX_FILES || curr_pcb->fd[fd].flags==0) return -1;
    return curr_pcb->fd[fd].file_ops->write(fd, buf, nbytes);
}

int32_t open(const uint8_t* filename){
    int i;
    dentry_t dentry;
    pcb_t* curr_pcb = current_pcb();

    if(filename == NULL || read_dentry_by_name(filename, &dentry) == -1) return -1;

    for(i=2; i<MAX_FILES; i++){
        if( curr_pcb->fd[i].flags == 0){
            curr_pcb->fd[i].flags = 1;
            curr_pcb->fd[i].inode = dentry.inode_num;
            curr_pcb->fd[i].file_position = 0;
            switch(dentry.file_type){
                case 0:
                    curr_pcb->fd[i].file_ops = &rtc_op;
                    break;
                case 1:
                    curr_pcb->fd[i].file_ops = &dir_op;
                    break;
                case 2:
                    curr_pcb->fd[i].file_ops = &file_op;
                    break;
                default:
                    break;
            }
            return 1;
        }
    }
    return -1;
    
}

int32_t close(int32_t fd){
    pcb_t* curr_pcb = current_pcb();
    if(fd < 2 || fd >= MAX_FILES || curr_pcb->fd[fd].flags==0) return -1;
    curr_pcb->fd[fd].flags = 0;
    curr_pcb->fd[fd].file_ops->close(fd);
    return 0;
}

int32_t getargs(uint8_t* buf, int32_t nbytes){
    return -1;
}

int32_t vidmap(uint8_t** screen_start){
    return -1;
}

int32_t set_handler(int32_t signum, void* handler_address){
    return -1;
}

int32_t sigreturn(void){
    return -1;
}

int32_t null_read(int32_t fd, void* buf, int32_t nbytes){
    return -1;
}

int32_t null_write(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}

int32_t null_open(const uint8_t* filename){
    return -1;
}

int32_t null_close(int32_t fd){
    return -1;
}

pcb_t* current_pcb(){
    uint32_t esp;
    asm volatile ("                 \n\
            movl %%esp, %0          \n\
            "
            : "=r"(esp)
    );
    return (pcb_t*)(esp & (KSTACK_START - KSTACK_SIZE));
}

