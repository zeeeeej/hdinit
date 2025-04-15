
#ifndef __HD_UTILS__
#define __HD_UTILS__

#include <sys/stat.h>
#include <stdio.h>

#define DEBUG_PARAM "-debug"

const char *hd_get_filename(const char *path);

int hd_if_file_path_not_exists_create_path(const char *file_path) ;

int hd_cp_file(const char *src_path, const char *dst_path); 

time_t hd_get_last_modified(const char *path);

// int hd_argc_argv_remove_debug(int argc,char const **argv,int *newArgc,char  ** newArgv){
//     if (argc<=1)
//     {
//         return 0;
//     } 
//     int ctx = 0;
//     for(int i =0 ;i<argc;i++){
//         if (strcmp(DEBUG_PARAM,argv[i])!=0)
//         {
//             ctx++;
//             continue;
//         }
//         strcpy(*argv[i],newArgv[i]);

//     }
//     *newArgc = ctx;
//     return 0;
// }

int hd_check_argc_argv_has_debug(int argc,  const char *argv[]);

int hd_printf_argc_argv(int argc,char const *argv[],char * const result);

void hd_print_progress_bar(int percent) ;

void hd_print_progress_double(double percentage);

int  hd_child_info_encode( char *dest,const char * s_name, int s_id,const char * s_version);
int  hd_child_info_decode( const char *source, char* s_name, int* s_id, char * s_version);


void hd_print_buffer(const char *buff, size_t size);

void hd_trigger_reboot();





#define HASH_SIZE 128

// 值类型枚举
typedef enum {
    MAP_INT,
    MAP_FLOAT,
    MAP_DOUBLE,
    MAP_STRING,
    MAP_POINTER
} ValueType;

// 通用值结构
typedef struct {
    ValueType type;
    union {
        int int_val;
        float float_val;
        double double_val;
        char* string_val;
        void* pointer_val;
    } data;
} MapValue;

// 哈希表节点
typedef struct HashNode {
    char* key;
    MapValue value;
    struct HashNode* next;
} HashNode;

// 哈希表结构
typedef struct {
    HashNode* buckets[HASH_SIZE];
} HashMap;

// 哈希函数
unsigned int hash(const char* key) ;

// 初始化哈希表
void map_init(HashMap* map);

// 插入键值对
void map_put(HashMap* map, const char* key, MapValue value) ;

// 获取值
int map_get(HashMap* map, const char* key, MapValue* out_value) ;

// 删除键值对
int map_remove(HashMap* map, const char* key) ;

// 释放整个哈希表
void map_free(HashMap* map) ;

// 创建各种类型的值
MapValue map_make_int(int val);

MapValue map_make_float(float val) ;

MapValue map_make_double(double val) ;

MapValue map_make_string(const char* val) ;

MapValue map_make_pointer(void* val);

// 打印值
void print_value(MapValue value) ;

// 打印值
void map_print(HashMap* map) ;

void map_print_debug(HashMap* map) ;

void map_pretty_print(HashMap* map);

#endif // __HD_UTILS__
