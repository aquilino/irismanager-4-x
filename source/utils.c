
#include "utils.h"
#include "language.h"
#include <sys/file.h>

#define FS_S_IFMT 0170000
#define FS_S_IFDIR 0040000

typedef s32 Lv2FsMode;
typedef s32 Lv2FsFile;

extern char * language[];
extern char self_path[MAXPATHLEN];
extern int game_list_category;


int copy_async(char *path1, char *path2, u64 size, char *progress_string1, char *progress_string2);

msgType mdialogyesno = MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO | MSG_DIALOG_DISABLE_CANCEL_ON | MSG_DIALOG_DEFAULT_CURSOR_NO;
msgType mdialogyesno2 = MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO | MSG_DIALOG_DISABLE_CANCEL_ON;

msgType mdialogok = MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK;

volatile int dialog_action = 0;

void my_dialog(msgButton button, void *userdata)
{
    switch(button) {

        case MSG_DIALOG_BTN_YES:
            dialog_action = 1;
            break;
        case MSG_DIALOG_BTN_NO:
        case MSG_DIALOG_BTN_ESCAPE:
        case MSG_DIALOG_BTN_NONE:
            dialog_action = 2;
            break;
        default:
		    break;
    }
}

void my_dialog2(msgButton button, void *userdata)
{
    switch(button) {

        case MSG_DIALOG_BTN_OK:
        case MSG_DIALOG_BTN_ESCAPE:
        case MSG_DIALOG_BTN_NONE:
            dialog_action = 1;
            break;
        default:
		    break;
    }
}

static volatile int progress_action2 = 0;

static float bar1_countparts = 0.0f, bar2_countparts = 0.0f;

char progress_bar_title[256];

//static msgType mdialogprogress = MSG_DIALOG_SINGLE_PROGRESSBAR | MSG_DIALOG_MUTE_ON;
static msgType mdialogprogress2 = MSG_DIALOG_DOUBLE_PROGRESSBAR | MSG_DIALOG_MUTE_ON;

static void progress_callback(msgButton button, void *userdata)
{
    switch(button) {

        case MSG_DIALOG_BTN_OK:
            progress_action2 = 1;
            break;
        case MSG_DIALOG_BTN_NO:
        case MSG_DIALOG_BTN_ESCAPE:
            progress_action2 = 2;
            break;
        case MSG_DIALOG_BTN_NONE:
            progress_action2 = -1;
            break;
        default:
		    break;
    }
}


void wait_dialog() 
{


    while(!dialog_action)
        {
        sysUtilCheckCallback();tiny3d_Flip();
        }

    msgDialogAbort();
    usleep(100000);
}

void DrawDialogOKTimer(char * str, float milliseconds)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogok, str, my_dialog2, (void*) 0x0000aaab, NULL );
    msgDialogClose(milliseconds);
            
    wait_dialog();
}

void DrawDialogOK(char * str)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogok, str, my_dialog2, (void*) 0x0000aaab, NULL );
            
    wait_dialog();
}

int DrawDialogYesNoTimer(char * str, float milliseconds)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno, str, my_dialog, (void*)  0x0000aaaa, NULL );
    msgDialogClose(milliseconds);
            
    wait_dialog();

    return dialog_action;
}

int DrawDialogYesNo(char * str)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno, str, my_dialog, (void*)  0x0000aaaa, NULL );
            
    wait_dialog();

    return dialog_action;
}

int DrawDialogYesNoTimer2(char * str, float milliseconds)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno2, str, my_dialog, (void*)  0x0000aaaa, NULL );
    msgDialogClose(milliseconds);
            
    wait_dialog();

    return dialog_action;
}

int DrawDialogYesNo2(char * str)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno2, str, my_dialog, (void*)  0x0000aaaa, NULL );
            
    wait_dialog();

    return dialog_action;
}

char * LoadFile(char *path, int *file_size)
{
    FILE *fp;
    char *mem = NULL;
    
    *file_size = 0;

    sysLv2FsChmod(path, FS_S_IFMT | 0777);

    fp = fopen(path, "rb");

	if (fp != NULL) {
        
        fseek(fp, 0, SEEK_END);
		
        *file_size = ftell(fp);
        
        mem = malloc(*file_size);

		if(!mem) {fclose(fp);return NULL;}
        
        fseek(fp, 0, SEEK_SET);

		fread((void *) mem, 1, *file_size, fp);

		fclose(fp);

    }

    return mem;
}

int SaveFile(char *path, char *mem, int file_size)
{
    FILE *fp;
   
    fp = fopen(path, "wb");

	if (fp != NULL) {
       
		fwrite((void *) mem, 1, file_size, fp);

		fclose(fp);

    } else return -1;

    sysLv2FsChmod(path, FS_S_IFMT | 0777);

    return 0;
}

u8 game_category[3]="??";

int parse_param_sfo(char * file, char *title_name)
{
    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;
    
    //*title_name = 0;

    if(!sysLv2FsOpen(file, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0))
    {
        unsigned len, pos, str;
        unsigned char *mem=NULL;

        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        mem= (unsigned char *) malloc(len+16);
        if(!mem) {
            sysLv2FsClose(fd);
            return -2;
        }

        memset(mem, 0, len+16);

        sysLv2FsLSeek64(fd, 0, 0, &position);
        
        if(sysLv2FsRead(fd, mem, len, &bytes)!=0)
            bytes =0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd); 

        str= (mem[8]+(mem[9]<<8));
        pos=(mem[0xc]+(mem[0xd]<<8));

        int indx=0;
        int ct=0;

        while(str<len)
        {
            if(mem[str]==0)
                break;
            
            if(!strcmp((char *) &mem[str], "TITLE"))
            {
                strncpy(title_name, (char *) &mem[pos], 63);
                ct++;
            }
            else if(!strcmp((char *) &mem[str], "CATEGORY")) {
                strncpy(title_name, (char *) &mem[pos], 63);
                ct++;
                memcpy((char *) game_category, (char *) &mem[pos], 2);
            }
            if(ct==2) {
                free(mem);
                return 0;
            }
            while(mem[str])
                str++;
            str++;
            pos+=(mem[0x1c+indx]+(mem[0x1d+indx]<<8));
            indx+=16;
        }

        if(mem)
            free(mem);
    }

    return -1;

}

int parse_param_sfo_id(char * file, char *title_id)
{
	Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;
	
    strncpy(title_id, "UNKNOWN", 63);

	if(!sysLv2FsOpen(file, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0))
		{
		unsigned len, pos, str;
		unsigned char *mem=NULL;

        sysLv2FsLSeek64(fd, 0, 2, &position);
		len = (u32) position;

		mem= (unsigned char *) malloc(len+16);
		if(!mem) {sysLv2FsClose(fd);return -2;}

		memset(mem, 0, len+16);

		sysLv2FsLSeek64(fd, 0, 0, &position);
		
        if(sysLv2FsRead(fd, mem, len, &bytes)!=0) bytes =0LL;

        len = (u32) bytes;

		sysLv2FsClose(fd); 

		str= (mem[8]+(mem[9]<<8));
		pos=(mem[0xc]+(mem[0xd]<<8));

		int indx=0;

		while(str<len)
			{
			if(mem[str]==0) break;
			
			if(!strcmp((char *) &mem[str], "TITLE_ID"))
				{
                memcpy(title_id, (char *) &mem[pos], 4);
                title_id[4] = '-';
				strncpy(&title_id[5], (char *) &mem[pos + 4], 58);
				free(mem);
				return 0;
				}
			while(mem[str]) str++;str++;
			pos+=(mem[0x1c+indx]+(mem[0x1d+indx]<<8));
			indx+=16;
			}
		if(mem) free(mem);
        
		}

	
	return -1;

}

int parse_ps3_disc(char *path, char * id)
{
	int n;
	
	Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;

    strncpy(id, "UNKNOWN", 63);
	
	if(!sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0))
		{
		unsigned len;
		unsigned char *mem=NULL;

		sysLv2FsLSeek64(fd, 0, 2, &position);
		len = (u32) position;

		mem= (unsigned char *) malloc(len+16);
		if(!mem) {sysLv2FsClose(fd);return -2;}

		memset(mem, 0, len+16);

		sysLv2FsLSeek64(fd, 0, 0, &position);

		if(sysLv2FsRead(fd, mem, len, &bytes)!=0) bytes =0LL;

        len = (u32) bytes;

		sysLv2FsClose(fd);

		for(n=0x20;n<0x200;n+=0x20)
			{
			if(!strcmp((char *) &mem[n], "TITLE_ID"))
				{
				n= (mem[n+0x12]<<8) | mem[n+0x13];
				memcpy(id, &mem[n], 16);

				return 0;
				}
			}
		}

	return -1;

}

int unlink_secure(void *path)
{
//    struct stat s;
    sysFSStat s;
    if(sysLv2FsStat(path, &s)>=0) {
        sysFsChmod(path, FS_S_IFMT | 0777);
        return sysLv2FsUnlink(path);
    }
    return -1;
}

int mkdir_secure(void *path)
{
    int ret = -1;
    DIR  *dir = opendir (path);
    if(!dir) {
        ret= mkdir(path, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
    } else closedir(dir);

    return ret;
}

int rmdir_secure(void *path)
{
    int ret = -1;
    DIR  *dir = opendir (path);
    if(dir) {
        closedir(dir);
        sysFsChmod(path, FS_S_IFDIR | 0777);
        ret= sysLv2FsRmdir(path);
    }

    return ret;
}


void utf8_to_ansi(char *utf8, char *ansi, int len)
{
u8 *ch= (u8 *) utf8;
u8 c;

    *ansi = 0;

	while(*ch!=0 && len>0){

	// 3, 4 bytes utf-8 code 
	if(((*ch & 0xF1)==0xF0 || (*ch & 0xF0)==0xe0) && (*(ch+1) & 0xc0)==0x80){

	*ansi++=' '; // ignore
	len--;
	ch+=2+1*((*ch & 0xF1)==0xF0);
	
	}
	else 
	// 2 bytes utf-8 code	
	if((*ch & 0xE0)==0xc0 && (*(ch+1) & 0xc0)==0x80){
	
	c= (((*ch & 3)<<6) | (*(ch+1) & 63));

	if(c>=0xC0 && c<=0xC5) c='A';
	else if(c==0xc7) c='C';
	else if(c>=0xc8 && c<=0xcb) c='E';
	else if(c>=0xcc && c<=0xcf) c='I';
	else if(c==0xd1) c='N';
	else if(c>=0xd2 && c<=0xd6) c='O';
	else if(c>=0xd9 && c<=0xdc) c='U';
	else if(c==0xdd) c='Y';
	else if(c>=0xe0 && c<=0xe5) c='a';
	else if(c==0xe7) c='c';
	else if(c>=0xe8 && c<=0xeb) c='e';
	else if(c>=0xec && c<=0xef) c='i';
	else if(c==0xf1) c='n';
	else if(c>=0xf2 && c<=0xf6) c='o';
	else if(c>=0xf9 && c<=0xfc) c='u';
	else if(c==0xfd || c==0xff) c='y';
	else if(c>127) c=*(++ch+1); //' ';

	*ansi++=c;
	len--;
	ch++;
	
	}
	else {
	
	if(*ch<32) *ch=32;
	*ansi++=*ch;
	
	len--;

	}

	ch++;
	}
	while(len>0) {
	*ansi++=0;
	len--;
	}
}

void sort_entries(t_directories *list, int *max)
{
	int n,m;
	int fi= (*max);
	

	for(n=0; n< (fi -1);n++)
		for(m=n+1; m< fi ;m++)
			{
			if((strcasecmp(list[n].title, list[m].title)>0  && ((list[n].flags | list[m].flags) & 2048)==0) || 
				((list[m].flags & 2048) && n==0))
				{
				t_directories swap;
					swap=list[n];list[n]=list[m];list[m]=swap;
				}
			}
}

void delete_entries(t_directories *list, int *max, u32 flag)
{
    int n;

    n=0;
    flag &= GAMELIST_FILTER; // filter entries
    while(n<(*max) )
    {
        if(list[n].flags & flag)
        {
            if((*max) >1)
            {
                list[n].flags=0;
                list[n]=list[(*max) -1];
                (*max) --;
            } else if((*max) == 1) {
                (*max) --;
                break;
            }
            
        } else
            n++;
    }
}

void fill_entries_from_device(char *path, t_directories *list, int *max, u32 flag, int sel)
{
    DIR  *dir;
    char file[0x420];
    char pattern[128];
    if(sel == GAMEBASE_MODE)
        strcpy(pattern, "%s/PS3_GAME/PARAM.SFO");
    else if(sel == HOMEBREW_MODE)
        strcpy(pattern, "%s/PARAM.SFO");
    
    delete_entries(list, max, flag & GAMELIST_FILTER);

    if((*max) <0)
        *max =0;

    if(*max >= MAX_DIRECTORIES)
                return;

    // add PSX Games
    if(sel == GAMEBASE_MODE && game_list_category != 1) {
        int n;
        strncpy(file, path, 0x420);
        n=1;while(file[n]!='/' && file[n]!=0)  n++;
        
        file[n]=0; strcat(&file[n], "/PSXGAMES");

        dir = opendir (file);
        if(dir) {
            while(1) {
            
            struct dirent *entry=readdir (dir);
            
                
            if(!entry) break;
            if(entry->d_name[0]=='.') continue;

            if(!(entry->d_type & DT_DIR)) continue;

            list[*max ].flags=flag  | (1<<23);

            strncpy(list[*max ].title, entry->d_name, 63);
            list[*max ].title[63]=0;

            strncpy(list[*max ].title_id, entry->d_name, 63);
            list[*max ].title_id[63]=0;

            sprintf(list[*max ].path_name, "%s/%s", file, entry->d_name);
            list[*max ].splitted = 0;

            if(!(flag & 1)) { // is not HDD
                struct stat s;
                char file2[0x420];
                char file3[0x420];

                sprintf(file2, "%s/VM1/%s/Internal_MC.VM1", self_path, list[*max ].title);
                sprintf(file3, "%s/Internal_MC.VM1", list[*max ].path_name);
                
                if(stat(file2, &s)>=0) {
                    
                    unlink_secure(file3);
                    if(copy_async(file2, file3, s.st_size, "Copying Memory Card to USB device...", NULL)<0) {
                        unlink_secure(file3); // delete possible truncated file
                        DrawDialogOK("Error copying the Memory Card to USB device");
                    } else unlink_secure(file2);
                    
                    sprintf(file2,"%s/VM1/%s", self_path, list[*max ].title);
                    rmdir_secure(file2);
                }
            }
            
            (*max) ++;
            if(*max >= MAX_DIRECTORIES)
                break;

            }

        closedir (dir);
        }
    }

    if(*max >= MAX_DIRECTORIES)
                return;

    if(!(flag & (1<<11)) && sel != HOMEBREW_MODE &&  game_list_category == 2) return;

    dir = opendir (path);
    if(!dir)
        return;

    while(1)
    {
        struct dirent *entry=readdir (dir);
        
            
        if(!entry) break;
        if(entry->d_name[0]=='.') continue;

        if(!(entry->d_type & DT_DIR)) continue;
        
        list[*max ].flags=flag;

        strncpy(list[*max ].title, entry->d_name, 63);
        list[*max ].title[63]=0;

        sprintf(list[*max ].path_name, "%s/%s", path, entry->d_name);

        if((sel == GAMEBASE_MODE) || (sel == HOMEBREW_MODE))
        {
            // read name in PARAM.SFO
            sprintf(file, pattern, list[*max ].path_name);

            //add splitted icon
            if(list[*max ].title[0]=='_')
                list[*max ].splitted = 1;
            else
                list[*max ].splitted = 0;

            //get tittle from sfo
            parse_param_sfo(file, list[*max ].title);

            // ignore some games for homebrew
            if((sel==HOMEBREW_MODE && game_category[0]!='H' && game_category[0]!='A') && 
               (game_category[0]!='C' && game_category[1]!='B'))
                continue;

            list[*max ].title[63]=0;

            sprintf(file, "%s/%s",  list[*max ].path_name, "PS3_DISC.SFB" );
            if( (sel == HOMEBREW_MODE) || 
               ((sel == GAMEBASE_MODE) && (parse_ps3_disc((char *) file, list[*max ].title_id)<0))) {
                    sprintf(file, pattern, list[*max ].path_name);
                    parse_param_sfo_id(file, list[*max ].title_id); // build de ID from param.sfo
            }
            list[*max ].title_id[63]=0;
            
        }
        else
        {
            struct stat s;
            sprintf(file, "%s/EBOOT.BIN", list[*max ].path_name);
            if(stat(file, &s)<0)
                continue;
        }

        (*max) ++;

        if(*max >= MAX_DIRECTORIES)
            break;

    }
    closedir (dir);
}

/*******************************************************************************************************************************************************/
/* CONSOLE DEBUG                                                                                                                                       */
/*******************************************************************************************************************************************************/

#define CONSOLE_WIDTH		(84)
#define CONSOLE_HEIGHT		(27)

static char dbg_str1[128];
static char dbg_str2[128];

static char dbg_data[128 * CONSOLE_HEIGHT];

int con_x = 0, con_y =0;

void initConsole()
{
    con_x = 0; con_y =0;
    dbg_str1[0] = dbg_str2[0] = 0;
    memset(dbg_data, 0, 128 * CONSOLE_HEIGHT);
}

static char buff[4096];

void DbgHeader(char *str)
{
    strncpy(dbg_str1, str, 128);
}

void DbgMess(char *str)
{
    strncpy(dbg_str2, str, 128);
}


void DbgDraw()
{
    int n;

    cls2();

    SetFontColor(0x0fcf2fff, 0x00000000);

    SetFontAutoCenter(0);
    SetCurrentFont(FONT_BUTTON);
    SetFontSize(10, 16);

    
    for(n = 0; n < CONSOLE_HEIGHT; n++) {
       DrawString(0, 56 + n * 16, &dbg_data[128 * n]);
    }
    
    SetFontColor(0xffffffff, 0x00000000);
    SetCurrentFont(FONT_BUTTON);

    SetFontSize(14, 32);
    SetFontAutoCenter(1);

    DrawString(0, 16, dbg_str1);

    DrawString(0, 480, dbg_str2);

    SetFontAutoCenter(0);


}

void DPrintf(char *format, ...)
{
    char *str = (char *) buff;
    va_list	opt;
	
	va_start(opt, format);
	vsprintf( (void *) buff, format, opt);
	va_end(opt);

    while(*str) {
        if(*str == '\n') {
            con_y++;
            con_x = 0;
            if(con_y >= CONSOLE_HEIGHT) {
                con_y = CONSOLE_HEIGHT - 1;
                memcpy(dbg_data, dbg_data + 128, 128 * (CONSOLE_HEIGHT -1));
                dbg_data[128 * (CONSOLE_HEIGHT -1)] = 0;
            } else dbg_data[128 * con_y + con_x] = 0;
        } else {
            if(con_x < CONSOLE_WIDTH) {
                dbg_data[128 * con_y + con_x] = *str;
                dbg_data[128 * con_y + con_x + 1] = 0;
                con_x++;
            } else {
            con_y++;
            con_x = 0;
            if(con_y >= CONSOLE_HEIGHT) {
                con_y = CONSOLE_HEIGHT - 1;
                memcpy(dbg_data, dbg_data + 128, 128 * (CONSOLE_HEIGHT -1));
                dbg_data[128 * (CONSOLE_HEIGHT -1)] = 0;
               
            }
            
            dbg_data[128 * con_y + con_x] = *str;
            dbg_data[128 * con_y + con_x + 1] = 0;
            con_x++;
            }
        }

        str++;
    }

    DbgDraw();
    tiny3d_Flip();
}

char *getfilename_part(char *path)
{
    int len = strlen(path) -1;

    while(len > 0) {if(path[len] == '/') {len++;break;};len--;}
 
    return &path[len];
}


/*******************************************************************************************************************************************************/
/* FAST FILES                                                                                                                                          */
/*******************************************************************************************************************************************************/

static int copy_split_to_cache = 0;

static int file_counter=0; // to count files

static time_t time_start; // time counter init

static char string1[256];

static int abort_copy=0; // abort process

static int copy_mode=0; // 0- normal 1-> pack files >= 4GB

static int copy_is_split=0; // return 1 if files is split

static s64 global_device_bytes=0;

#define MAX_FAST_FILES 16

typedef struct _t_fast_files
{
	s64 readed; // global bytes readed
	s64 writed; // global bytes writed
	s64 off_readed; // offset correction for bigfiles_mode == 2  (joining)
	s64 len;    // global len of the file (value increased in the case of bigfiles_ mode == 2)

	int giga_counter; // counter for split files to 1GB for bigfiles_mode == 1 (split)
	u32 fl; // operation control
	int bigfile_mode;
	int pos_path; // filename position used in bigfiles

	char pathr[0x420]; // read path 
	char pathw[0x420]; // write path


	int use_doublebuffer; // if files >= 4MB use_doblebuffer =1;

	void *mem; // buffer for read/write files ( x2 if use_doublebuffer is fixed)
	int size_mem; // size of the buffer for read

	int number_frag; // used to count fragments files in bigfile_mode

	sysFSAio t_read;  // used for async read
	sysFSAio t_write; // used for async write

} t_fast_files __attribute__((aligned(8)));

static t_fast_files *fast_files=NULL;

static int fast_num_files=0;

static int fast_used_mem=0;

static int current_fast_file_r=0;
static int current_fast_file_w=0;

static int fast_read=0, fast_writing=0;

static int files_opened=0;


static int total_fast_files = 0;

static int fast_copy_async(char *pathr, char *pathw, int enable)
{
   
	fast_num_files = 0;

	fast_read    = 0;
	fast_writing = 0;

	fast_used_mem = 0;
	files_opened  = 0;

	current_fast_file_r = current_fast_file_w = 0;

	if(enable) {

		if(sysFsAioInit(pathr)!= 0)  return -1;
		if(sysFsAioInit(pathw)!= 0)  return -1;

		fast_files= (t_fast_files *) memalign(8, sizeof(t_fast_files) * (MAX_FAST_FILES));
		if(!fast_files) return -2;
       
        
        memset((void *) fast_files, 0, sizeof(t_fast_files) * (MAX_FAST_FILES));

		return 0;

    } else {

		if(fast_files) {

            free(fast_files);
            fast_files = NULL;
            
        }

		sysFsAioFinish(pathr);
		sysFsAioFinish(pathw);
    }

    return 0;

}

static int fast_copy_process();

#define MAX_FILECACHED 4

static int nfilecached = 0;
static s64 filecached_bytes[MAX_FILECACHED];
static char filecached[MAX_FILECACHED][2][0x420];

static char * path_cache = NULL;

static s64 copy_total_size = 0;

static int fast_copy_add(char *pathr, char *pathw, char *file)
{
	int size_mem;
    
	int strl = strlen(file);

	sysFSStat s;
					
	if(fast_num_files >= MAX_FAST_FILES || fast_used_mem >= 0x400000) {

	int ret = fast_copy_process();

		if(ret < 0 || abort_copy) {
            DPrintf("%s%i\n", language[FASTCPADD_FAILED], ret);
            return ret;
        }

	}

	if(fast_num_files >= MAX_FAST_FILES) {DPrintf("%s\n", language[FASTCPADD_ERRTMFILES]); return -1;}
	
	fast_files[fast_num_files].bigfile_mode = 0;

	if(strl > 6) {

		char *p = file;
		p+= strl - 6; // adjust for .666xx
		if(p[0] == '.' && p[1] == '6' && p[2] == '6' && p[3] == '6') {  
			if(p[4] != '0' ||  p[5] != '0')  {return 0;} // ignore this files
   			fast_files[fast_num_files].bigfile_mode = 2; // joining split files
					
		}
		
	}

    if(strl > 7) {

		char *p = file;
		p+= strl - 7; // adjust for .666xx
		if(p[0] == '.'  && p[2] == '.' && p[3] == 'p') {  
			if(p[4] == 'a' ||  p[5] == 'r' ||  p[6] == 't')  {return 0;} // ignore this files
   				
		}
		
	}

    // test if exists .1.part
    sprintf(buff,"%s/%s.1.part", pathr, file);
    
    if(sysFsStat(buff, &s)==0) {
        fast_files[fast_num_files].bigfile_mode = 3; // joining split files
    }
	
    if(copy_split_to_cache && fast_files[fast_num_files].bigfile_mode != 2 && fast_files[fast_num_files].bigfile_mode != 3) return 0;
    
    
    sprintf(fast_files[fast_num_files].pathr, "%s/%s", pathr, file);

	if(sysFsStat(fast_files[fast_num_files].pathr, &s) < 0) 
        {DPrintf("%s\n", language[FASTCPADD_FAILEDSTAT]); abort_copy = 1; return -1;}
    
    if(copy_split_to_cache)
    {
        if(nfilecached <= 0) return 0;

        if(fast_files[fast_num_files].bigfile_mode == 3) {

            sprintf(buff, "%s/%s", pathr, file);

            int n;

            for(n = 0; n < nfilecached; n++)
                if(!strcmp(&filecached[n][0][0], buff)) break;
            
            if(n == nfilecached) return 0;

            sprintf(fast_files[fast_num_files].pathw, "%s/%s", path_cache, file);
           
        } else {

            sprintf(buff, "%s/%s", pathr, file);

            char * a = strstr((char *) buff, ".66600");
            if(a) a[0] = 0;

            int n;

            for(n = 0; n < nfilecached; n++)
                if(!strcmp(&filecached[n][0][0], buff)) break;
            
            if(n == nfilecached) return 0;

            sprintf(fast_files[fast_num_files].pathw, "%s/%s", path_cache, file);
        }

    } else
	    sprintf(fast_files[fast_num_files].pathw, "%s/%s", pathw, file);

	// zero files
	if((s64) s.st_size == 0LL) {
		int fdw;

		if(sysFsOpen(fast_files[fast_num_files].pathw, SYS_O_CREAT | SYS_O_TRUNC | SYS_O_WRONLY, &fdw, 0,0) != 0) {
			DPrintf("%s:\n%s\n\n", language[FASTCPADD_ERROPEN], fast_files[current_fast_file_r].pathw);
			abort_copy = 1;
			return -1;
	    }
	
        sysFsClose(fdw);

		sysFsChmod(fast_files[fast_num_files].pathw, FS_S_IFMT | 0777);
		
        DPrintf("%s %s\nw%s 0 B\n", language[FASTCPADD_COPYING], fast_files[current_fast_file_r].pathr, language[GLUTIL_WROTE]);
		
        file_counter++;
		
        return 0;
	}

	if(fast_files[fast_num_files].bigfile_mode == 2) {

		fast_files[fast_num_files].pathw[strlen(fast_files[fast_num_files].pathw)-6] = 0; // truncate the extension
		fast_files[fast_num_files].pos_path=strlen(fast_files[fast_num_files].pathr) - 6;
		fast_files[fast_num_files].pathr[fast_files[fast_num_files].pos_path] = 0; // truncate the extension

	}

    if(fast_files[fast_num_files].bigfile_mode == 3) {

		//fast_files[fast_num_files].pathw[strlen(fast_files[fast_num_files].pathw)-6] = 0; // truncate the extension
		fast_files[fast_num_files].pos_path=strlen(fast_files[fast_num_files].pathr);
		//fast_files[fast_num_files].pathr[fast_files[fast_num_files].pos_path] = 0; // truncate the extension

	}
	
	if(copy_mode == 1) {
		if(((s64) s.st_size) >= 0x100000000LL) {
			fast_files[fast_num_files].bigfile_mode = 1;
			fast_files[fast_num_files].pos_path     = strlen(fast_files[fast_num_files].pathw);
			fast_files[fast_num_files].giga_counter = 0;
			
			copy_is_split = 1;
		}
		
	}
	

	fast_files[fast_num_files].number_frag = 0;
	fast_files[fast_num_files].fl = 1;

	fast_files[fast_num_files].len = (s64) s.st_size;
	fast_files[fast_num_files].use_doublebuffer = 0;
	fast_files[fast_num_files].readed = 0LL;
	fast_files[fast_num_files].writed = 0LL;

	fast_files[fast_num_files].t_read.fd  = -1;
	fast_files[fast_num_files].t_write.fd = -1;

	if(((s64) s.st_size) >= 0x200000LL) {
		size_mem = 0x200000;
		fast_files[fast_num_files].use_doublebuffer = 1;
	} else size_mem = ((int) s.st_size);

    
	fast_files[fast_num_files].mem = memalign(32, size_mem+size_mem * (fast_files[fast_num_files].use_doublebuffer != 0) + 0x420);
	fast_files[fast_num_files].size_mem = size_mem;

	if(!fast_files[fast_num_files].mem) {DPrintf("%s\n", language[FASTCPADD_FAILFASTFILE]);abort_copy = 1;return -1;}

	fast_used_mem+= size_mem;

	fast_num_files++;

	return 0;
}

void fast_func_read(sysFSAio *xaio, s32 error, s32 xid, u64 size)
{
	t_fast_files* fi = (t_fast_files *) xaio->usrdata;

	if(error != 0 || size != xaio->size) {
		fi->readed = -1; return;
	} else fi->readed += (s64) size;

	fast_read = 2;fi->fl = 3;
	
}

static s64 write_progress = 0;

void fast_func_write(sysFSAio *xaio, s32 error, s32 xid, u64 size)
{
	t_fast_files* fi = (t_fast_files *) xaio->usrdata;

	if(error != 0 || size != xaio->size) {
		fi->writed = -1;
	} else {

		fi->writed += (s64) size;
		fi->giga_counter += (int) size;
		global_device_bytes += (s64) size;
        write_progress = (s64) size;
	}

	fast_writing = 2;
}

int fast_copy_process()
{

	int n;

	int fdr, fdw;

	static int id_r = -1, id_w = -1;

	int error = 0;
    int time_left = 0;

	int i_reading = 0;

	s64 write_end = 0, write_size = 0;
    
    write_progress = 0;

	
	while(current_fast_file_w < fast_num_files || fast_writing) {


		if(abort_copy) break;
		
		// open read
		if(current_fast_file_r < fast_num_files && fast_files[current_fast_file_r].fl == 1 && !i_reading && !fast_read && files_opened < 2) {
			
			fast_files[current_fast_file_r].readed = 0LL;
			fast_files[current_fast_file_r].writed = 0LL;
			fast_files[current_fast_file_r].off_readed = 0LL;

			fast_files[current_fast_file_r].t_read.fd  = -1;
			fast_files[current_fast_file_r].t_write.fd = -1;
				
			if(fast_files[current_fast_file_r].bigfile_mode == 1) {
				
                DPrintf("%s >= 4GB\n %s\n", language[GLUTIL_SPLITFILE], fast_files[current_fast_file_r].pathr);
					sprintf(&fast_files[current_fast_file_r].pathw[fast_files[current_fast_file_r].pos_path], ".666%2.2i",
						fast_files[current_fast_file_r].number_frag);
			}
				
			if(fast_files[current_fast_file_r].bigfile_mode == 2) {
				
                DPrintf("%s >= 4GB\n %s\n", language[FASTCPPRC_JOINFILE], fast_files[current_fast_file_r].pathw);
				sprintf(&fast_files[current_fast_file_r].pathr[fast_files[current_fast_file_r].pos_path], ".666%2.2i",
				fast_files[current_fast_file_r].number_frag);
			}

            if(fast_files[current_fast_file_r].bigfile_mode == 3) {
				
                DPrintf("%s >= 4GB\n %s\n", language[FASTCPPRC_JOINFILE], fast_files[current_fast_file_r].pathw);
                if(fast_files[current_fast_file_r].number_frag!=0) 
				    sprintf(&fast_files[current_fast_file_r].pathr[fast_files[current_fast_file_r].pos_path], ".%i.part",
				    fast_files[current_fast_file_r].number_frag);
			}
			
			//DPrintf("Open R: %s\nOpen W: %s, Index %i/%i\n", fast_files[current_fast_file_r].pathr,
			//	fast_files[current_fast_file_r].pathw, current_fast_file_r, fast_num_files);
            
            int err;
         
           
			err = sysFsOpen(fast_files[current_fast_file_r].pathr, SYS_O_RDONLY, &fdr, 0,0);
                
			if( err != 0) {
				
                DPrintf("Error Opening (read): %x %i\n%s\n\n", err, files_opened, fast_files[current_fast_file_r].pathr);
				error =-1;
				break;

			} else files_opened++;
            
            msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);write_progress = 0;
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, getfilename_part(fast_files[current_fast_file_r].pathw));
			if(sysFsOpen(fast_files[current_fast_file_r].pathw, SYS_O_CREAT | SYS_O_TRUNC | SYS_O_WRONLY, &fdw, 0, 0) != 0) {
				
                DPrintf("Error Opening (write):\n%s\n\n", fast_files[current_fast_file_r].pathw);
				error = -2;
				break;

			} else files_opened++;

			//if(fast_files[current_fast_file_r].bigfile_mode==0) { }
					//DPrintf("Copying %s\n", fast_files[current_fast_file_r].pathr);
			if(fast_files[current_fast_file_r].bigfile_mode && fast_files[current_fast_file_r].bigfile_mode!=3)
				DPrintf("    -> .666%2.2i\n", fast_files[current_fast_file_r].number_frag);

            if(fast_files[current_fast_file_r].bigfile_mode==3) {
			    DPrintf("    -> .%i.part\n", fast_files[current_fast_file_r].number_frag);
            }

            fast_files[current_fast_file_r].t_read.fd = fdr;

            fast_files[current_fast_file_r].t_read.offset = 0LL;
            fast_files[current_fast_file_r].t_read.buffer_addr = (u32) (u64) fast_files[current_fast_file_r].mem;
        
            fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].len - fast_files[current_fast_file_r].readed;

            if((s64) fast_files[current_fast_file_r].t_read.size > fast_files[current_fast_file_r].size_mem)
                fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].size_mem;

            fast_files[current_fast_file_r].t_read.usrdata = (u64 )&fast_files[current_fast_file_r];

            fast_files[current_fast_file_r].t_write.fd = fdw;
            fast_files[current_fast_file_r].t_write.usrdata = (u64 )&fast_files[current_fast_file_r];
            fast_files[current_fast_file_r].t_write.offset = 0LL;
            if(fast_files[current_fast_file_r].use_doublebuffer)
                fast_files[current_fast_file_r].t_write.buffer_addr = 
                    (u32) (u64) (((char *) fast_files[current_fast_file_r].mem) + fast_files[current_fast_file_r].size_mem);
            else
                fast_files[current_fast_file_r].t_write.buffer_addr = (u32) (u64) fast_files[current_fast_file_r].mem;

			fast_read = 1; fast_files[current_fast_file_r].fl = 2;
				
            if(sysFsAioRead(&fast_files[current_fast_file_r].t_read, &id_r, fast_func_read) != 0) {
				id_r  = -1;
				error = -3;
				DPrintf("Fail to perform Async Read\n\n");
				fast_read = 0;
				break;
			}

			i_reading = 1;

		}

		// fast read end

            if(current_fast_file_r < fast_num_files && fast_files[current_fast_file_r].fl == 3 && !fast_writing) {
                id_r = -1;
                
                if(fast_files[current_fast_file_r].readed < 0LL) {
                    DPrintf("Error Reading %s\n", fast_files[current_fast_file_r].pathr);
                    error = -3;
                    break;
                }
                
                // double buffer

                if(fast_files[current_fast_file_r].use_doublebuffer) {
                    //DPrintf("Double Buff Write\n");
                        
                    current_fast_file_w = current_fast_file_r;
                    
                    memcpy(((char *) fast_files[current_fast_file_r].mem) + fast_files[current_fast_file_r].size_mem,
                    fast_files[current_fast_file_r].mem, fast_files[current_fast_file_r].size_mem);

                    fast_files[current_fast_file_w].t_write.size = fast_files[current_fast_file_r].t_read.size;
                    
                    if(fast_files[current_fast_file_w].bigfile_mode == 1)
                        fast_files[current_fast_file_w].t_write.offset = (s64) fast_files[current_fast_file_w].giga_counter;
                    else
                        fast_files[current_fast_file_w].t_write.offset = fast_files[current_fast_file_w].writed;
                    
                    fast_writing = 1;

                    if(sysFsAioWrite(&fast_files[current_fast_file_w].t_write, &id_w, fast_func_write) != 0) {
                    
                        id_w  = -1;
                        error = -4;
                        DPrintf("Fail to perform Async Write\n\n");
                        fast_writing = 0;
                        break;
                    }

                    if(fast_files[current_fast_file_r].readed < fast_files[current_fast_file_r].len) {
                        
                        fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].len - fast_files[current_fast_file_r].readed;
                        
                        if((s64) fast_files[current_fast_file_r].t_read.size > fast_files[current_fast_file_r].size_mem)
                                fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].size_mem;

                        fast_files[current_fast_file_r].fl = 2;
                        fast_files[current_fast_file_r].t_read.offset = fast_files[current_fast_file_r].readed - fast_files[current_fast_file_r].off_readed;

                        fast_read = 1;
                        
                        if(sysFsAioRead(&fast_files[current_fast_file_r].t_read, &id_r, fast_func_read) != 0) {
                            id_r = -1;
                            error = -3;
                            
                            DPrintf("Fail to perform Async Read\n\n");
                            
                            fast_read = 0;
                            break;
                        }
                } else {

                    if(fast_files[current_fast_file_r].bigfile_mode == 2 || fast_files[current_fast_file_r].bigfile_mode == 3) {
                        
                        sysFSStat s;
                                
                        fast_files[current_fast_file_r].number_frag++;

                        if(fast_files[current_fast_file_r].number_frag)
                            msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);

                        fast_files[current_fast_file_r].off_readed = fast_files[current_fast_file_r].readed;

                        if(fast_files[current_fast_file_r].bigfile_mode == 3) {
                            DPrintf("    -> .%i.part\n", fast_files[current_fast_file_r].number_frag);
                            
                            sprintf(&fast_files[current_fast_file_r].pathr[fast_files[current_fast_file_r].pos_path], ".%i.part",
                            fast_files[current_fast_file_r].number_frag);
                        } else {
                            DPrintf("    -> .666%2.2i\n", fast_files[current_fast_file_r].number_frag);
                      
                            sprintf(&fast_files[current_fast_file_r].pathr[fast_files[current_fast_file_r].pos_path], ".666%2.2i",
                            fast_files[current_fast_file_r].number_frag);
                        }

                        if(sysFsStat(fast_files[current_fast_file_r].pathr, &s) < 0) {current_fast_file_r++; i_reading = 0;}
                        else {
                            if(fast_files[current_fast_file_r].t_read.fd >= 0) {
                                 int r = sysFsClose(fast_files[current_fast_file_r].t_read.fd); 
                                 if(r<0) {
                                    DPrintf("Error en Close (.666) %x\n", (u32) r);
                                    error = -4;
                                    break;
                                 }

                                 fast_read = 0;

                                 files_opened--;
                            }
                        
                        fast_files[current_fast_file_r].t_read.fd = -1;
                        
                        int err =sysFsOpen(fast_files[current_fast_file_r].pathr, SYS_O_RDONLY, &fdr, 0, 0);
                        if(err != 0) {
                            
                            DPrintf("Error Opening (read): %x\n%s\n\n", err, fast_files[current_fast_file_r].pathr);
                            error = -1;
                            break;
                        } else files_opened++;
                     

                        fast_files[current_fast_file_r].t_read.fd = fdr;
                        fast_files[current_fast_file_r].len += (s64) s.st_size;
                        fast_files[current_fast_file_r].t_read.offset = 0LL;
                        fast_files[current_fast_file_r].t_read.buffer_addr = (u32) (u64) fast_files[current_fast_file_r].mem;

                        fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].len-fast_files[current_fast_file_r].readed;
                        if((s64) fast_files[current_fast_file_r].t_read.size > fast_files[current_fast_file_r].size_mem)
                            fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].size_mem;

                        fast_files[current_fast_file_r].t_read.usrdata = (u64 )&fast_files[current_fast_file_r];

                        fast_read = 1;

                        if(sysFsAioRead(&fast_files[current_fast_file_r].t_read, &id_r, fast_func_read) != 0) {
                            
                            id_r  = -1;
                            error = -3;
                            DPrintf("Fail to perform Async Read\n\n");
                            fast_read = 0;
                            break;
                        }

                    fast_files[current_fast_file_r].fl = 2;

                    }
                
                } else {fast_files[current_fast_file_r].fl = 5; current_fast_file_r++; i_reading = 0;}
                                                  
            }

        } else {
            // single buffer	
                
            //DPrintf("Single Buff Write\n");

            current_fast_file_w = current_fast_file_r;
            fast_files[current_fast_file_w].t_write.size = fast_files[current_fast_file_r].t_read.size;

            fast_files[current_fast_file_w].t_write.offset = fast_files[current_fast_file_w].writed;
            
            fast_writing = 1;

                if(sysFsAioWrite(&fast_files[current_fast_file_w].t_write, &id_w, fast_func_write) != 0) {

                    id_w  = -1;
                    error = -4;
                    DPrintf("Fail to perform Async Write\n\n");
                    fast_writing = 0;
                    break;
                }
                
                
                current_fast_file_r++;
                i_reading = 0;
            }
    
        }
		
	    // fast write end
	    if(fast_writing > 1) {
	    
            fast_writing = 0;
            id_w = -1;

            if(fast_files[current_fast_file_w].writed < 0LL) {

                DPrintf("Error Writing %s\n", fast_files[current_fast_file_w].pathw);
                error = -4;
                break;
            }

            write_end  = fast_files[current_fast_file_w].writed;
            write_size = fast_files[current_fast_file_w].len;

            if(fast_files[current_fast_file_w].writed >= fast_files[current_fast_file_w].len) {

                if(fast_files[current_fast_file_w].t_read.fd >= 0) {
                    int r = sysFsClose(fast_files[current_fast_file_w].t_read.fd);
                    if(r<0) {
                        DPrintf("Error en Close (read) %x\n", (u32) r);
                        error = -4;
                        break;
                    }

                    fast_read = 0;

                    files_opened--;
                    
                }
                
                fast_files[current_fast_file_w].t_read.fd = -1;
                
                if(fast_files[current_fast_file_w].t_write.fd >= 0) {
                    
                    int r= sysFsClose(fast_files[current_fast_file_w].t_write.fd); 
                    if(r<0) {
                        DPrintf("Error en Close (write) %x\n", (u32) r);
                        error = -4;
                        break;
                    }

                    files_opened--;
                    
                    fast_files[current_fast_file_w].t_write.fd = -1;
                    
                    sysFsChmod(fast_files[current_fast_file_w].pathw, FS_S_IFMT | 0777);
                }
                
                

                if(fast_files[current_fast_file_w].bigfile_mode == 1) {

                    fast_files[current_fast_file_w].pathw[fast_files[current_fast_file_w].pos_path] = 0;
                }

                if(write_size < 1024LL)
                    DPrintf("%s %lli B %s\n\n", language[GLUTIL_WROTE], write_size, fast_files[current_fast_file_w].pathw);
                else
                    if(write_size < 0x100000LL) DPrintf("%s %lli KB %s\n\n", language[GLUTIL_WROTE], write_size / 1024LL, fast_files[current_fast_file_w].pathw);
                        else  DPrintf("%s %lli MB %s\n\n", language[GLUTIL_WROTE], write_size / 0x100000LL, fast_files[current_fast_file_w].pathw);

                fast_files[current_fast_file_w].fl = 4; //end of proccess
                
                fast_files[current_fast_file_w].writed = -1LL;
                current_fast_file_w++;
                //if(current_fast_file_r<current_fast_file_w) current_fast_file_w=current_fast_file_r;
                file_counter++;
            } else if(fast_files[current_fast_file_w].bigfile_mode == 1 && fast_files[current_fast_file_w].giga_counter >= 0x40000000) {
                // split big files
                
                if(fast_files[current_fast_file_w].t_write.fd >= 0) {
                    int r = sysFsClose(fast_files[current_fast_file_w].t_write.fd); 
                    if(r<0) {
                        DPrintf("Error en Close (write split) %x\n", (u32) r);
                        error = -4;
                        break;
                    }
                    files_opened--;
                    
                    fast_files[current_fast_file_w].t_write.fd = -1;

                    sysFsChmod(fast_files[current_fast_file_w].pathw, FS_S_IFMT | 0777);
                }
                
                        
                fast_files[current_fast_file_w].giga_counter = 0;
                fast_files[current_fast_file_w].number_frag++;

                sprintf(&fast_files[current_fast_file_w].pathw[fast_files[current_fast_file_w].pos_path], ".666%2.2i",
                    fast_files[current_fast_file_w].number_frag);
                DPrintf("    -> .666%2.2i\n", fast_files[current_fast_file_w].number_frag);
                //msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);write_progress = 0;
                msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, getfilename_part(fast_files[current_fast_file_w].pathw));
                if(sysFsOpen(fast_files[current_fast_file_w].pathw, SYS_O_CREAT | SYS_O_TRUNC | SYS_O_WRONLY, &fdw, 0, 0) != 0) {
                    DPrintf("Error Opening2 (write):\n%s\n\n", fast_files[current_fast_file_w].pathw);
                    error = -2;
                    break;
                } else files_opened++;

                fast_files[current_fast_file_w].t_write.fd = fdw;
            }

				
        }

            
        int seconds = (int) (time(NULL) - time_start);
        //calc time left
        if(!copy_total_size)
        {
            if(progress_action2) {
                abort_copy = 1;
                DPrintf("%s \n", language[GLUTIL_ABORTEDUSER]);
                error = -666;
            }

            if(write_progress != 0) {
                  bar1_countparts += (100.0f*((double) write_progress)) / ((double) copy_total_size);
                  bar2_countparts += (100.0f*((double) write_progress)) / ((double) write_size);

                  write_progress = 0;
            }


            sprintf(string1, "(%i/%i)", file_counter, total_fast_files);
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, string1);

            if(bar1_countparts >= 1.0f) {
                msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (u32) bar1_countparts);
                bar1_countparts-= (float) ((u32) bar1_countparts);
               
            }

            if(bar2_countparts >= 1.0f) {
                msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, (u32) bar2_countparts);
                bar2_countparts-= (float) ((u32) bar2_countparts);
               
            }
            

            sprintf(string1, "%s: %i (%2.2i%%) %s: %2.2i:%2.2i:%2.2i  Vol: %1.2f GB\n", language[FASTCPPRC_COPYFILE], file_counter, 
                    (int)(write_end * 100ULL / write_size), language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60, 
                    ((double) global_device_bytes) / (1024.0* 1024.* 1024.0));
        }
        else
        {
           
            int tleft = ((copy_total_size - global_device_bytes) * seconds) / global_device_bytes;
            if(abs(time_left - tleft) >= 10) //more than 10 secs diff, update time
                time_left = tleft;
            
            if(progress_action2) {
                abort_copy = 1;
                DPrintf("%s \n", language[GLUTIL_ABORTEDUSER]);
                error = -666;
            }

            
            if(write_progress != 0) {
                  bar1_countparts += (100.0f*((double) write_progress)) / ((double) copy_total_size);
                  bar2_countparts += (100.0f*((double) write_progress)) / ((double) write_size);
                  write_progress = 0;
            }

            sprintf(string1, "(%i/%i)", file_counter, total_fast_files);
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, string1);

            if(bar1_countparts >= 1.0f) {
                msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (u32) bar1_countparts);
                bar1_countparts-= (float) ((u32) bar1_countparts);
               
            }

            if(bar2_countparts >= 1.0f) {
                msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, (u32) bar2_countparts);
                bar2_countparts-= (float) ((u32) bar2_countparts);
               
            }
        
            sprintf(string1, "%s: %i (%2.2i%%) %s: %2.2i:%2.2i:%2.2i %1.2f/%1.2f GB\n", language[FASTCPPRC_COPYFILE], file_counter, 
                    (int)(write_end * 100ULL / write_size), language[GLUTIL_TIMELEFT], time_left / 3600, (time_left / 60) % 60, time_left % 60, 
                    ((double) global_device_bytes) / (1024.0* 1024.* 1024.0), ((double) copy_total_size/ (1024.0 * 1024. * 1024.0)));
        }
            
        cls2();

        strcpy(dbg_str1, string1);
        strcpy(dbg_str2, language[GLUTIL_HOLDTRIANGLEAB]);
        DbgDraw();

        tiny3d_Flip();
        
        pad_last_time = 0;

        ps3pad_read();

        if ((new_pad & BUTTON_TRIANGLE)) {

            abort_copy = 1;
            DPrintf("%s \n", language[GLUTIL_ABORTEDUSER]);
            error = -666;
            break;
        }
        
    }

	if(error && error != -666) {
        
		DPrintf(language[FASTCPPTC_OPENERROR], files_opened);
		DPrintf("\n");

		cls2();

        strcpy(dbg_str1, string1);
        strcpy(dbg_str2, language[GLUTIL_HOLDTRIANGLEAB]);
        DbgDraw();

		tiny3d_Flip();

		usleep(20*1000000);
	}


	if(fast_writing == 1 && id_w >= 0) {
		sysFsAioCancel(id_w);
        msgDialogAbort();
		id_w = -1;
		usleep(200000);
	}
	
	fast_writing = 0;

	if(fast_read == 1 && id_r >= 0) {
		
        sysFsAioCancel(id_r);
		id_r =-1;
		usleep(200000);
	}
	
	fast_read = 0;

	for(n = 0; n < fast_num_files; n++) {
		if(fast_files[n].t_read.fd >= 0) {
			
			sysFsClose(fast_files[n].t_read.fd); 
            fast_files[n].t_read.fd = -1;
			files_opened--;
		}
		
        if(fast_files[n].t_write.fd >= 0)	{
			
			sysFsClose(fast_files[n].t_write.fd); 
            fast_files[n].t_write.fd = -1;
			files_opened--;
		}

		if(fast_files[n].mem) free(fast_files[n].mem); fast_files[n].mem = NULL;
	}

	fast_num_files = 0;

	fast_writing = 0;

	fast_used_mem = 0;

	current_fast_file_r = current_fast_file_w = 0;

	if(error) {msgDialogAbort();abort_copy=666; DPrintf("Error at point #1\n");}

	return error;
}

static s64 count_cache_bytes(char *path)
{
    int n = 0;
    s64 bytes = 0;
    
    while(n < 100)
    {
        struct stat s;
        sprintf(buff, "%s.666%2.2i", path, n);

        if(stat(buff, &s)<0)
            break; //end loop

        bytes += s.st_size;
        n++;
    }

    return bytes; //return total bytes counted

}

static int num_directories=0, num_files_big=0, num_files_split=0;

static int my_game_test(char *path)
{
	DIR  *dir;
   dir=opendir (path);
   if(!dir) return -1;
   sysFsChmod(path, FS_S_IFDIR | 0777);

   while(1)
		{
		struct dirent *entry=readdir (dir);
		if(!entry) break;

		if(entry->d_name[0]=='.' && entry->d_name[1]==0) continue;
		if(entry->d_name[0]=='.' && entry->d_name[1]=='.' && entry->d_name[2]==0) continue;

		if((entry->d_type & DT_DIR))
		{
			
            char *d1= (char *) malloc(0x420);
				
            num_directories++;

            if(!d1) {closedir (dir);DPrintf("malloc() Error!!!\n\n");abort_copy=2;return -1;}
            sprintf(d1,"%s/%s", path, entry->d_name);
            my_game_test(d1);
            free(d1);

            if(abort_copy) break;
		}
		else
		{
            char *f= (char *) malloc(0x420);

            struct stat s;
					
            s64 size = 0LL;

            int is_split = 0;

            if(!f) {DPrintf("malloc() Error!!!\n\n");abort_copy=2;closedir (dir);return -1;}
            sprintf(f,"%s/%s", path, entry->d_name);

            if(stat(f, &s)<0) {abort_copy=3;DPrintf("File error!!!\n -> %s\n\n", f);if(f) free(f);break;}

            size= s.st_size;

            if(strlen(entry->d_name)>6)
            {
                char *p= f;
                p+= strlen(f)-6; // adjust for .666xx
                if(p[0]== '.' && p[1]== '6' && p[2]== '6' && p[3]== '6')
                {
                    DPrintf("%s %lli MB %s\n\n", language[GLUTIL_SPLITFILE], size/0x100000LL, f);
                    num_files_split++;
                    is_split = 1;

                    if(copy_split_to_cache && p[4] == '0' && p[5] == '0') 
                    {
                        if(nfilecached < MAX_FILECACHED) 
                        {
                            sprintf(buff, language[GAMETESTS_FOUNDINSTALL], entry->d_name);
                            if(DrawDialogYesNo(buff) == 1)
                            {
                                sprintf(&filecached[nfilecached][0][0], "%s/%s", path, entry->d_name);
                                sprintf(&filecached[nfilecached][1][0], "%s", entry->d_name);

                                char * a = strstr((char *) &filecached[nfilecached][0][0], ".66600");
                                if(a) a[0] = 0;
                                a = strstr((char *) &filecached[nfilecached][1][0], ".66600");
                                if(a) a[0] = 0;
                            
                                filecached_bytes[nfilecached]=count_cache_bytes(&filecached[nfilecached][0][0]);

                                //prepare next
                                nfilecached++;
                            }
                        }
                    }
                }
            }

            if(strlen(entry->d_name)>7)
            {
                char *p= f;
                p+= strlen(f)-7; // adjust for .666xx
                if(p[2]== '.' && p[3]== 'p' &&  p[4]== 'a' && p[5]== 'r' && p[6]== 't')
                {
                    DPrintf("%s %lli MB %s\n\n", language[GLUTIL_SPLITFILE], size/0x100000LL, f);
                    num_files_split++;
                    is_split = 1;

                    if(copy_split_to_cache && p[0] == '.' && p[1] == '1') 
                    {   num_files_split++;
                        if(nfilecached < MAX_FILECACHED) 
                        {
                            sprintf(buff, language[GAMETESTS_FOUNDINSTALL], entry->d_name);
                            if(DrawDialogYesNo(buff) == 1)
                            {
                                sprintf(&filecached[nfilecached][0][0], "%s/%s", path, entry->d_name);
                                sprintf(&filecached[nfilecached][1][0], "%s", entry->d_name);

                                char * a = strstr((char *) &filecached[nfilecached][0][0], ".1.part");
                                if(a) a[0] = 0;
                                a = strstr((char *) &filecached[nfilecached][1][0], ".1.part");
                                if(a) a[0] = 0;
                            
                                filecached_bytes[nfilecached]=count_cache_bytes(&filecached[nfilecached][0][0]);

                                //prepare next
                                nfilecached++;
                            }
                        }
                    }
                }
            }
			
            if(size>=0x100000000LL)	{DPrintf("%s %lli MB %s\n\n", language[GAMETESTS_BIGFILE], size/0x100000LL, f);num_files_big++;}

            if(f) free(f);

            //prepare info for user
            int seconds= (int) (time(NULL)-time_start);

            file_counter++;

            global_device_bytes+=size;

            sprintf(string1,"%s: %i %s: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", language[GAMETESTS_TESTFILE], file_counter,
                    language[GLUTIL_TIME], seconds/3600, (seconds/60) % 60, seconds % 60, 
                    ((double) global_device_bytes)/(1024.0*1024.0*1024.0));
			
            cls2();

            strcpy(dbg_str1, string1);
            strcpy(dbg_str2, language[GLUTIL_HOLDTRIANGLEAB]);
            DbgDraw();

            tiny3d_Flip();

            pad_last_time = 0;
            ps3pad_read();

            if(abort_copy) break;

            if(!copy_split_to_cache && (new_pad & BUTTON_TRIANGLE))
            {
                abort_copy=1;
            }

            if(abort_copy) break;

        }

    }

    closedir (dir);

    return 0;
}

static int my_game_countsize(char *path)
{
    DIR  *dir;
    dir=opendir (path);
    if(!dir) return -1;
    
    DPrintf("count in %s\n", path);
    
    while(1)
    {
		struct dirent *entry=readdir (dir);
		if(!entry) break;

		if(entry->d_name[0]=='.' && entry->d_name[1]==0) continue;
		if(entry->d_name[0]=='.' && entry->d_name[1]=='.' && entry->d_name[2]==0) continue;

		if((entry->d_type & DT_DIR))
		{
			char *d1= (char *) malloc(0x420);
				
			if(!d1) {closedir (dir);DPrintf("malloc() Error!!!\n\n");abort_copy=2;return -1;}
			sprintf(d1,"%s/%s", path, entry->d_name);
			my_game_countsize(d1);
			free(d1);

			if(abort_copy) break;
		}
		else
		{
			char *f= (char *) malloc(0x420);
			struct stat s;
					
			s64 size = 0LL;

			if(!f) {DPrintf("malloc() Error!!!\n\n");abort_copy=2;closedir (dir);return -1;}
			sprintf(f,"%s/%s", path, entry->d_name);

			if(stat(f, &s)<0) {abort_copy=3;DPrintf("File error!!!\n -> %s\n\n", f);if(f) free(f);break;}

			size= s.st_size;
                
			if(f) free(f);
				
   	        file_counter++;
				
		    copy_total_size+=size;
			
		    sprintf(string1,"%s: %i Vol: %1.2f GB\n", language[GAMETESTS_CHECKSIZE], file_counter, ((double) copy_total_size)/(1024.0*1024.*1024.0));
		    cls2();

            strcpy(dbg_str1, string1);
            strcpy(dbg_str2, language[GLUTIL_HOLDTRIANGLESK]);
            DbgDraw();
                
            tiny3d_Flip();

            pad_last_time = 0;
            ps3pad_read();

            if(new_pad & BUTTON_TRIANGLE)
			{
			    abort_copy=1;
			}
			
		    if(abort_copy) break;

		}

	}

	closedir (dir);

return 0;
}

static int my_game_delete(char *path)
{
	DIR  *dir;
	char *f = NULL;
    int seconds;

   dir = opendir (path);
   if(!dir) return -1;
   sysFsChmod(path, FS_S_IFDIR | 0777);

   copy_split_to_cache = 0;
   
   while(1) {
		struct dirent *entry = readdir (dir);
		if(!entry) break;
		
		if(entry->d_name[0] == '.' && entry->d_name[1] == 0) continue;
		if(entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == 0) continue;

		if((entry->d_type & DT_DIR)) {
			
			char *d1 = (char *) malloc(0x420);

			if(!d1) {closedir (dir); DPrintf("malloc() Error!!!\n\n"); abort_copy=2; return -1;}

			sprintf(d1,"%s/%s", path, entry->d_name);
			my_game_delete(d1);

            DPrintf("Deleting <%s>\n\n", path);
            if(sysLv2FsRmdir(d1)) {abort_copy = 3; DPrintf("Deleting Error!!!\n -> <%s>\n\n", entry->d_name); if(d1) free(d1); break;}
			
            free(d1);
			
            if(abort_copy) break;
			
            file_counter--;

			goto display_mess;

		} else{

			f = (char *) malloc(0x420);
			
			if(!f) {DPrintf("malloc() Error!!!\n\n");abort_copy = 2; closedir (dir); return -1;}
			
            sprintf(f, "%s/%s", path, entry->d_name);

			sysFsChmod(f, FS_S_IFMT | 0777);
			if(sysLv2FsUnlink(f)){abort_copy = 3;DPrintf("Deleting Error!!!\n -> %s\n\n", f); if(f) free(f); break;}

			DPrintf("%s %s\n\n", language[GAMEDELFL_DELETED], f);

			if(f) free(f);

			display_mess:

			seconds = (int) (time(NULL) - time_start);
			sprintf(string1,"%s: %i %s: %2.2i:%2.2i:%2.2i\n", language[GAMEDELFL_DELETING], file_counter, 
        			language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60);

			file_counter++;
		
			cls2();

            strcpy(dbg_str1, string1);
            strcpy(dbg_str2, language[GLUTIL_HOLDTRIANGLEAB]);
            DbgDraw();

			tiny3d_Flip();

            pad_last_time = 0;
			ps3pad_read();

			if(abort_copy) break;

			if (new_pad & BUTTON_TRIANGLE) {
				abort_copy = 1;
			}
					
			if(abort_copy) break;

		}

	}

	closedir (dir);

	return 0;
}

static int _my_game_copy(char *path, char *path2)
{
	Lv2FsFile  dir;

	
    if (sysFsOpendir(path, &dir)) {DPrintf("Error in sysFsOpendir()\n"); abort_copy = 7; return -1;}
    
	while(1) {

		sysFSDirent entry;
        u64 read;
        read = sizeof(sysFSDirent);

	    if(sysFsReaddir(dir, &entry, &read) || !read) break;

		if(entry.d_name[0] == '.' && entry.d_name[1] == 0) continue;
		if(entry.d_name[0] == '.' && entry.d_name[1] == '.' && entry.d_name[2] == 0) continue;

       // if(copy_split_to_cache && (entry.d_type & DT_DIR)) continue;

		if((entry.d_type & DT_DIR)) {

			if(abort_copy) break;

			char *d1 = (char *) malloc(0x420);
			char *d2 = (char *) malloc(0x420);

			if(!d1 || !d2) {if(d1) free(d1); if(d2) free(d2); sysFsClosedir(dir); DPrintf("malloc() Error!!!\n\n"); abort_copy = 2; return -1;}

			sprintf(d1,"%s/%s", path, entry.d_name);
			sprintf(d2,"%s/%s", path2, entry.d_name);
			if(!copy_split_to_cache) DPrintf("D1: %s\nD2: %s\n", path, path2);

			//if(strcmp(path2, "/dev_bdvd/PS3_UPDATE") == 0)
			//{
			if(!copy_split_to_cache) mkdir_secure(d2);
			_my_game_copy(d1, d2);
			//}

			free(d1); free(d2);

			if(abort_copy) break;
		} else {
			
            if(strcmp(entry.d_name, "PS3UPDAT.PUP") == 0) {
			
            } else {
				
               if(!copy_split_to_cache) DPrintf("EPATH: %s\nEPATH2: %s\nENTRY %s\n", path, path2, entry.d_name);

				if(fast_copy_add(path, path2, entry.d_name) < 0) {
					
                    abort_copy = 666;
                    DPrintf("Failed in fast_copy_add()\n");
					sysFsClosedir(dir);
					return -1;
				}
			}
	
		}
		
		if(abort_copy) break;
	}

	sysFsClosedir(dir);
	if(abort_copy) return -1;

	return 0;

}


static int my_game_copy(char *path, char *path2)
{

    progress_action2 = 0;

    bar1_countparts = 0.0f;
    bar2_countparts = 0.0f;

    msgDialogOpen2(mdialogprogress2, progress_bar_title, progress_callback, (void *) 0xadef0045, NULL);
    
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, " ");
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, " ");
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);

    sysUtilCheckCallback();tiny3d_Flip();

	global_device_bytes = 0;

	if(fast_copy_async(path, path2, 1) < 0) {abort_copy = 665;  msgDialogAbort(); return -1;}

	int ret= _my_game_copy(path, path2);

	int ret2 = fast_copy_process();

	fast_copy_async(path, path2, 0);

    msgDialogAbort();

	if(ret < 0 || ret2 < 0) return -1;

	return 0;
}


static char filename[0x420];

void copy_from_selection(int game_sel)
{
    copy_split_to_cache = 0;
    copy_total_size = 0LL;
    file_counter = 0;
    abort_copy = 0;

    initConsole();
    my_game_countsize(directories[game_sel].path_name);
    total_fast_files = file_counter;

    if(!(directories[game_sel].flags & 2048)) {
        if(!(directories[game_sel].flags & 1)) { // test for HDD0
            u32 blockSize;
            u64 freeSize;
            u64 free_hdd0;
            sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
            free_hdd0 = ( ((u64)blockSize * freeSize));

            if((copy_total_size + 0x40000000LL) >= (s64) free_hdd0) {
                sprintf(filename, "%s\n\n%s%1.2f GB", "Error: no space in HDD0 to copy it", "You need ", 
                    ((double) (copy_total_size + 0x40000000LL - free_hdd0))/(1024.0*1024.*1024.0));
                DrawDialogOK(filename);
                return;
            }
        }
    }

    if(abort_copy) //abort by user or got an error
    { 
        if(DrawDialogYesNo(language[GAMECPYSL_GSIZEABCNTASK]) != 1)
        {
            forcedevices = 2046;
            return;
        }
        else
        {
            //old mode
            copy_total_size = 0;
        }
    } 

    if(directories[game_sel].flags & 2048)  {copy_from_bluray();return;}

    int n, ret;
    int curr_device = 0;
    char name[0x420];
    int dest = 0;

    dialog_action = 0;
    abort_copy = 0;

    char * hdd_folder2 = hdd_folder;

    char * path_install = __MKDEF_GAMES_DIR;

    if((directories[game_sel].flags >> 23) & 1) {path_install = "PSXGAMES"; hdd_folder2 = "dev_hdd0";}

    if(directories[game_sel].flags & 1) {// is hdd0
   
            
        for(n=1;n<11;n++) {
            
            dialog_action = 0;
            
            if((fdevices >> n) & 1) {
                if(copy_total_size)
                {
                    sprintf(filename, "%s\n\n%s HDD0 %s USB00%c?\nVol: %1.2f GB", directories[game_sel].title, 
                        language[GLUTIL_WANTCPYFROM], language[GLUTIL_WTO], 47 + n,
                        ((double) copy_total_size)/(1024.0*1024.*1024.0)); 
                }
                else
                {
                    sprintf(filename, "%s\n\n%s HDD0 %s USB00%c?", directories[game_sel].title, language[GLUTIL_WANTCPYFROM], language[GLUTIL_WTO], 47 + n); 
                }

                ret = msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*) 0x0000aaaa, NULL );
                
                wait_dialog();
                

                if(dialog_action == 1)  {curr_device = n;break;} // exit
            }
        }
        
      
       dest = n;

       if(dialog_action == 1) {

            u32 blockSize;
            u64 freeSize;
            u64 free_usb;

            sprintf(progress_bar_title, "HDD0 -> USB00%c", 47 + n);

            sprintf(name, "/dev_usb00%c/", 47 + curr_device);
            sysFsGetFreeSize(name, &blockSize, &freeSize);

            free_usb = ( ((u64)blockSize * freeSize));

            if((copy_total_size + 0x100000LL) >= (s64) free_usb) {
                sprintf(filename, "%s\n\n%s%1.2f GB\n\n%s", "Warning: no space in USB to copy it", "You need ",
                    ((double) (copy_total_size + 0x100000LL - free_usb))/(1024.0*1024.*1024.0),  "abort?");
               
                dialog_action = 0;
                
                ret = msgDialogOpen2( mdialogyesno2, filename, my_dialog, (void*) 0x0000aaaa, NULL);

                wait_dialog();

                if(dialog_action == 1)  {return;} else dialog_action = 1; // exit
            }
            
            if((directories[game_sel].flags >> 23) & 1) {
                sprintf(name, "/dev_usb00%c/PSXGAMES", 47 + curr_device);
                mkdir_secure(name);
                sprintf(name, "/dev_usb00%c/PSXGAMES", 47 + curr_device);
                mkdir_secure(name);

                char * p = strstr(directories[game_sel].path_name, "/PSXGAMES");
          
                if(!p) p = "NULL"; else p+= 10;

                sprintf(name, "/dev_usb00%c/%s/%s", 47 + curr_device, path_install, p);
                mkdir_secure(name);

            } else {
                sprintf(name, "/dev_usb00%c/" __MKDEF_GAMES_DIR, 47 + curr_device);
                mkdir_secure(name);
                sprintf(name, "/dev_usb00%c/" __MKDEF_GAMES_DIR, 47 + curr_device);
                mkdir_secure(name);

                char * p = strstr(directories[game_sel].path_name, "/" __MKDEF_GAMES_DIR);
                if(!p) p = strstr(directories[game_sel].path_name, "/GAMES");

                if(!p) p = "NULL"; else p+= 7;

                sprintf(name, "/dev_usb00%c/" __MKDEF_GAMES_DIR "/%s", 47 + curr_device, p);
                mkdir_secure(name);
            }
                
        }

    } else if(fdevices & 1) { //is usb

        for(n = 1; n < 11; n++) {
            if((directories[game_sel].flags >> n) & 1) break;
        }

        if(n == 11) return;

        curr_device = 0;
        
        dest = 0;

        sprintf(filename, "%s\n\n%s USB00%c %s HDD0?\nVol: %1.2f GB", directories[game_sel].title, 
                        language[GLUTIL_WANTCPYFROM], 47 + n, language[GLUTIL_WTO], 
                        ((double) copy_total_size)/(1024.0*1024.*1024.0)); 

        dialog_action = 0;
        ret = msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*)0x0000aaaa, NULL );
            
        wait_dialog();

        if(dialog_action == 1) {

            sprintf(progress_bar_title, "USB00%c -> HDD0", 47 + n); 
           
            char *p;
            if((directories[game_sel].flags >> 23) & 1) {
                p = strstr(directories[game_sel].path_name, "/PSXGAMES");  

                if(!p) p = "NULL"; else p+= 10;

            } else {

                p = strstr(directories[game_sel].path_name, "/" __MKDEF_GAMES_DIR);

                if(!p) p= strstr(directories[game_sel].path_name, "/GAMES");

                if(!p) p = "NULL"; else p+= 7;
            }
            

            while(p[0] == '_') p++; // skip special char

            if(!memcmp(hdd_folder,"dev_hdd0", 9) || ((directories[game_sel].flags >> 23) & 1)){

                sprintf(name, "/%s/%s", hdd_folder2, path_install);	
                mkdir_secure(name);
                sprintf(name, "/%s/%s/%s", hdd_folder2, path_install, p);	
                mkdir_secure(name);

            } else if (!memcmp(hdd_folder,"dev_hdd0_2", 11)){

                sprintf(name, "/%s/GAMES", "dev_hdd0");	
                mkdir_secure(name);
                sprintf(name, "/%s/GAMES/%s", "dev_hdd0", p);	
                mkdir_secure(name);

            } else{

                sprintf(name, "/dev_hdd0/game/%s", hdd_folder);	
                mkdir_secure(name);
                sprintf(name, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR, hdd_folder);	
                mkdir_secure(name);
                sprintf(name, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR "/%s", hdd_folder, p);	
                mkdir_secure(name);
            }
        }
        
    }

    // reset to update datas
    forcedevices = (1 << curr_device);

    if(dialog_action == 1) {
        time_start = time(NULL);

        abort_copy = 0;
        initConsole();
        file_counter = 0;
        new_pad = 0;

        DPrintf("%s %s\n %s %s\n\n", language[GAMECPYSL_STARTED], directories[game_sel].path_name, language[GLUTIL_WTO], name);

        if(curr_device != 0) copy_mode = 1; // break files >= 4GB
        else copy_mode = 0;

        copy_is_split=0;

        my_game_copy((char *) directories[game_sel].path_name, (char *) name);

        cls2();

        int seconds = (int) (time(NULL) - time_start);
        int vflip = 0;
        
        if(!abort_copy) {
                
            char *p; 
            if((directories[game_sel].flags >> 23) & 1) p = strstr(directories[game_sel].path_name, "/PSXGAMES"); 
            else {
                p = strstr(directories[game_sel].path_name, "/" __MKDEF_GAMES_DIR);

                if(!p) p = strstr(directories[game_sel].path_name, "/GAMES");
            }

            if(!p) p = "NULL"; else p+= 7;
            
            if(p[0] == '_') copy_is_split=555; // only rename the game
            while(p[0] == '_') p++; // skip special char

            if(dest == 0) {
                // change to no split!!!
                if(!memcmp(hdd_folder,"dev_hdd0", 9) || ((directories[game_sel].flags >> 23) & 1)) {
                    sprintf(filename, "/%s/%s/%s", hdd_folder2, path_install, p);
                } else if(!memcmp(hdd_folder,"dev_hdd0_2", 11)) {
                    sprintf(filename, "/%s/GAMES/%s", "dev_hdd0", p);
                } else{
                    sprintf(filename, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR "/%s", hdd_folder, p);
                }
            } else {
                if(copy_is_split)
                    sprintf(filename, "/dev_usb00%c/%s/_%s", 47+dest, path_install, p);
                else
                    sprintf(filename, "/dev_usb00%c/%s/%s", 47+dest, path_install, p);
            }
            //DrawDialogOK(name);
            //DrawDialogOK(filename);

            // try rename
            if(copy_is_split)
            ret = sysLv2FsRename(name, filename);

           
            if(dest != 0 && copy_is_split) {
                sprintf(filename, language[GAMECPYSL_SPLITEDUSBNFO], directories[game_sel].title, 47 + curr_device);
            
                dialog_action = 0;

                ret = msgDialogOpen2( mdialogok, filename, my_dialog2, (void*) 0x0000aaab, NULL );
            
                wait_dialog();
            }

        }

        while(1) {
            
            if(abort_copy) sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i\n", language[GLUTIL_ABORTED], 
                                    language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60);
            else {
                    
                sprintf(string1,"%s: %i %s: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", language[GAMECPYSL_DONE], file_counter, 
                        language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60, ((double) global_device_bytes) / (1024.0 * 1024. * 1024.0));
            }

            
            cls2();

            strcpy(dbg_str1, string1);

            if(vflip & 32)
                strcpy(dbg_str2, language[GLUTIL_XEXIT]);
            else
                strcpy(dbg_str2, "");

            vflip++;
                
            DbgDraw();

            tiny3d_Flip();

            ps3pad_read();

            if (new_pad & BUTTON_CROSS) {
            
                new_pad = 0;
                break;
            }

        }

        if(abort_copy) {

            if(dest == 0) sprintf(filename, "%s\n\n%s HDD0?", directories[game_sel].title, language[GAMECPYSL_FAILDELDUMP]);
                else sprintf(filename, "%s\n\n%s USB00%c?", directories[game_sel].title, language[GAMECPYSL_FAILDELDUMP], 47 + dest);

            dialog_action = 0;
            
            ret = msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*) 0x0000aaaa, NULL );
                    
            wait_dialog();

            if(dialog_action == 1) {

                abort_copy = 0;
                time_start = time(NULL);
                file_counter = 0;

                my_game_delete((char *) name);
    
                rmdir_secure((char *) name); // delete this folder

                game_sel = 0;

            } else {
                
                char *p;
                
                if((directories[game_sel].flags >> 23) & 1) p = strstr(directories[game_sel].path_name, "/PSXGAMES");
                else {
                    p = strstr(directories[game_sel].path_name, "/" __MKDEF_GAMES_DIR);

                    if(!p) p = strstr(directories[game_sel].path_name, "/GAMES");
                }

                if(!p) p = "NULL"; else p+= 7;
                
                if(p[0] == '_') p++; // skip special char

                if(dest == 0) {

                    if(!memcmp(hdd_folder,"dev_hdd0", 9) || ((directories[game_sel].flags >> 23) & 1)) {
                        sprintf(filename, "/%s/%s/_%s", hdd_folder2, path_install, p);
                    } else if (!memcmp(hdd_folder,"dev_hdd0_2", 11)) {
                        sprintf(filename, "/%s/GAMES/_%s", "dev_hdd0", p);
                    } else{
                        sprintf(filename, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR "/_%s", hdd_folder, p);
                    }
                } else {
                    sprintf(filename, "/dev_usb00%c/" __MKDEF_GAMES_DIR "/_%s", 47 + dest, p);
                }
                    
                ret = sysLv2FsRename(name, filename);
                
            }
        }

        game_sel = 0;
        
    }
	
}

void copy_from_bluray()
{
    char name[0x420];

    int curr_device = 0;
    sysFSStat status;

    char id[16];
    
    int n, ret;

    dialog_action = 0;
    abort_copy = 0;

    for(n = 0; n < 11;n++) {
        
        dialog_action = 0;
        
        if((fdevices >> n) & 1) {

            if(n == 0) sprintf(filename, "%s\n\n%s BDVD %s HDD0?\nVol: %1.2f GB", bluray_game, 
                                language[GLUTIL_WANTCPYFROM], language[GLUTIL_WTO], ((double) copy_total_size)/(1024.0*1024.*1024.0));
            else sprintf(filename, "%s\n\n%s BDVD %s USB00%c?\nVol: %1.2f GB",  bluray_game, 
                            language[GLUTIL_WANTCPYFROM], language[GLUTIL_WTO], 47 + n, ((double) copy_total_size)/(1024.0*1024.*1024.0));

            ret = msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*) 0x0000aaaa, NULL);
            
            wait_dialog();

            if(dialog_action == 1)  
                { if(n!=0) sprintf(progress_bar_title, "BDVD -> USB00%c", 47 + n);
                  else sprintf(progress_bar_title, "BDVD -> HDD0");
                  curr_device = n; break;
                } // exit
        }
    }

        
    // reset to update datas
    forcedevices = (1 << curr_device);

    if(dialog_action == 1) {
                
            if(curr_device == 0) sprintf(name, "/dev_hdd0");
            else sprintf(name, "/dev_usb00%c", 47 + curr_device);

            if(curr_device == 0) {
                u32 blockSize;
                u64 freeSize;
                u64 free_hdd0;
                sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
                free_hdd0 = ( ((u64)blockSize * freeSize));

                if((copy_total_size + 0x40000000LL) >= (s64) free_hdd0) {
                    sprintf(filename, "%s\n\n%s%1.2f GB", "Error: no space in HDD0 to copy it", "You need ", 
                        ((double) (copy_total_size + 0x40000000LL - free_hdd0))/(1024.0*1024.*1024.0));
                    DrawDialogOK(filename);
                    return;
                }
            } else {
                
                u32 blockSize;
                u64 freeSize;
                u64 free_usb;
                
                sprintf(filename,"%s/", name);
                sysFsGetFreeSize(filename, &blockSize, &freeSize);
                free_usb = ( ((u64)blockSize * freeSize));
                
                if((copy_total_size + 0x100000LL) >= (s64) free_usb) {
                    sprintf(filename, "%s\n\n%s%1.2f GB\n\n%s", "Warning: no space in USB to copy it", "You need ",
                        ((double) (copy_total_size + 0x100000LL - free_usb))/(1024.0*1024.*1024.0), "abort?");

                    dialog_action = 0;

                    ret = msgDialogOpen2( mdialogyesno2, filename, my_dialog, (void*) 0x0000aaaa, NULL);
            
                    wait_dialog();

                    if(dialog_action == 1)  {return;}  // exit
                    else dialog_action = 1;
                }
            }
            
            if (sysFsStat(name, &status) == 0 && !parse_ps3_disc((char *) "/dev_bdvd/PS3_DISC.SFB", id)) {
                
                
                if(curr_device == 0) {
                    if(!memcmp(hdd_folder,"dev_hdd0", 9)) {

                        sprintf(name, "/%s/" __MKDEF_GAMES_DIR, hdd_folder);	
                        mkdir_secure(name);
                        sprintf(name, "/%s/" __MKDEF_GAMES_DIR "/%s", hdd_folder, id);	
                        mkdir_secure(name);

                    } else if(!memcmp(hdd_folder,"dev_hdd0_2", 11)) {

                        sprintf(name, "/%s/GAMES", "dev_hdd0");	
                        mkdir_secure(name);
                        sprintf(name, "/%s/GAMES/%s", "dev_hdd0", id);	
                        mkdir_secure(name);

                    } else {
                        
                        sprintf(name, "/dev_hdd0/game/%s", hdd_folder);	
                        mkdir_secure(name);
                        sprintf(name, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR, hdd_folder);	
                        mkdir_secure(name);
                        sprintf(name, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR "/%s", hdd_folder, id);	
                        mkdir_secure(name);
                    }
                } else {
                    
                    sprintf(name, "/dev_usb00%c/" __MKDEF_GAMES_DIR, 47 + curr_device);
                    mkdir_secure(name);
                    sprintf(name, "/dev_usb00%c/" __MKDEF_GAMES_DIR, 47 + curr_device);
                    mkdir_secure(name);
                    sprintf(name, "/dev_usb00%c/" __MKDEF_GAMES_DIR "/%s", 47 + curr_device, id);
                    mkdir_secure(name);
                }

                time_start = time(NULL);
                abort_copy = 0;
                initConsole();
                file_counter = 0;
                new_pad = 0;
                    
                if(curr_device != 0) copy_mode = 1; // break files >= 4GB
                else copy_mode = 0;

                copy_is_split = 0;

                my_game_copy((char *) "/dev_bdvd", (char *) name);

                int seconds = (int) (time(NULL) - time_start);
                int vflip = 0;

                if(copy_is_split && !abort_copy) {

                    if(curr_device == 0) {

                        if (!memcmp(hdd_folder,"dev_hdd0", 9)) {

                            sprintf(filename, "/%s/" __MKDEF_GAMES_DIR "/_%s", hdd_folder, id);

                        } else if (!memcmp(hdd_folder,"dev_hdd0_2", 11)) {

                            sprintf(filename, "/%s/GAMES/_%s", "dev_hdd0", id);

                        } else {
                            sprintf(filename, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR "/_%s", hdd_folder, id);	
                        }
                    } else {
                            sprintf(filename, "/dev_usb00%c/" __MKDEF_GAMES_DIR "/_%s", 47 + curr_device, id);
                    }
                        
            ret = sysLv2FsRename(name, filename);

            if(curr_device == 0) 
                sprintf(filename, language[GAMECPYSL_SPLITEDHDDNFO], id);
            else
                sprintf(filename, language[GAMECPYSL_SPLITEDUSBNFO], id, 47 + curr_device);

            dialog_action = 0;
            ret = msgDialogOpen2( mdialogok, filename, my_dialog2, (void*) 0x0000aaab, NULL );
            wait_dialog();
            
        }

        while(1) {

            if(abort_copy) sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i\n", language[GLUTIL_ABORTED], 
                                    language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60);
            else {
                sprintf(string1,"%s: %i %s: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", language[GAMECPYSL_DONE], file_counter, 
                        language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60, 
                        ((double) global_device_bytes) / (1024.0 * 1024. * 1024.0));
            }

            cls2();
                        
            strcpy(dbg_str1, string1);

            if(vflip & 32)
                strcpy(dbg_str2, language[GLUTIL_XEXIT]);
            else
                strcpy(dbg_str2, "");

            vflip++;
            
            DbgDraw();

            tiny3d_Flip();
            //pad_last_time = 0;
            ps3pad_read();

            if(new_pad & BUTTON_CROSS) {
                new_pad = 0;
                break;
            }

        }
            
        
        if(abort_copy) {

            if(curr_device == 0)   sprintf(filename, "%s\n\n%s HDD0?", id, language[GAMECPYSL_FAILDELDUMP]);
                else sprintf(filename, "%s\n\n%s USB00%c?", id, language[GAMECPYSL_FAILDELDUMP], 47 + curr_device);

            dialog_action = 0;
            ret = msgDialogOpen2(mdialogyesno, filename, my_dialog, (void*) 0x0000aaaa, NULL );
                    
            wait_dialog();
        
            if(dialog_action == 1) {
                
                time_start = time(NULL);
                file_counter = 0;
                abort_copy = 0;
                my_game_delete((char *) name);

                rmdir_secure((char *) name); // delete this folder

                } else {
                    if(curr_device == 0) {
                        if(!memcmp(hdd_folder,"dev_hdd0", 9)) {
                            sprintf(filename, "/%s/" __MKDEF_GAMES_DIR "/_%s", hdd_folder, id);	
                        } else if(!memcmp(hdd_folder,"dev_hdd0_2", 11)) {
                            sprintf(filename, "/%s/GAMES/_%s", "dev_hdd0", id);	
                        } else {
                            sprintf(filename, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR "/_%s", hdd_folder, id);	
                        }
                    } else {
                        sprintf(filename, "/dev_usb00%c/" __MKDEF_GAMES_DIR "/_%s", 47 + curr_device, id);
                    }
                    
                ret = sysLv2FsRename(name, filename);
                
                }
            }
        }
    }
}

/////////////////

float cache_need_free = 0.0f;

void copy_to_cache(int game_sel, char * hmanager_path)
{
    
    if(directories[game_sel].flags & 2048)  {return;}
    if(directories[game_sel].flags & 1)  {return;}

    int n, ret;

    char name[0x420];
    char name2[0x420];
    int dest = 0;

    dialog_action = 0;
    abort_copy = 0;

   
    for(n = 1; n < 11; n++) {
       if((directories[game_sel].flags >> n) & 1) break;
    }

    if(n == 11) return;

    dest = n;
      
    sprintf(filename, "%s\n\n%s USB00%c %s HDD0 CACHE?", directories[game_sel].title, language[GLUTIL_WANTCPYFROM], 47 + dest, language[GLUTIL_WTO]);
        
    dialog_action = 0;
    ret = msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*)0x0000aaaa, NULL );
            
    wait_dialog();

    if(dialog_action == 1) {

        sprintf(name2, "%s/PS3_GAME", directories[game_sel].path_name);

        path_cache = name;

        nfilecached = 0;
        filecached[nfilecached][0][0] = 0;
        filecached[nfilecached][1][0] = 0;
  
        // reset to update datas
        
        
        time_start = time(NULL);

        abort_copy = 0;
        initConsole();
        file_counter = 0;
        new_pad = 0;

        //////////////
        
        global_device_bytes = 0;
        cache_need_free = 0;
        num_directories = file_counter = num_files_big = num_files_split = 0;

        copy_split_to_cache = 1;
        my_game_test((char *) name2);
        copy_split_to_cache = 0;

        if(!nfilecached)
        {
            sprintf(string1, language[GAMECHCPY_ISNEEDONEFILE]);

            DrawDialogOK(string1);

            cache_need_free = 0.0f;

            global_device_bytes = 0;

            abort_copy = 0;
            file_counter = 0;
            copy_mode = 0;

            new_pad = 0;

            return;
        }

        u32 blockSize;
        u64 freeSize;
        float freeSpace;
        float tmp_total_bytes = (global_device_bytes/ 1073741824.0); //save total bytes counted

        sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
        freeSpace = ( ((u64)blockSize * freeSize));
        freeSpace = freeSpace / 1073741824.0;

        for(n = 0; n < nfilecached; n++)
            cache_need_free += filecached_bytes[n];

        global_device_bytes = cache_need_free; // update with correct value

        cache_need_free = (cache_need_free / 1073741824.0) + 2.0f; // +2 for system

        if(freeSpace < cache_need_free) {
            sprintf(string1, language[GAMECHCPY_NEEDMORESPACE], freeSpace, cache_need_free);

            DrawDialogOK(string1);
            
            draw_cache_external();

            new_pad = 0;
        }

        sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
        freeSpace = ( ((u64)blockSize * freeSize));
        freeSpace = freeSpace / 1073741824.0;

        if(freeSpace < cache_need_free) {
            sprintf(string1, language[GAMECHCPY_NOSPACE], freeSpace, cache_need_free);

            DrawDialogOK(string1);

            cache_need_free = 0.0f;

            global_device_bytes = 0;

            abort_copy = 0;
            file_counter = 0;
            copy_mode = 0;

            new_pad = 0;

            return;
        }

        sprintf(string1, language[GAMECHCPY_CACHENFOSTART], (cache_need_free - 2.0f), tmp_total_bytes, (tmp_total_bytes - (cache_need_free - 2.0f)) , freeSpace);
        DrawDialogOK(string1);

        sprintf(name, "%s/cache", hmanager_path);	
        mkdir_secure(name);
        sprintf(name, "%s/cache/%s", hmanager_path, directories[game_sel].title_id);	
        mkdir_secure(name);

        copy_total_size = global_device_bytes;

        cache_need_free = 0.0f;
        global_device_bytes = 0;

        ////////////////

        DPrintf("%s %s\n %s %s\n\n", language[GAMECPYSL_STARTED], directories[game_sel].path_name, language[GLUTIL_WTO], name);

        abort_copy = 0;
        file_counter = 0;
        copy_mode = 0;

        copy_is_split=0;

        copy_split_to_cache = 1;
        my_game_copy((char *) name2, (char *) name);
        copy_split_to_cache = 0;

        int seconds = (int) (time(NULL) - time_start);
        int vflip = 0;
        
        

        while(1) {
            
            if(abort_copy) sprintf(string1, "%s  %s: %2.2i:%2.2i:%2.2i\n", language[GLUTIL_ABORTED], 
                                    language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60);
            else {
                    
                sprintf(string1,"%s: %i %s: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", language[GAMECPYSL_DONE], file_counter, 
                        language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60, 
                        ((double) global_device_bytes) / (1024.0 * 1024. * 1024.0));
            }

            
            cls2();

            strcpy(dbg_str1, string1);

            if(vflip & 32)
                strcpy(dbg_str2, language[GLUTIL_XEXIT]);
            else
                strcpy(dbg_str2, "");

            vflip++;
                
            DbgDraw();

            tiny3d_Flip();

            //pad_last_time = 0;
            ps3pad_read();

            if (new_pad & BUTTON_CROSS) {
            
                new_pad = 0;
                break;
            }

        }

        if(abort_copy || nfilecached ==0) {

           
            sprintf(filename, "%s\n\n%s USB00%c?", directories[game_sel].title, language[GAMECHCPY_FAILDELFROM], 47 + dest);

            dialog_action = 0;
            
            ret = msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*) 0x0000aaaa, NULL );
                    
            wait_dialog();

            if(dialog_action == 1) {

                abort_copy = 0;
                time_start = time(NULL);
                file_counter = 0;

                my_game_delete((char *) name);
    
                rmdir_secure((char *) name); // delete this folder

                game_sel = 0;

            }
        } else {
        
           sprintf(name, "%s/cache/%s/paths.dir", hmanager_path, directories[game_sel].title_id);
           SaveFile(name, (char *) filecached, 2048 * nfilecached);
           sprintf(name, "%s/cache/%s/name_entry", hmanager_path, directories[game_sel].title_id);
           SaveFile(name, (char *) directories[game_sel].title, 64);
        }
        
    }
	
}
////////////////////
void delete_game(int game_sel)
{
   
    int n, ret;

    copy_split_to_cache = 0;
    
    if(directories[game_sel].flags & 2048) return;

    for(n = 0; n < 11; n++){
        if((directories[game_sel].flags >> n) & 1) break;
    }

    if(n == 0)
        sprintf(filename, "%s\n\n%s HDD0?", directories[game_sel].title, language[GAMEDELSL_WANTDELETE]); 
    else
        sprintf(filename, "%s\n\n%s USB00%c?", directories[game_sel].title, language[GAMEDELSL_WANTDELETE], 47 + n); 

    dialog_action = 0;
        
    ret = msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*) 0x0000aaaa, NULL );
         
    wait_dialog();
            
    // reset to update datas
    forcedevices = (1 << n);

    if(dialog_action == 1) {

        time_start = time(NULL);
        
        abort_copy = 0;
        initConsole();
        file_counter = 0;
        new_pad = 0;
        
        DPrintf("%s %s\n\n", language[GAMEDELSL_STARTED], directories[game_sel].path_name);

        my_game_delete((char *) directories[game_sel].path_name);
        
        rmdir_secure((char *) directories[game_sel].path_name); // delete this folder

        game_sel = 0;

        int seconds = (int) (time(NULL) - time_start);
        int vflip = 0;

        while(1){

            if(abort_copy) sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i\n", language[GLUTIL_ABORTED], 
                                    language[GLUTIL_TIME], seconds / 3600, (seconds/60) % 60, seconds % 60);
            else
                sprintf(string1,"%s: %i %s: %2.2i:%2.2i:%2.2i\n", language[GAMEDELSL_DONE], file_counter, 
                                    language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60);

            cls2();

            strcpy(dbg_str1, string1);

            if(vflip & 32)
                strcpy(dbg_str2, language[GLUTIL_XEXIT]);
            else
                strcpy(dbg_str2, "");

            vflip++;
            
            DbgDraw();

            tiny3d_Flip();

            ps3pad_read();
            if (new_pad & BUTTON_CROSS) {

                new_pad = 0;
                break;
            }
        }
    }
}


void test_game(int game_sel)
{
   
   time_start= time(NULL);
			
    abort_copy=0;

    copy_split_to_cache = 0;

    initConsole();

    file_counter = 0;
    new_pad = 0;
    
    global_device_bytes = 0;

    num_directories = file_counter = num_files_big = num_files_split = 0;
    
    
    my_game_test(directories[game_sel].path_name);
    

    DPrintf(language[GAMETSTSL_FINALNFO], num_directories, file_counter, num_files_big, num_files_split);
    DPrintf("\n\n");

    int seconds = (int) (time(NULL) - time_start);
    int vflip = 0;

    while(1){

        if(abort_copy) sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i\n", language[GLUTIL_ABORTED], 
                                language[GLUTIL_TIME], seconds / 3600, (seconds/60) % 60, seconds % 60);
        else
            sprintf(string1,"%s: %i %s: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", language[GAMETSTSL_TESTED], file_counter, 
                                language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60, 
                                ((double) global_device_bytes) / (1024.0 * 1024.* 1024.0));

        cls2();

        strcpy(dbg_str1, string1);

        if(vflip & 32)
            strcpy(dbg_str2, language[GLUTIL_XEXIT]);
        else
            strcpy(dbg_str2, "");

        vflip++;
        
        DbgDraw();

        tiny3d_Flip();

        ps3pad_read();
        if (new_pad & BUTTON_CROSS) {

            new_pad = 0;
            break;
        }
    }


    // rename in test for non executable games
    if(num_files_split && (directories[game_sel].flags & 2046)) {
       
        char *str = strstr(directories[game_sel].path_name, __MKDEF_GAMES_DIR);

            if(str && str[7] != '_') {
                int n = (str - directories[game_sel].path_name);
                memcpy(filename, directories[game_sel].path_name, n + 7);filename[n+7] = '_'; filename[n+8] =0;
                strcat(filename, str + 7);
                sysLv2FsRename(directories[game_sel].path_name, filename);
                //DrawDialogOK(filename);
                forcedevices = 2046;
            }
    
    }
}

void DeleteDirectory(const char* path)
{
	int dfd;
	u64 read;
	sysFSDirent dir;
	if (sysLv2FsOpenDir(path, &dfd))
		return;
    sysFsChmod(path, FS_S_IFDIR | 0777);
    
    read = sizeof(sysFSDirent);
	while (!sysLv2FsReadDir(dfd, &dir, &read)) {
		char newpath[0x440];
		if (!read)
			break;
		if (!strcmp(dir.d_name, ".") || !strcmp(dir.d_name, ".."))
			continue;

		strcpy(newpath, path);
		strcat(newpath, "/");
		strcat(newpath, dir.d_name);

		if (dir.d_type & DT_DIR) {
			DeleteDirectory(newpath);
            sysFsChmod(newpath, FS_S_IFDIR | 0777);
            sysLv2FsRmdir(newpath);
		} else {
            sysFsChmod(path, FS_S_IFMT | 0777);
			sysLv2FsUnlink(newpath);
		}
	}
	sysLv2FsCloseDir(dfd);
}

void FixDirectory(const char* path)
{
	int dfd;
	u64 read;
	sysFSDirent dir;
    
	if (sysLv2FsOpenDir(path, &dfd))
		return;

    sysFsChmod(path, FS_S_IFDIR | 0777);

    read = sizeof(sysFSDirent);
	while (!sysLv2FsReadDir(dfd, &dir, &read)) {
		char newpath[0x440];
		if (!read)
			break;
		if (!strcmp(dir.d_name, ".") || !strcmp(dir.d_name, ".."))
			continue;

		strcpy(newpath, path);
		strcat(newpath, "/");
		strcat(newpath, dir.d_name);

		if (dir.d_type & DT_DIR) {

			FixDirectory(newpath);
          
		} else {
			sysLv2FsChmod(newpath, FS_S_IFMT | 0777);
		}
	}
	sysLv2FsCloseDir(dfd);
}

/*******************************************************************************************************************************************************/
/* Configfiles                                                                                                                                         */
/*                                                                                                                                                     */
/* Caprice32 - Amstrad CPC Emulator                                                                                                                    */
/*   (c) Copyright 1997-2004 Ulrich Doewich                                                                                                            */
/*   (c) Copyright 2011 D_Skywalk - ported and adapted for Irismanager                                                                                 */
/*******************************************************************************************************************************************************/

/*
   Ex:
   snd_pp_device = getConfigValueInt("/home/user/config.ini", "sound", "pp_device", 0);
   max_tracksize = getConfigValueInt(chFileName, "file", "max_track_size", 6144-154);
*/

int getConfigMemValueInt (char* mem, int size, char* pchSection, char* pchKey, int iDefaultValue)
{
   
   char chLine[MAX_CFGLINE_LEN + 1];
   char* pchToken;
   int n = 0;

      while(n<size) { // grab one line
        
        int flag = 0;
        int m = 0;
        while(mem && n<size && m<MAX_CFGLINE_LEN-1) {
            chLine[m] = mem[n]; n++;
            if(chLine[m]=='\n' || chLine[m]=='\r') {m++; flag = 1; continue;}
            if(flag) {n--;break;}
            m++;
        }

        chLine[m]= 0;  // grab one line
         pchToken = strtok(chLine, "[]"); // check if there's a section key
         if((pchToken != NULL) && (pchToken[0] != '#') && (strcmp(pchToken, pchSection) == 0)) {
            while(n<size) { // get the next line
                m = 0; flag = 0;
                while(mem && n<size && m<MAX_CFGLINE_LEN-1) {
                    chLine[m] = mem[n]; n++;
                    if(chLine[m]=='\n' || chLine[m]=='\r') {m++; flag = 1; continue;}
                    if(flag) {n--;break;}
                    m++;
                }
                chLine[m]= 0;
               pchToken = strtok(chLine, "\t =\n\r"); // check if it has a key=value pair
               if((pchToken != NULL) && (pchToken[0] != '#') && (strcmp(pchToken, pchKey) == 0)) {
                  char* pchPtr = strtok(NULL, "\t =#\n\r"); // get the value if it matches our key
                  if (pchPtr != NULL) {
                     return (strtol(pchPtr, NULL, 0)); // return as integer
                  } else {
                     return iDefaultValue; // no value found
                  }
               }
            }
         }
      }

   return iDefaultValue; // no value found
}

/*
   Ex:
   getConfigValueString(chFileName, "file", "snap_path", snap_path, sizeof(snap_path)-1, defaultPath);
   getConfigValueString(chFileName, "file", "push_file", snap_file, sizeof(push_file)-1, "push0.sna");

*/

void convertStringEndl(char* string, int iSize);


int getConfigMemValueString (char* mem, int size, char* pchSection, char* pchKey, char* pchValue, int iSize, char* pchDefaultValue)
{
   
   char chLine[MAX_CFGLINE_LEN + 1];
   char* pchToken;
   int n = 0;

     // open the config file

      while(n<size) { // grab one line
        
        int flag = 0;
        int m = 0;
        while(mem && n<size && m<MAX_CFGLINE_LEN-1) {
            chLine[m] = mem[n]; n++;
            if(chLine[m]=='\n' || chLine[m]=='\r') {m++; flag = 1; continue;}
            if(flag) {n--;break;}
            m++;
        }
        chLine[m]= 0;


         pchToken = strtok(chLine, "[]"); // check if there's a section key
         if((pchToken != NULL) && (pchToken[0] != '#') && (strcmp(pchToken, pchSection) == 0)) {
            while(n<size) { // get the next line
                m = 0; flag = 0;
                while(mem && n<size && m<MAX_CFGLINE_LEN-1) {
                    chLine[m] = mem[n]; n++;
                    if(chLine[m]=='\n' || chLine[m]=='\r') {m++; flag = 1; continue;}
                    if(flag) {n--;break;}
                    m++;
                }
                chLine[m]= 0;

               pchToken = strtok(chLine, "\t =\n\r"); // check if it has a key=value pair
               if((pchToken != NULL) && (pchToken[0] != '#') && (strcmp(pchToken, pchKey) == 0)) {
                  char* pchPtr = strtok(NULL, "\t "); // unlink pre tabs and spaces
                  pchPtr = strtok(NULL, "\t=#\n\r"); // get the value if it matches our key
                  if ((pchPtr != NULL)&&( *pchPtr != '\0')) {
                     convertStringEndl(pchPtr, iSize);
                     strncpy(pchValue, pchPtr, iSize); // copy to destination
                     pchValue[iSize -1] = 0;
                     return 0;
                  } else {
                     strncpy(pchValue, pchDefaultValue, iSize); // no value found, return the default
                     pchValue[iSize -1] = 0;
                     return 1;
                  }
               }
            }
         }
      }
     
   
   strncpy(pchValue, pchDefaultValue, iSize); // no value found, return the default
   pchValue[iSize -1] = 0;
   return 1;

}

#define MAX_CONVERSIONS 2

char conversionTable[MAX_CONVERSIONS][2] = 
{ 
    { '@', '\n' },
    { '_', ' ' },
};

void convertStringEndl(char* string, int iSize)
{
    int n;
    int m = 0;

    do
    {
        if(string[m] == 9) string[m] = ' ';
        if(string[m] < 32) {string[m] = 0; return;} // break the string

        for(n = 0; n < MAX_CONVERSIONS; n++)
            if(string[m] == conversionTable[n][0])
                string[m] = conversionTable[n][1];
        m++;
    }
    while(iSize--);
}

/*******************************************************************************************************************************************************/
/* Favourites                                                                                                                                          */
/*******************************************************************************************************************************************************/

int havefavourites = 0;
tfavourites favourites;

static tfavourites favourites_gamebase;
static tfavourites favourites_homebrew;

void LoadFavourites(char * path, int mode)
{
    int n, mode_flag;
    tfavourites * pfavourites = NULL;

    if(mode == GAMEBASE_MODE) {
        pfavourites = &favourites_gamebase;
        mode_flag = 0;
        strcat(path, "favourites.bin");
    } else if(mode == HOMEBREW_MODE) {
        pfavourites = &favourites_homebrew;
        mode_flag = 0xffff0000;
        strcat(path, "favourites2.bin");
    } else
        return;

    memset(pfavourites, 0, sizeof(*pfavourites));

    int file_size;
    char *file = LoadFile(path, &file_size);

    if(file) {
        tfavourites *fav = (tfavourites *) file;
        if(file_size == sizeof(*pfavourites) && fav->version == 100)
            memcpy(pfavourites, file, sizeof(*pfavourites));
        free(file);
    }

    for(n = 0; n < 12; n++) {
        pfavourites->list[n].index  = -1;
        pfavourites->list[n].flags &=  mode_flag;
    }

}

void SaveFavourites(char * path, int mode)
{
    favourites.version = 100;
    if(mode == GAMEBASE_MODE) {
        strcat(path, "favourites.bin");
        favourites_gamebase =favourites;
    }
    else if (mode == HOMEBREW_MODE) {
        strcat(path, "favourites2.bin");
        favourites_homebrew =favourites;
    }
      
    SaveFile(path, (void *) &favourites, sizeof(favourites)); // use favourites_gamebase?
}

void GetFavourites(int mode)
{
    if(mode == HOMEBREW_MODE)
        favourites = favourites_homebrew;
    else
        favourites = favourites_gamebase;
}

void SetFavourites(int mode)
{
    if(mode == HOMEBREW_MODE)
        favourites_homebrew = favourites;
    else
        favourites_gamebase = favourites;
}

void UpdateFavourites(t_directories *list, int nlist)
{
    int n, m;

    havefavourites = 0;

    for(m = 0; m < 12; m++) {
        favourites.list[m].index = -1;

        for(n = 0; n < nlist; n++) {
            if(favourites.list[m].title_id[0] !=0 && !strncmp(list[n].title_id, favourites.list[m].title_id, 64)) {
                if((favourites.list[m].index < 0) || 
                   (favourites.list[m].flags & GAMELIST_FILTER) > (list[n].flags & GAMELIST_FILTER)) {
                    //strncpy(favourites.list[m].title_id, list[n].title_id, 64);
                    //strncpy(favourites.list[m].title, list[n].title, 64);
                    favourites.list[m].index = n;
                    favourites.list[m].flags = list[n].flags;
                    havefavourites = 1;
                } 
            }
        }
    }
}


int TestFavouritesExits(char *id)
{
    int m;
    for(m = 0; m < 12; m++) {
        if(!strncmp(favourites.list[m].title_id, id, 64)) return 1;
    }
    
    return 0;
}

void AddFavourites(int indx, t_directories *list, int position_list)
{
    strncpy(favourites.list[indx].title_id, list[position_list].title_id, 64);
    strncpy(favourites.list[indx].title, list[position_list].title, 64);
    favourites.list[indx].index = position_list;
    favourites.list[indx].flags = list[position_list].flags;
    havefavourites = 1;

}

int DeleteFavouritesIfExits(char *id)
{
    int m;
    for(m = 0; m < 12; m++) {
        if(!strcmp(favourites.list[m].title_id, id)) {
            memset(favourites.list[m].title_id, 0, 64);
            memset(favourites.list[m].title, 0, 64);
            favourites.list[m].index = -1;
        }
    }
    
    havefavourites = 0;

    for(m = 0; m < 12; m++) {
        if(favourites.list[m].index >= 0) havefavourites = 1;
    }

    return 0;
}

int param_sfo_util(char * path, int patch_app)
{
    int patched = 0;
	Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;
    char file[0x420];
    char file2[0x420];

    u8 * app_ver = NULL;

    char str_version[8];
    
    unsigned char *mem = NULL;
    unsigned char *mem2 = NULL;

    int n;
    char * version = LoadFile("/dev_flash/vsh/etc/version.txt", &n);

    if(!version) return -2;

    char *v = strstr(version, "release:");

    if(v) {memcpy(str_version, v + 8, 7); str_version[7] = 0;}
    
    free(version);

    if(!v) return -3;


    sprintf(file, "%s/PS3_GAME/PARAM.SFO", path);
    sprintf(file2, "%s/PS3_GAME/_PARAM.SFO", path);
	
    if(!sysLv2FsOpen(file2, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0)) {
		unsigned len, pos, str;
		

        sysLv2FsLSeek64(fd, 0, 2, &position);
		len = (u32) position;

		mem2 = (unsigned char *) malloc(len+16);
		if(!mem2) {sysLv2FsClose(fd); return -2;}

		memset(mem2, 0, len+16);

		sysLv2FsLSeek64(fd, 0, 0, &position);
		
        if(sysLv2FsRead(fd, mem2, len, &bytes)!=0) bytes =0LL;

        len = (u32) bytes;

		sysLv2FsClose(fd);

		str= (mem2[8]+(mem2[9]<<8));
		pos=(mem2[0xc]+(mem2[0xd]<<8));

		int indx=0;

		while(str<len) {
			if(mem2[str]==0) break;
			
			

            if(!strcmp((char *) &mem2[str], "APP_VER")) {
               
				app_ver = &mem2[pos];
                
			}

			while(mem2[str]) str++;str++;
			pos+=(mem2[0x1c+indx]+(mem2[0x1d+indx]<<8));
			indx+=16;
		}
    
    }

    n = sysLv2FsOpen(file, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);
    
    if(n) {n = sysLv2FsOpen(file2, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0); patched = 1;}

	if(!n) {
		unsigned len, pos, str;
		

        sysLv2FsLSeek64(fd, 0, 2, &position);
		len = (u32) position;

		mem = (unsigned char *) malloc(len+16);
		if(!mem) {sysLv2FsClose(fd); return -2;}

		memset(mem, 0, len+16);

		sysLv2FsLSeek64(fd, 0, 0, &position);
		
        if(sysLv2FsRead(fd, mem, len, &bytes)!=0) bytes =0LL;

        len = (u32) bytes;

		sysLv2FsClose(fd);

		str= (mem[8]+(mem[9]<<8));
		pos=(mem[0xc]+(mem[0xd]<<8));

		int indx=0;

		while(str<len) {
			if(mem[str]==0) break;
			
			if(!strcmp((char *) &mem[str], "PS3_SYSTEM_VER")) {
				if(strcmp((char *) &mem[pos], str_version) > 0) {
                    memcpy(&mem[pos], str_version, 8);
                    patched = 1;
                }
			}

            if(!strcmp((char *) &mem[str], "APP_VER")) {
               u8 old =  mem[pos + 1];
               
               if(app_ver) {
                    
                   mem[pos + 1] = app_ver[1];
                   if(mem[pos + 1] == '9') mem[pos + 1] = '1';
               } else mem[pos + 1] = '1';
                
               if(patch_app) {
				    mem[pos + 1] = '9';
               }

               if(old != mem[pos + 1]) patched = 1;
			}

			while(mem[str]) str++;str++;
			pos+=(mem[0x1c+indx]+(mem[0x1d+indx]<<8));
			indx+=16;
		}

        if(patched) {
            sysLv2FsRename(file, file2);
            SaveFile(file, (char *) mem, len);   
        }

    if(mem) free(mem);
    if(mem2) free(mem2);

    return 0;
        
    }

	return -1;

}

int param_sfo_patch_category_to_cb(char * path_src, char *path_dst)
{
    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;
    char file[0x420];
    char file2[0x420];
    
    unsigned char *mem2 = NULL;

    sprintf(file, "%s", path_src);
    sprintf(file2, "%s", path_dst);
    
    if(!sysLv2FsOpen(file, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0)) {
        unsigned len, pos, str;
        
        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        mem2 = (unsigned char *) malloc(len+16);
        if(!mem2) {
            sysLv2FsClose(fd);
            return -2;
        }

        memset(mem2, 0, len+16);

        sysLv2FsLSeek64(fd, 0, 0, &position);
        
        if(sysLv2FsRead(fd, mem2, len, &bytes)!=0)
            bytes =0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        str= (mem2[8]+(mem2[9]<<8));
        pos=(mem2[0xc]+(mem2[0xd]<<8));
        int indx=0;

        while(str<len) {
            if(mem2[str] == 0)
                break;

            if(!strcmp((char *) &mem2[str], "CATEGORY")) {
                mem2[pos]='H';
                mem2[pos+1]='G';
            }

            if(!strcmp((char *) &mem2[str], "APP_VER")) {
                mem2[pos]='9';mem2[pos+1]='9';mem2[pos+3]='9';mem2[pos+4]='9';
            }

            while(mem2[str])
                str++;
            str++;
            pos += (mem2[0x1c+indx] + (mem2[0x1d+indx]<<8));
            indx += 16;
        }

        if(SaveFile(file2, (char *) mem2, len)) {
            free(mem2);
            return -2;
        }

        if(mem2)
            free(mem2);

        return 0;
    }

    return -1;
}

/*******************************************************************************************************************************************************/
/* sys8 path table                                                                                                                                     */
/*******************************************************************************************************************************************************/


static char *table_compare[17];
static char *table_replace[17];

static int ntable = 0;

void reset_sys8_path_table()
{
    while(ntable > 0) {

        if(table_compare[ntable - 1]) free(table_compare[ntable - 1]);
        if(table_replace[ntable - 1]) free(table_replace[ntable - 1]);

        ntable --;
    }
}

void add_sys8_path_table(char * compare, char * replace)
{
    if(ntable >= 16) return;

    table_compare[ntable] = malloc(strlen(compare) + 1);
    if(!table_compare[ntable]) return;
    strncpy(table_compare[ntable], compare, strlen(compare) + 1);
    
    table_replace[ntable] = malloc(strlen(replace) + 1);
    if(!table_replace[ntable]) return;
    strncpy(table_replace[ntable], replace, strlen(replace) + 1);

    ntable++;

    table_compare[ntable] = NULL;
}


void build_sys8_path_table()
{

    path_open_entry *pentries;

    int entries = 0;

    int arena_size = 0;

    int n, m;

    sys8_path_table(0LL);

    if(ntable <= 0) return;

    while(table_compare[entries] != NULL) {
        int l = strlen(table_compare[entries]);

        arena_size += 0x420;
        for(m = 0x80; m <= 0x420; m += 0x20)
            if(l < m) {arena_size += m;break;}

    entries++;
    }

    if(!entries) return;

    char * datas = memalign(16, arena_size + sizeof(path_open_entry) * (entries + 1));
    
    if(!datas) return;

    u64 dest_table_addr = 0x80000000007FF000ULL - (u64)((arena_size + sizeof(path_open_entry) * (entries + 1) + 15) & ~15);

    u32 arena_offset = (sizeof(path_open_entry) * (entries + 1));

    pentries = (path_open_entry *) datas;

    for(n = 0; n < entries; n++) {
    
        int l = strlen(table_compare[n]);

        int size = 0;
        for(m = 0x80; m <= 0x420; m += 0x20)
            if(l < m) {size += m; break;}

        pentries->compare_addr = dest_table_addr + (u64) (arena_offset);

        pentries->replace_addr = dest_table_addr + (u64) (arena_offset + size);
        

        strncpy(&datas[arena_offset], table_compare[n], size);
        strncpy(&datas[arena_offset + size], table_replace[n], 0x420);

        pentries->compare_len = strlen(&datas[arena_offset]);
        pentries->replace_len = strlen(&datas[arena_offset + size]);

        arena_offset += size + 0x420;
        pentries ++;
       
    }
    
    pentries->compare_addr = 0ULL;

    sys8_memcpy(dest_table_addr, (u64) datas, (u64) (arena_size + sizeof(path_open_entry) * (entries + 1)));

    free(datas);

    reset_sys8_path_table();

    // set the path table
    sys8_path_table( dest_table_addr);
}


void copy_usb_to_iris(char * path)
{
    int n;
    char name[0x420];

    copy_split_to_cache = 0;
    copy_total_size = 0LL;
    file_counter = 0;
    abort_copy = 0;

    for(n=1;n<11;n++) {
         if((fdevices >> n) & 1) break;
    }

    if(n == 12) return;
    
    n+= 47;
    sprintf(name, "/dev_usb00%c/iris", n);

    initConsole();
    my_game_countsize(name);

    u32 blockSize;
    u64 freeSize;
    u64 free_hdd0;
    sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
    free_hdd0 = ( ((u64)blockSize * freeSize));

    if((copy_total_size + 0x40000000LL) >= (s64) free_hdd0) {
        sprintf(filename, "%s\n\n%s%1.2f GB", "Error: no space in HDD0 to copy it", "You need ", 
            ((double) (copy_total_size + 0x40000000LL - free_hdd0))/(1024.0*1024.*1024.0));
        DrawDialogOK(filename);
        return;
    }

    if(abort_copy) //abort by user or got an error
    { 
        if(DrawDialogYesNo(language[GAMECPYSL_GSIZEABCNTASK]) != 1)
        {
            return;
        }
        else
        {
            //old mode
            copy_total_size = 0;
        }
    } 

    int ret;
 
    int dest = 0;

    dialog_action = 0;
    abort_copy = 0;

    dest = 0;

    sprintf(filename, "/dev_usb/iris %s Iris Manager?\nVol: %1.2f MB", 
                        language[GLUTIL_WTO], 
                        ((double) copy_total_size)/(1024.0*1024)); 

    dialog_action = 0;
    
    ret = msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*)0x0000aaaa, NULL );
            
    wait_dialog();


    if(dialog_action == 1) {
        time_start = time(NULL);

        abort_copy = 0;
        initConsole();
        file_counter = 0;
        new_pad = 0;

        DPrintf("%s %s\n %s %s\n\n", language[GAMECPYSL_STARTED], "USB", language[GLUTIL_WTO], "Iris Manager");

        copy_mode = 0;

        copy_is_split=0;

        // sys8_perm_mode(1);
        my_game_copy((char *) name, (char *) path);
        // sys8_perm_mode(0);

        cls2();

        int seconds = (int) (time(NULL) - time_start);
        int vflip = 0;
        
        
        while(1) {
            
            if(abort_copy) sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i\n", language[GLUTIL_ABORTED], 
                                    language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60);
            else {
                    
                sprintf(string1,"%s: %i %s: %2.2i:%2.2i:%2.2i Vol: %1.2f MB\n", language[GAMECPYSL_DONE], file_counter, 
                        language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60, ((double) global_device_bytes) / (1024. * 1024.0));
            }

            
            cls2();

            strcpy(dbg_str1, string1);

            if(vflip & 32)
                strcpy(dbg_str2, language[GLUTIL_XEXIT]);
            else
                strcpy(dbg_str2, "");

            vflip++;
                
            DbgDraw();

            tiny3d_Flip();
            
            ps3pad_read();

            if (new_pad & BUTTON_CROSS) {
            
                new_pad = 0;
                break;
            }

        }    
        
    }
	
}

