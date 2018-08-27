
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

/*------------------------THERE MIGHT BE A PROBLEM------------------------*/
#define PROC_ORIGIN_STACK 0x400

/*------------------------DEFINITION FOR CALCULATOR------------------------*/
const char sign[4] = { '+','-','*','/' };
const char level[4] = { 2,2,1,1 };
const char number[10] = { '0','1','2','3','4','5','6','7','8','9' };

struct ans{
	int num;
	int err;
};

struct readStr{
	int argc;
	char * argv[PROC_ORIGIN_STACK];
};

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	struct task* p_task;
	struct proc* p_proc= proc_table;
	char* p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16   selector_ldt = SELECTOR_LDT_FIRST;
        u8    privilege;
        u8    rpl;
	int   eflags;
	int   i, j;
	int   prio;
	for (i = 0; i < NR_TASKS+NR_PROCS; i++) {
	        if (i < NR_TASKS) {     /* 任务 */
                        p_task    = task_table + i;
                        privilege = PRIVILEGE_TASK;
                        rpl       = RPL_TASK;
                        eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
			prio      = 15;
                }
                else {                  /* 用户进程 */
                        p_task    = user_proc_table + (i - NR_TASKS);
                        privilege = PRIVILEGE_USER;
                        rpl       = RPL_USER;
                        eflags    = 0x202; /* IF=1, bit 2 is always 1 */
			prio      = 5;
                }

		strcpy(p_proc->name, p_task->name);	/* name of the process */
		p_proc->pid = i;			/* pid */

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(struct descriptor));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(struct descriptor));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		/* p_proc->nr_tty		= 0; */

		p_proc->p_flags = 0;
		p_proc->p_msg = 0;
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;
		p_proc->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p_proc->filp[j] = 0;

		p_proc->ticks = p_proc->priority = prio;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

        /* proc_table[NR_TASKS + 0].nr_tty = 0; */
        /* proc_table[NR_TASKS + 1].nr_tty = 1; */
        /* proc_table[NR_TASKS + 2].nr_tty = 1; */

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

	init_clock();
        init_keyboard();

	restart();

	while(1){}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}

/*------------------------CLEAR THE SCREEN------------------------*/
void clear()
{	
	clear_screen(0,console_table[current_console].cursor);
    console_table[current_console].crtc_start = console_table[current_console].orig;
    console_table[current_console].cursor = console_table[current_console].orig;    
}
void clearScreen() {
	int i;
	disp_pos = 0;
	for(i = 0; i < 80 * 50; i++)
	{
		disp_str(" ");
	}
	disp_pos = 0;
}
void clearAllScreen() {
	int i;
	disp_pos = 0;
	for(i = 0; i < 80 * 300; i++)
	{
		disp_str(" ");
	}
	disp_pos = 0;	
}

PUBLIC void addTwoString(char *to_str,char *from_str1,char *from_str2){
    int i=0,j=0;
    while(from_str1[i]!=0)
        to_str[j++]=from_str1[i++];
    i=0;
    while(from_str2[i]!=0)
        to_str[j++]=from_str2[i++];
    to_str[j]=0;
}

/*------------------------WELCOME INFORMATION------------------------*/
void displayWelcomeInfo(){
	clearScreen();
	printf("================================================================================");
	printf("\n\n\n\n\n");
	printf("                     Welcome to .. Whatever this is\n");
	printf("\n\n\n\n\n");
	printf("              You can input [help] to see all the commands\n");
	printf("\n\n\n\n\n\n\n");
	printf("                               Made by:\n");
	printf("        1652751 Junqing LIANG 1652784 Minyu TENG 1551534 Yunxin SUN\n");
	printf("                                2018.8\n");
	printf("===============================================================================");
}

/*****************************************************************************
 *                                Custom Command
 *****************************************************************************/
char* findpass(char *src)
{
    char pass[128];
    int flag = 0;
    char *p1, *p2;

    p1 = src;
    p2 = pass;

    while (p1 && *p1 != ' ')
    {
        if (*p1 == ':')
            flag = 1;

        if (flag && *p1 != ':')
        {
            *p2 = *p1;
            p2++;
        }
        p1++;
    }
    *p2 = '\0';

    return pass;
}

void clearArr(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
        arr[i] = 0;
}

void printTitle()
{
    clear(); 	

  //  disp_color_str("dddddddddddddddd\n", 0x9);
    if(current_console==0){
    	displayWelcomeInfo();
    }
    else{
    	printf("[TTY #%d]\n", current_console);
    } 
}

void doTest(char *path)
{
    struct dir_entry *pde = find_entry(path);
    printf(pde->name);
    printf("\n");
    printf(pde->pass);
    printf("\n");
}

int verifyFilePass(char *path, int fd_stdin)
{
    char pass[128];

    struct dir_entry *pde = find_entry(path);

    /*printf(pde->pass);*/

    if (strcmp(pde->pass, "") == 0)
        return 1;

    printf("Please input the file password: ");
    read(fd_stdin, pass, 128);

    if (strcmp(pde->pass, pass) == 0)
        return 1;

    return 0;
}

/*------------------------WON'T WORK------------------------*/
/*
void doEncrypt(char *path, int fd_stdin)
{
    //查找文件
    //struct dir_entry *pde = find_entry(path);

    char pass[128] = {0};

    printf("Please input the new file password: ");
    read(fd_stdin, pass, 128);

    if (strcmp(pass, "") == 0)
    {
        //printf("A blank password!\n");
        strcpy(pass, "");
    }
    //以下内容用于加密
    int i, j;

    char filename[MAX_PATH];
    memset(filename, 0, MAX_FILENAME_LEN);
    struct inode * dir_inode;

    if (strip_path(filename, path, &dir_inode) != 0)
        return 0;

    if (filename[0] == 0)   // path: "/" 
        return dir_inode->i_num;

    //Search the dir for the file.
    int dir_blk0_nr = dir_inode->i_start_sect;
    int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    int nr_dir_entries =
      dir_inode->i_size / DIR_ENTRY_SIZE; /**
                           * including unused slots
                           * (the file has been deleted
                           * but the slot is still there)
                           *
    int m = 0;
    struct dir_entry * pde;
    for (i = 0; i < nr_dir_blks; i++) {
        RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
        pde = (struct dir_entry *)fsbuf;
        for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++)
        {
            if (memcmp(filename, pde->name, MAX_FILENAME_LEN) == 0)
            {
                //刷新文件
                strcpy(pde->pass, pass);
                WR_SECT(dir_inode->i_dev, dir_blk0_nr + i);
                return;
                //return pde->inode_nr;
            }
            if (++m > nr_dir_entries)
                break;
        }
        if (m > nr_dir_entries) // all entries have been iterated
            break;
    }

}
*/

void help()
{
    printf("=============================help information==================================\n");
    printf("***********  Commands  ***********|***************  description  *************\n");
    printf("1.  help                          : Show this help message\n");
    printf("2.  clear                         : Clear the screen\n");
    printf("3.  process                       : Show all process information\n");
    printf("4.  ls                            : List files in current directory\n");
    printf("5.  create      [file]            : Create a new file\n");
    printf("6.  cat         [file]            : Print the file\n");
    printf("7.  vi          [file]            : Modify the content of the file\n");
    printf("8.  delete      [file]            : Delete a file\n");
    printf("9.  cp          [SOURCE] [DEST]   : Copy a file\n");
    printf("10. mv          [SOURCE] [DEST]   : Move a file\n");   
//    printf("11. encrypt     [file]            : Encrypt a file\n");
    printf("11. cd          [pathname]        : Change the directory\n");
    printf("12. mkdir       [directory name]  : Create a new directory in current directory\n");
	printf("13. calendar                      : Show the calendar\n");
	printf("14. calculator                    : Calculate, eg. calendar 2+6\n");
    printf("15. minesweeper                   : The Minesweeper Game\n");
	printf("16. gobang                        : The GoBang Game\n");
	printf("17. shutdown                      : Shut the computer down\n");
    printf("==============================================================================\n");
}

void ProcessManage()
{
    int i;
    printf("=============================================================================\n");
    printf("      processID      |    name       | spriority    | running?\n");
    //进程号，进程名，优先级，是否是系统进程，是否在运行
    printf("-----------------------------------------------------------------------------\n");
    for ( i = 0 ; i < NR_TASKS + NR_PROCS ; ++i )//逐个遍历
    {
        /*if ( proc_table[i].priority == 0) continue;//系统资源跳过*/
        printf("        %d           %s            %d                yes\n", proc_table[i].pid, proc_table[i].name, proc_table[i].priority);
    }
    printf("=============================================================================\n");
}

//游戏运行库
unsigned int _seed2 = 0xDEADBEEF;

void srand(unsigned int seed){
	_seed2 = seed;
}

int rand() {
	int next = _seed2;
	int result;

	next *= 1103515245;
	next += 12345;
	result = (next / 65536) ;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (next / 65536) ;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (next / 65536) ;

	_seed2 = next;

	return result>0 ? result : -result;
}

void show_mat(int *mat,int *mat_state, int touch_x,int touch_y,int display){
	int x, y;
	for (x = 0; x < 10; x++){
		printf("  %d", x);
	}
	printf("\n");
	for (x = 0; x < 10; x++){
		printf("---");
	}
	for (x = 0; x < 10; x++){
		printf("\n%d|", x);
		for (y = 0; y < 10; y++){
			if (mat_state[x * 10 + y] == 0){				
					if (x == touch_x && y == touch_y)
						printf("*  ");
					else if (display!=0 && mat[x * 10 + y] == 1)
						printf("#  ");
					else
						printf("-  ");
				
			}
			else if (mat_state[x * 10 + y] == -1){
				printf("O  ");
			}
			else{
				printf("%d  ", mat_state[x * 10 + y]);
			}
			
		}
	}
	printf("\n");
}

void init_game(int *mat, int mat_state[100]){
	int sum = 0;
	int x, y;
	for (x = 0; x < 100; x++)
		mat[x] = mat_state[x] = 0;
	while (sum < 15){
		x = rand() % 10;
		y = rand() % 10;
		if (mat[x * 10 + y] == 0){
			sum++;
			mat[x * 10 + y] = 1;
		}
	}
	show_mat(mat,mat_state,-1,-1,0);
	/*for (x = 0; x < 10; x++){
		printf("  %d", x);
	}
	for (x = 0; x < 10; x++){
		printf("\n%d ", x);
		for (y = 0; y < 10; y++){
			printf("%d  ", mat[x * 10 + y]);
		}
	}
	printf("\n");*/
}

int check(int x, int y, int *mat){
	int i, j,now_x,now_y,result = 0;
	for (i = -1; i <= 1; i++){
		for (j = -1; j <= 1; j++){
			if (i == 0 && j == 0)
				continue;
			now_x = x + i;
			now_y = y + j;
			if (now_x >= 0 && now_x < 10 && now_y >= 0 && now_y <= 9){
				if (mat[now_x * 10 + now_y] == 1)
					result++;
			}
		}
	}
	return result;
}

void dfs(int x, int y, int *mat, int *mat_state,int *left_coin){
	int i, j, now_x, now_y,temp;
	if (mat_state[x*10+y] != 0)
		return;
	(*left_coin)--;
	temp = check(x, y,mat);
	if (temp != 0){
		mat_state[x * 10 + y] = temp;
	}
	else{
		mat_state[x * 10 + y] = -1;
		for (i = -1; i <= 1; i++){
			for (j = -1; j <= 1; j++){
				if (i == 0 && j == 0)
					continue;
				now_x = x + i;
				now_y = y + j;
				if (now_x >= 0 && now_x < 10 && now_y >= 0 && now_y <= 9){				
					dfs(now_x, now_y,mat,mat_state,left_coin);
				}
			}
		}
	}
}

void game(int fd_stdin){
	int mat[100] = { 0 };
	int mat_state[100] = { 0 };
	char keys[128];
	int x, y, left_coin = 100,temp;
	int flag = 1;
	
	while (flag == 1){
		init_game(mat, mat_state);
		left_coin = 100;

		printf("-------------------------------------------\n\n");
		printf("Input next x and y: ");

		while (left_coin != 15){

			clearArr(keys, 128);
            int r = read(fd_stdin, keys, 128);
            if(keys[0]>'9'||keys[0]<'0'||keys[1]!=' '||keys[2]>'9'||keys[2]<'0'||keys[3]!=0){
            	printf("Please input again!\n");
				continue;
            } 
            x = keys[0]-'0';
            y = keys[2]-'0';
			if (x < 0 || x>9 || y < 0 || y>9){
				printf("Please input again!\n");
				continue;
			}

			if (mat[x * 10 + y] == 1){
				break;
			}
			else{
				dfs(x, y, mat, mat_state, &left_coin);
				if (left_coin <= 15)
					break;
				show_mat(mat, mat_state, -1, -1, 0);
				printf("-------------------------------------------\n\n");
				printf("Input next x and y: ");
				/*printf("%d\n", left_coin);
				for (x = 0; x < 10; x++){

					printf("  %d", x);
				}
				for (x = 0; x < 10; x++){
					printf("\n%d ", x);
					for (y = 0; y < 10; y++){
						printf("%d  ", mat[x * 10 + y]);
					}
				}
				printf("\n\n");*/
			}
		}
		if (mat[x * 10 + y] == 1){
			printf("\n\nFAIL!\n");
			show_mat(mat, mat_state, x, y, 1);
		}
		else{
			printf("\n\nSUCCESS!\n");
			show_mat(mat, mat_state, -1, -1, 1);
		}

		printf("Do you want to continue?(yes ot no)\n");
		clearArr(keys, 128);
        int r = read(fd_stdin, keys, 128);
      //  printf("%s\n",keys);
        if (keys[0]=='n' && keys[1]=='o' && keys[2]==0)
        {
        	flag = 0;
        //	printf("%s\n",keys);
            break;
        }
	}	
}

void shell(char *tty_name){
	 int fd;

    //int isLogin = 0;

    char rdbuf[512];
    char cmd[512];
    char arg1[512];
    char arg2[512];
    char buf[1024];
    char temp[512];
   
          
    

    int fd_stdin  = open(tty_name, O_RDWR);
    assert(fd_stdin  == 0);
    int fd_stdout = open(tty_name, O_RDWR);
    assert(fd_stdout == 1);

    //animation();
    clear();
    //animation_l();
    
	openAnimation();
    displayWelcomeInfo();

   
    
   	

   char current_dirr[512] = "/";
   
    while (1) {  
        //清空数组中的数据用以存放新数据
        clearArr(rdbuf, 512);
        clearArr(cmd, 512);
        clearArr(arg1, 512);
        clearArr(arg2, 512);
        clearArr(buf, 1024);
        clearArr(temp, 512);

        printf("~%s$ ", current_dirr);

        int r = read(fd_stdin, rdbuf, 512);

        if (strcmp(rdbuf, "") == 0)
            continue;

        //解析命令
        int i = 0;
        int j = 0;
        while(rdbuf[i] != ' ' && rdbuf[i] != 0)
        {
            cmd[i] = rdbuf[i];
            i++;
        }
        i++;
        while(rdbuf[i] != ' ' && rdbuf[i] != 0)
        {
            arg1[j] = rdbuf[i];
            i++;
            j++;
        }
        i++;
        j = 0;
        while(rdbuf[i] != ' ' && rdbuf[i] != 0)
        {
            arg2[j] = rdbuf[i];
            i++;
            j++;
        }
        //清空缓冲区
        rdbuf[r] = 0;

		int argc = 0;
		char * argv[PROC_ORIGIN_STACK];
		char * p = rdbuf;
		char * s;
		int word = 0;
		char ch;
		do {
			ch = *p;
			if (*p != ' ' && *p != 0 && !word) {
				s = p;
				word = 1;
			}
			if ((*p == ' ' || *p == 0) && word) {
				word = 0;
				argv[argc++] = s;
				*p = 0;
			}
			p++;
		} while(ch);
		argv[argc] = 0;

		if (!strcmp(cmd, "shutdown")) {
			closeAnimation();
			while(1);
		} else if (!strcmp(cmd, "calculator")){
			calculateMain(argc,argv);
		} else if (!strcmp(cmd, "calendar")){
			calendarMain();
		} else if (!strcmp(cmd, "gobang")){
			goBangGameStart();
		}
		
        else if (strcmp(cmd, "process") == 0)
        {
            ProcessManage();
        }
        else if (strcmp(cmd, "help") == 0)
        {
            help();
        }      
        else if (strcmp(cmd, "clear") == 0)
        {
            printTitle();
        }
        else if (strcmp(cmd, "ls") == 0)
        {
            ls(current_dirr);
        }
        else if (strcmp(cmd, "create") == 0)
        {
            if(arg1[0]!='/'){
                addTwoString(temp,current_dirr,arg1);
                memcpy(arg1,temp,512);                
            }

            fd = open(arg1, O_CREAT | O_RDWR);
            if (fd == -1)
            {
                printf("Failed to create file! Please check the filename!\n");
                continue ;
            }
            write(fd, buf, 1);
            printf("File created: %s (fd %d)\n", arg1, fd);
            close(fd);
        }
        else if (strcmp(cmd, "cat") == 0)
        {
            if(arg1[0]!='/'){
                addTwoString(temp,current_dirr,arg1);
                memcpy(arg1,temp,512);                
            }

            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("Failed to open file! Please check the filename!\n");
                continue ;
            }
            if (!verifyFilePass(arg1, fd_stdin))
            {
                printf("Authorization failed\n");
                continue;
            }
            read(fd, buf, 1024);
            close(fd);
            printf("%s\n", buf);
        }
        else if (strcmp(cmd, "vi") == 0)
        {
            if(arg1[0]!='/'){
                addTwoString(temp,current_dirr,arg1);
                memcpy(arg1,temp,512);                
            }

            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("Failed to open file! Please check the filename!\n");
                continue ;
            }
            if (!verifyFilePass(arg1, fd_stdin))
            {
                printf("Authorization failed\n");
                continue;
            }
            int tail = read(fd_stdin, rdbuf, 512);
            rdbuf[tail] = 0;

            write(fd, rdbuf, tail+1);
            close(fd);
        }
        else if (strcmp(cmd, "delete") == 0)
        {

            if(arg1[0]!='/'){
                addTwoString(temp,current_dirr,arg1);
                memcpy(arg1,temp,512);                
            }

            int result;
            result = unlink(arg1);
            if (result == 0)
            {
                printf("File deleted!\n");
                continue;
            }
            else
            {
                printf("Failed to delete file! Please check the filename!\n");
                continue;
            }
        } 
        else if (strcmp(cmd, "cp") == 0)
        {
            //首先获得文件内容
            if(arg1[0]!='/'){
                addTwoString(temp,current_dirr,arg1);
                memcpy(arg1,temp,512);                
            }
            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("File not exists! Please check the filename!\n");
                continue ;
            }
            
            int tail = read(fd, buf, 1024);
            close(fd);

            if(arg2[0]!='/'){
                addTwoString(temp,current_dirr,arg2);
                memcpy(arg2,temp,512);                
            }
            
            fd = open(arg2, O_CREAT | O_RDWR);
            if (fd == -1)
            {
                //文件已存在，什么都不要做
            }
            else
            {
                //文件不存在，写一个空的进去
                char temp2[1024];
                temp2[0] = 0;
                write(fd, temp2, 1);
                close(fd);
            }
             
            //给文件赋值
            fd = open(arg2, O_RDWR);
            write(fd, buf, tail+1);
            close(fd);
        } 
        else if (strcmp(cmd, "mv") == 0)
        {
             if(arg1[0]!='/'){
                addTwoString(temp,current_dirr,arg1);
                memcpy(arg1,temp,512);                
            }
            //首先获得文件内容
            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("File not exists! Please check the filename!\n");
                continue ;
            }
           
            int tail = read(fd, buf, 1024);
            close(fd);

            if(arg2[0]!='/'){
                addTwoString(temp,current_dirr,arg2);
                memcpy(arg2,temp,512);                
            }
            
            fd = open(arg2, O_CREAT | O_RDWR);
            if (fd == -1)
            {
                //文件已存在，什么都不要做
            }
            else
            {
                //文件不存在，写一个空的进去
                char temp2[1024];
                temp2[0] = 0;
                write(fd, temp2, 1);
                close(fd);
            }
             
            //给文件赋值
            fd = open(arg2, O_RDWR);
            write(fd, buf, tail+1);
            close(fd);
            //最后删除文件
            unlink(arg1);
        }   
/*        else if (strcmp(cmd, "encrypt") == 0)
        {
            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("File not exists! Please check the filename!\n");
                continue ;
            }
            if (!verifyFilePass(arg1, fd_stdin))
            {
                printf("Authorization failed\n");
                continue;
            }
            //doEncrypt(arg1, fd_stdin);
        } */
        else if (strcmp(cmd, "test") == 0)
        {
            doTest(arg1);
        }
        else if (strcmp(cmd, "minesweeper") == 0){
        	game(fd_stdin);
        }
        else if (strcmp(cmd, "mkdir") == 0){
            i = j =0;
            while(current_dirr[i]!=0){
                arg2[j++] = current_dirr[i++];
            }
            i = 0;
            
            while(arg1[i]!=0){
                arg2[j++]=arg1[i++];
            }
            arg2[j]=0;
            printf("%s\n", arg2);
            fd = mkdir(arg2);            
        }
        else if (strcmp(cmd, "cd") == 0){
            if(arg1[0]!='/'){ // not absolute path from root
                i = j =0;
                while(current_dirr[i]!=0){
                    arg2[j++] = current_dirr[i++];
                }
                i = 0;
               
                while(arg1[i]!=0){
                    arg2[j++]=arg1[i++];
                }
                arg2[j++] = '/';
                arg2[j]=0;
                memcpy(arg1, arg2, 512);
		
            }
            else if(strcmp(arg1, "/")!=0){
                for(i=0;arg1[i]!=0;i++){}
                arg1[i++] = '/';
                arg1[i] = 0;
            }
            printf("%s\n", arg1);
	    int nump=0;
           char eachf[512];
		memset(eachf,0,sizeof(eachf)); 
		const char *s = arg1;
		char *t = eachf;
				if(arg1[0]=="/")
					*s++;
				while (*s) {	
					*t++ = *s++;
				}
		t = eachf;
		while (*t) {	
			char *ps = t+1;
			if(*ps==0){
				if(*t == '/')
				*t = 0;			
			}		
			t++;
		}	
            fd = open(eachf, O_RDWR);
            printf("%s\n", eachf);
            if(fd == -1){
                printf("The path not exists!Please check the pathname!\n");
            }
            else{
                memcpy(current_dirr, arg1, 512);
                printf("Change dir %s success!\n", current_dirr);
            }
        }
        else
            printf("Command not found, please check!\n");
            
    }
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{

	
	//while(1);
	char tty_name[] = "/dev_tty0";
	shell(tty_name);
	assert(0); /* never arrive here */
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	char tty_name[] = "/dev_tty1";
	shell(tty_name);
	
	assert(0); /* never arrive here */
}

/*======================================================================*
                               TestC
 *======================================================================*/
void TestC()
{
	//char tty_name[] = "/dev_tty2";
	//shell(tty_name);
	spin("TestC");
	/* assert(0); */
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}


//well let's give it a go ...

/*------------------------OPEN THE COMPUTER------------------------*/
void openAnimation()
{              
	int i, j;
	char progressBar[80];
	for (i = 0; i < 10; i++) {
		clearScreen();
		disp_str("\n\n");
		disp_str("                                             ,1&1\n");
		disp_str("                                           ;3&B#:\n");
		disp_str("                                         .G@@@@9\n");
		disp_str("                                         X@@MAh\n");
		disp_str("                                 ,irr;.  551r::iri:\n");
		disp_str("                             .18AM####HG3558AB####MH9i\n");
		disp_str("                            1H@@#MMMMM##@@@##MMMMMM#@Bi\n");
		disp_str("                           5@#MMMMMMMMMMMMMMMMMMMMMM5.\n");
		disp_str("                          ,M#MMMMMMMMMMMMMMMMMMMMM#1\n");
		disp_str("                          i#MMMMMMMMMMMMMMMMMMMMM#H\n");
		disp_str("                          ,MMMMMMMMMMMMMMMMMMMMMMMM;\n");
		disp_str("                           G@MMMMMMMMMMMMMMMMMMMMM#H;\n");
		disp_str("                           ;#MMMMMMMMMMMMMMMMMMMMMM#M9r.\n");
		disp_str("                            5@MMMMMMMMMMMMMMMMMMMMMMM@#i\n");
		disp_str("                             S@#MMMMMMMMMMMMMMMMMMMM#Mr\n");
		disp_str("                              hM@MMMMMMMM##MMMMMMM#@Hi\n");
		disp_str("                               ;GM@@@@#MBHHB##@@@#B3.\n");
		disp_str("                                 :1SShs;,..,;s5S5r.\n\n");
		disp_str("\n");
		switch (i) {
			case 0: disp_str("                    ____\n"); break;
			case 1: disp_str("                    ________\n"); break;
			case 2: disp_str("                    ____________\n"); break;
			case 3: disp_str("                    ________________\n"); break;
			case 4: disp_str("                    ____________________\n"); break;
			case 5: disp_str("                    ________________________\n"); break;
			case 6: disp_str("                    ____________________________\n"); break;
			case 7: disp_str("                    ________________________________\n"); break;
			case 8: disp_str("                    ____________________________________\n"); break;
			case 9: disp_str("                    ________________________________________\n"); break;
			default: break;
		}
		milli_delay(2000);
	}    
}    


/*------------------------SHUT THE COMPUTER DOWN------------------------*/
void closeAnimation() {
	clearAllScreen();
	printf("\n\n\n\n\n");
	printf("                   GGGGG OOOOO OOOOO DDD     BBBB  Y   Y EEEEE\n");
	printf("                   G     O   O O   O D  D    B   B  Y Y  E\n");
	printf("                   G  GG O   O O   O D   D   BBBB    Y   EEEEE\n");
	printf("                   G   G O   O O   O D  D    B   B   Y   E\n");
	printf("                   GGGGG OOOOO OOOOO DDD     BBBB    Y   EEEEE");
}

/*------------------------SCANF------------------------*/
struct readStr readScanf(){

	struct readStr rs;
	int i;
	int argc = 0;
	char * argv[PROC_ORIGIN_STACK];
	char rdbuf[128];
	int r = read(0, rdbuf, 70);
	rdbuf[r] = 0;

	rs.argc = 0;

	char * p = rdbuf;
	char * s;
	int word = 0;
	char ch;
	do {
		ch = *p;
		if (*p != ' ' && *p != 0 && !word) {
			s = p;
			word = 1;
		}
		if ((*p == ' ' || *p == 0) && word) {
			word = 0;
			argv[argc++] = s;
			*p = 0;
		}
		p++;
	} while(ch);
	argv[argc] = 0;
	rs.argc = argc;
	for (i = 0; i <= argc; i++){
		rs.argv[i] = argv[i];
	}
	return rs;
}

/*------------------------SIMPLE CALCULATOR-----------------------*/
int checkOutArit(char * str) {
	int i, j, flag;
	for (i = 0; i < strlen(str); i++) {
		flag = 0;

		for (j = 0; j < 4; j++) {
			if (str[i] == sign[j]) {
				flag = 1;
				break;
			}
		}

		if (flag == 0) {
			for (j = 0; j < 10; j++) {
				if (str[i] == number[j]) {
					flag = 1;
					break;
				}
			}
		}
		if (flag == 0) {
			return 0;
		}
	}

	return 1;
}

//寻找符号的编号
int findSignNumber(char reSign) {
	int i;
	for (i = 0; i < 4; i++)
	{
		if (reSign == sign[i]) {
			return i;
		}
	}

	return -1;
}

//构建数字
struct ans createNumber(char *str, int first, int last) {
	int a = 1,sum = 0,i;
	struct ans reAns;

	if (first > last) {
		reAns.num = 0;
		reAns.err = 2;
	}
	else {
		for (i = last; i >= first; i--) {
			sum = sum + (str[i] - '0') * a;
			a = a * 10;
		}
		reAns.num = sum;
		reAns.err = -1;
	}

	return reAns;
}

//单一表达式求值
struct ans calculateOnly(int num1, int num2, char op) {
	struct ans reAns;

	reAns.err = -1;
	if (op == '+') {
		reAns.num = num1 + num2;
	}
	if (op == '-') {
		reAns.num = num1 - num2;
	}
	if (op == '*') {
		reAns.num = num1 * num2;
	}
	if (op == '/') {
		if (num2 == 0){
			reAns.num = 0;
			reAns.err = 3;
		}
		else {
			reAns.num = num1 / num2;
		}
	}

	return reAns;
}

//递归进行计算
struct ans calculateArit(char *str, int first, int last) {
	int i,signNumber,maxLevel = 0,maxAdress = -1;
	struct ans num1, num2;

	//寻找最高优先级
	for (i = first; i <= last; i++) {
		signNumber = findSignNumber(str[i]);
		if (signNumber != -1 && level[signNumber] > maxLevel)
		{
			maxLevel = level[signNumber];
			maxAdress = i;
		}
	}

	if (maxLevel == 0) {
		return createNumber(str, first, last);
	}
	else {
		num1 = calculateArit(str, first, maxAdress - 1);
		num2 = calculateArit(str, maxAdress + 1, last);
		if (num1.err == -1 && num2.err == -1) {
			return calculateOnly(num1.num, num2.num, str[maxAdress]);
		}
		else {
			if (num1.err != -1) {
				return num1;
			}
			else {
				return num2;
			}
		}
	}
}

//输出违规操作提示
void printfLawlessOperation(int lawlessNum) {
	if (lawlessNum == 1) {
		printf("-----> Lawlessness character! <-----\n\n");
	}
	if (lawlessNum == 2) {
		printf("-----> Half-baked expression! <-----\n\n");
	}
	if (lawlessNum == 3) {
		printf("-----> Divide zero operation! <-----\n\n");
	}
}

void calculateMain(int argc, char *argv[]) {
	struct ans reAns;
	printf("-----> Calculator <-----\n");
	if (checkOutArit(argv[1]) == 0) {
		printfLawlessOperation(1);
	}
	else {
		reAns = calculateArit(argv[1], 0, strlen(argv[1]) - 1);
		if (reAns.err == -1){
			printf("%d\n\n", reAns.num);
		}
		else {
			printfLawlessOperation(reAns.err);
		}
	}
}


/*------------------------CALENDAR------------------------*/
/*------------------------THERE IS A BIG PROBLEM!!------------------------*/
#define N 7
void calendarMain(){
	int year, month;
	printf("Please enter the year and month, eg. 2018 8\n");
	struct readStr rs;

	while(1){
		rs = readScanf();

		year=createNumber(rs.argv[0],0,strlen(rs.argv[0]) - 1).num;
		month=createNumber(rs.argv[1],0,strlen(rs.argv[1]) - 1).num;
		if(month<1||month>12){
			printf("Please enter the right month!\n");
		}
		else{
			break;
		}	
	}
	calendar(year,month);
}

void print(int day,int tian)
{
	int a[N][N],i,j,sum=1;
	for(i=0,j=0;j<7;j++)
	{
		if(j<day)
			printf("    ");
		else
		{
			a[i][j]=sum;
			printf("   %d",sum++);
			// printf("aaa\n");
		}
	}
	printf("\n");
	for(i=1;sum<=tian;i++)
	{
		for(j=0;sum<=tian&&j<7;j++)
		{
			a[i][j]=sum;
			if (sum<10)
			{
				printf("   %d", sum++);
			}
			else{
				printf("  %d",sum++);
			}
		}
		printf("\n");
	}
}

int duo(int year)
{
	if(year%4==0&&year%100!=0||year%400==0)
		return 1;
	else
		return 0;
}

int calendar(int year,int month)
{
	int day,tian,preday,strday;
	char * e_month="";
	switch(month){
		case 1:	e_month="Jan.";break;
		case 2:	e_month="Feb.";break;
		case 3:	e_month="Mar.";break;
		case 4:	e_month="Apr.";break;
		case 5:	e_month="May";break;
		case 6:	e_month="Jun.";break;
		case 7:	e_month="Jul.";break;
		case 8:	e_month="Aug.";break;
		case 9:	e_month="Sep.";break;
		case 10:	e_month="Oct.";break;
		case 11:	e_month="Nov.";break;
		case 12:	e_month="Dec.";break;
	}

	printf("************%s %d************\n",e_month,year);
	printf(" SUN MON TUE WED THU FRI SAT\n");
	switch(month)
	{
		case 1:
		tian=31;
		preday=0;
		break;
		case 2:
		tian=28;
		preday=31;
		break;
		case 3:
		tian=31;
		preday=59;
		break;
		case 4:
		tian=30;
		preday=90;
		break;
		case 5:
		tian=31;
		preday=120;
		break;
		case 6:
		tian=30;
		preday=151;
		break;
		case 7:
		tian=31;
		preday=181;
		break;
		case 8:
		tian=31;
		preday=212;
		break;
		case 9:
		tian=30;
		preday=243;
		break;
		case 10:
		tian=31;
		preday=273;
		break;
		case 11:
		tian=30;
		preday=304;
		break;
		default:
		tian=31;
		preday=334;
	}
	if(duo(year)&&month>2)
	preday++;
	if(duo(year)&&month==2)
	tian=29;
	day=((year-1)*365+(year-1)/4-(year-1)/100+(year-1)/400+preday+1)%7;
	print(day,tian);
}


/*------------------------GOBANG GAME------------------------*/
char gameMap[15][15];

int maxInt(int x,int y){
	return x>y?x:y;
}

int selectPlayerOrder()
{
	printf("o player\n");
	printf("* computer\n");
	printf("who play first?[1/user  other/computer]\n");
	struct readStr rs;
	rs = readScanf();
	if (strcmp(rs.argv[0][0],"1")==0) return 1;
	else return 0;
}

void displayGameState()
{
	int n=15;
	int i,j;
	for (i=0; i<=n; i++)
	{
		if (i<10) printf("%d   ",i);
		else printf("%d  ",i);
	}
	printf("\n");
	for (i=0; i<n; i++)
	{
		if (i<9) printf("%d   ",i+1);
		else printf("%d  ",i+1);
		for (j=0; j<n; j++)
		{
			if (j<10) printf("%c   ",gameMap[i][j]);
			else printf("%c   ",gameMap[i][j]);
		}
		printf("\n");
	}

}

int checkParameter(int x, int y)	//检查玩家输入的参数是否正确
{
	int n=15;
	if (x<0 || y<0 || x>=n || y>=n) return 0;
	if (gameMap[x][y]!='_') return 0;
	return 1;
}

//更新的位置为x，y，因此 只要检查坐标为x，y的位置
int win(int x,int y)		//胜利返回1    否则0（目前无人获胜）
{
	int n=15;
	int i,j;
	int gameCount;
	//左右扩展
	gameCount=1;
	for (j=y+1; j<n; j++)
	{
		if (gameMap[x][j]==gameMap[x][y]) gameCount++;
		else break;
	}
	for (j=y-1; j>=0; j--)
	{
		if (gameMap[x][j]==gameMap[x][y]) gameCount++;
		else break;
	}
	if (gameCount>=5) return 1;

	//上下扩展
	gameCount=1;
	for (i=x-1; i>0; i--)
	{
		if (gameMap[i][y]==gameMap[x][y]) gameCount++;
		else break;
	}
	for (i=x+1; i<n; i++)
	{
		if (gameMap[i][y]==gameMap[x][y]) gameCount++;
		else break;
	}
	if (gameCount>=5) return 1;

	//正对角线扩展
	gameCount=1;
	for (i=x-1,j=y-1; i>=0 && j>=0; i--,j--)
	{
		if (gameMap[i][j]==gameMap[x][y]) gameCount++;
		else break;
	}
	for (i=x+1,j=y+1; i<n && j<n; i++,j++)
	{
		if (gameMap[i][j]==gameMap[x][y]) gameCount++;
		else break;
	}
	if (gameCount>=5) return 1;

	//负对角线扩展
	gameCount=1;
	for (i=x-1,j=y+1; i>=0 && j<n; i--,j++)
	{
		if (gameMap[i][j]==gameMap[x][y]) gameCount++;
		else break;
	}
	for (i=x+1,j=y-1; i<n && j>=0; i++,j--)
	{
		if (gameMap[i][j]==gameMap[x][y]) gameCount++;
		else break;
	}
	if (gameCount>=5) return 1;

	return 0;
}

void free1(int x,int y1,int y2,int* ff1,int* ff2)
{
	int n=15;
	int i;
	int f1=0,f2=0;
	for (i=y1; i>=0; i++)
	{
		if (gameMap[x][i]=='_') f1++;
		else break;
	}
	for (i=y2; i<n; i++)
	{
		if (gameMap[x][i]=='_') f2++;
		else break;
	}
	*ff1=f1;
	*ff2=f2;
}

void free2(int x1,int x2,int y,int *ff1,int *ff2)
{
	int n=15;
	int i;
	int f1=0,f2=0;
	for (i=x1; i>=0; i--)
	{
		if (gameMap[i][y]=='_') f1++;
		else break;
	}
	for (i=x2; i<n; i++)
	{
		if (gameMap[i][y]=='_') f2++;
		else break;
	}
	*ff1=f1;
	*ff2=f2;
}

void free3(int x1,int y1,int x2,int y2,int *ff1,int *ff2)
{
	int n=15;
	int x,y;
	int f1=0;
	int f2=0;
	for (x=x1,y=y1; 0<=x && 0<=y; x--,y--)
	{
		if (gameMap[x][y]=='_') f1++;
		else break;
	}
	for (x=x2,y=y2; x<n &&  y<n; x++,y++)
	{
		if (gameMap[x][y]=='_') f2++;
		else break;
	}
	*ff1=f1;
	*ff2=f2;
}

void free4(int x1,int y1,int x2,int y2,int *ff1,int *ff2)
{
	int n=15;
	int x,y;
	int f1=0,f2=0;
	for (x=x1,y=y1; x>=0 && y<n; x--,y++)
	{
		if (gameMap[x][y]=='_') f1++;
		else break;
	}
	for (x=x2,y=y2; x<n && y>=0; x++,y--)
	{
		if (gameMap[x][y]=='_') f2++;
		else break;
	}
	*ff1=f1;
	*ff2=f2;
}

int getPossibleByAD(int attack,int defence,int attackFree1,int attackFree2,int defenceFree1,int defenceFree2)
{
	if (attack>=5) return 20;						//5攻击
	if (defence>=5) return 19;						//5防御
	if (attack==4 && (attackFree1>=1 && attackFree2>=1)) return 18;		//4攻击 2边
	if (attack==4 && (attackFree1>=1 || attackFree2>=1)) return 17;		//4攻击 1边
	if (defence==4 && (defenceFree1>=1 || defenceFree2>=1)) return 16;	//4防御
	if (attack==3 && (attackFree1>=2 && attackFree2>=2)) return 15;		//3攻击 2边
	if (defence==3 && (defenceFree1>=2 && defenceFree2>=2)) return 14;	//3防御 2边
	if (defence==3 && (defenceFree1>=2 || defenceFree2>=2)) return 13;	//3防御 1边
	if (attack==3 && (attackFree1>=2 || attackFree2>=2)) return 12;		//3攻击 1边
	if (attack==2 && (attackFree1>=3 && attackFree2>=3)) return 11;		//2攻击 2边
	if (defence==2 && defenceFree1+defenceFree2>=3) return 10;	//2防御 2边
	if (defence==2 && defenceFree1+defenceFree2>=3) return 9;		//2防御 1边
	if (attack==1 && attackFree1+attackFree2>=4) return 8;
	if (defence==1 && defenceFree1+defenceFree2>=4) return 7;
	return 6;
}

int getPossible(int x,int y)
{
	int n=15;
	int attack;
	int defence;
	int attackFree1;
	int defenceFree1;
	int attackFree2;
	int defenceFree2;
	int possible=-100;

	//左右扩展
	int al,ar;
	int dl,dr;
	//横向攻击
	for (al=y-1; al>=0; al--)
	{
		if (gameMap[x][al]!='*') break;
	}
	for (ar=y+1; ar<n; ar++)
	{
		if (gameMap[x][ar]!='*') break;
	}
	//横向防守
	for (dl=y-1; dl>=0; dl--)
	{
		if (gameMap[x][dl]!='o') break;
	}
	for (dr=y+1; dr<n; dr++)
	{
		if (gameMap[x][dr]!='o') break;
	}
	attack=ar-al-1;
	defence=dr-dl-1;
	free1(x,al,ar,&attackFree1,&attackFree2);
	free1(x,dl,dr,&defenceFree1,&defenceFree2);
	possible=maxInt(possible,getPossibleByAD(attack,defence,attackFree1,attackFree2,defenceFree1,defenceFree2));

	//竖向进攻
	for (al=x-1; al>=0; al--)
	{
		if (gameMap[al][y]!='*') break;
	}
	for (ar=x+1; ar<n; ar++)
	{
		if (gameMap[ar][y]!='*') break;
	}
	//竖向防守
	for (dl=x-1; dl>=0; dl--)
	{
		if (gameMap[dl][y]!='o') break;
	}
	for (dr=x+1; dr<n; dr++)
	{
		if (gameMap[dr][y]!='o') break;
	}
	attack=ar-al-1;
	defence=dr-dl-1;
	free2(al,ar,y,&attackFree1,&attackFree2);
	free2(dl,dr,y,&defenceFree1,&defenceFree2);
	possible=maxInt(possible,getPossibleByAD(attack,defence,attackFree1,attackFree2,defenceFree1,defenceFree2));

	//正对角线进攻
	int al1,al2,ar1,ar2;
	int dl1,dl2,dr1,dr2;
	for (al1=x-1,al2=y-1; al1>=0 && al2>=0; al1--,al2--)
	{
		if (gameMap[al1][al2]!='*') break;
	}
	for (ar1=x+1,ar2=y+1; ar1<n && ar2<n; ar1++,ar2++)
	{
		if (gameMap[ar1][ar2]!='*') break;
	}
	//正对角线防守
	for (dl1=x-1,dl2=y-1; dl1>=0 && dl2>=0; dl1--,dl2--)
	{
		if (gameMap[dl1][dl2]!='o') break;
	}
	for (dr1=x+1,dr2=y+1; dr1<n && dr2<n; dr1++,dr2++)
	{
		if (gameMap[dr1][dr2]!='o') break;
	}
	attack=ar1-al1-1;
	defence=dr1-dl1-1;
	free3(al1,al2,ar1,ar2,&attackFree1,&attackFree2);
	free3(dl1,dl2,dr1,dr2,&defenceFree1,&defenceFree2);
	possible=maxInt(possible,getPossibleByAD(attack,defence,attackFree1,attackFree1,defenceFree1,defenceFree2));

	//负对角线进攻
	for (al1=x-1,al2=y+1; al1>=0 && al2<n; al1--,al2++)
	{
		if (gameMap[al1][al2]!='*') break;
	}
	for (ar1=x+1,ar2=y-1; ar1<n && ar2>=0; ar1++,ar2--)
	{
		if (gameMap[ar1][ar2]!='*') break;
	}
	//负对角线防守
	for (dl1=x-1,dl2=y+1; dl1>=0 && dl2<n; dl1--,dl2++)
	{
		if (gameMap[dl1][dl2]!='o') break;
	}
	for (dr1=x+1,dr2=y-1; dr1<n && dr2>=0; dr1++,dr2--)
	{
		if (gameMap[dr1][dr2]!='o') break;
	}
	attack=ar1-al1-1;
	defence=dr1-dl1-1;
	free4(al1,al2,ar1,ar2,&attackFree1,&attackFree2);
	free4(dl1,dl2,dr1,dr2,&defenceFree1,&defenceFree2);
	possible=maxInt(possible,getPossibleByAD(attack,defence,attackFree1,attackFree2,defenceFree1,defenceFree2));
	return possible;
}


void goBangGameStart()
{
	int playerStep=0;
	int computerStep=0;
	int n=15;
	int i,j;

	int goFlag=1;
//	while (1)
	while(goFlag)
	{
		for (i=0; i<n; i++)
			for (j=0; j<n; j++)
				gameMap[i][j]='_';


		gameMap[n>>1][n>>1]='*';
		displayGameState();
		printf("[computer step:%d]%d,%d\n",++computerStep,(n>>1)+1,(n>>1)+1);
		/*else
		{
			displayGameState();
		}*/

		while (1)
		{
			int x,y;
			while (1)
			{
				printf("[player step:%d]\n",++playerStep);
				//scanf("%d%d",&x,&y);
				struct readStr rs;
				rs = readScanf();
				x = createNumber(rs.argv[0],0,strlen(rs.argv[1]) - 1).num;
				y = createNumber(rs.argv[1],0,strlen(rs.argv[1]) - 1).num;
				x--,y--;
				if ( checkParameter(x,y) )
				{
					gameMap[x][y]='o';
					break;
				}
				else
				{
					playerStep--;
					printf("the position you put error\n");
				}
			}
			if (win(x,y))
			{
				displayGameState();
				printf("Congratulation you won the game\n");
				goFlag=0;
				break;
			}
			int willx,willy,winPossible=-100;
			for (i=0; i<n; i++){
				for (j=0; j<n; j++)
				{
					if (gameMap[i][j]=='_')
					{
						int possible=getPossible(i,j);
						if (possible>=winPossible)
						{
							willx=i; willy=j;
							winPossible=possible;
						}
					}
				}
			}
			gameMap[willx][willy]='*';
			displayGameState();
			printf("[computer step:%d]%d,%d\n",++computerStep,willx+1,willy+1);
			if (win(willx,willy))
			{
				printf("Sorry you lost the game\n");
				goFlag=0;
				break;
			}
		}
	}
}
