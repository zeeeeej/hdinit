
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


#endif // __HD_UTILS__