#ifndef _SYS_H_
#define _SYS_H_
#include "common.h"

void* sys_create_sem(int count, int maxcount, char* name);
void* sys_get_sem(char* name);
void sys_close_sem(void* handle);
int sys_wait_sem(void* handle, uint time=0);
void sys_signal_sem(void* handle);
void* sys_create_smem(uint size, char* name);
void* sys_get_smem(char* name);
void sys_close_smem(void* handle);
void* sys_map_smem(void* handle);
void sys_sem_signal_all(void* handle);

void sys_show_message(char* msg);
void sys_sleep(int millisec);
bool sys_has_dup_process(char* name);
void* sys_create_process(char* name);
void* sys_get_process(char* name);
int sys_get_current_process_name(char* name, int len);
int sys_get_current_process_path(char* path, int len);
void* sys_create_thread(int (*cb)(void*), void* param);
void sys_close_process(void* hproc);
void sys_close_thread(void* hthread);
int sys_wait_process(void* hproc, uint time=0);
int sys_wait_thread(void* hthread, uint time=0);

//Dynamic linked library operations
void* sys_load_library(char* name);
void sys_free_library(void* handle);
void* sys_get_lib_proc(void* handle, char* procname);

//File operations
#define FILE_OPEN_EXISTING       0
#define FILE_CREATE_NEW          1
#define FILE_CREATE_ALWAYS       2
#define FILE_OPEN_ALWAYS         3
#define FILE_TRUNCATE_EXISTING   4
#define FILE_MASK 0x7

#define FILE_READ 16
#define FILE_WRITE 32
#define FILE_NOCACHE 64
#define FILE_EXCLUSIVE_WRITE 128

#define SEEK_BEGIN 0
#define SEEK_CUR 1
#define SEEK_END 2

#define FILE_TYPE_NORMAL 1
#define FILE_TYPE_DIR 2

void* sys_fopen(char* pathname, dword flags);
int sys_fread(void* fd, void* buf, uint len, uint* rdlen=NULL);
int sys_fwrite(void* fd, void* buf, uint len, uint* wrlen=NULL);
int sys_fseek(void* fd, uint offlow, uint* offhigh=NULL, int seektype=SEEK_BEGIN);
int sys_get_file_size(void* fd, uint* sizelow, uint* sizehigh=NULL);
int sys_set_file_size(void* fd, uint sizelow, uint* sizehigh=NULL);
int sys_fflush(void* fd);
void sys_fclose(void* fd);

int sys_get_current_dir(char* pathbuf, uint size);
int sys_set_current_dir(char* pathbuf);
int sys_is_absolute_path(char* path, char dsym);
int sys_fstat(char* pathname, dword* type);
int sys_ftraverse(char* pathname, int(*cb)(char*, dword, void*, char), void* param);
int sys_mkdir(char* path);
int sys_fmove(char* from, char* to);
int sys_fcopy(char* from, char* to);
int sys_fdelete(char* pathname);

//Time support
struct DateTime
{
	short year;
	byte month;
	byte day;
	byte weekday;
	byte hour;
	byte minute;
	byte second;
	ushort millisecond;
};
void sys_get_date_time(DateTime* time);
int sys_get_file_time(char* path,DateTime* creation_time,DateTime* modify_time,DateTime* access_time);
int sys_set_file_time(char* path,DateTime* creation_time,DateTime* modify_time,DateTime* access_time);

#endif