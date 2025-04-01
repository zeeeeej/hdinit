
#ifndef __HD_UTILS__
#define __HD_UTILS__

#include <sys/stat.h>
#include <stdio.h>

#define DEBUG_PARAM "-debug"

time_t get_last_modified(const char *path) {
    struct stat attr;
    if (stat(path, &attr) == 0) {
        return attr.st_mtime;
    }
    return 0;
}


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


int hd_check_argc_argv_has_debug(int argc,  const char *argv[]){
    if (argc<=1)
    {
        return 0;
    } 
    for(int i =0 ;i<argc;i++){
        if (0==strcmp(DEBUG_PARAM,(argv[i])))
        {
            return 0;
        }
        
    }
    return -1;
    
}

int hd_printf_argc_argv(int argc,char const *argv[],char * const result){
        char temp[2048];
        result[0] = '\0'; // 清空结果字符串
    
        // 添加标题行
        snprintf(temp, sizeof(temp), "\nargc=%d \n", argc);
        strcat(result, temp);
        for(int i = 0;i<argc;i++){
            snprintf(temp, sizeof(temp), "  argv[%d] %s \n", i,argv[i]);
            strcat(result, temp);
        }
        snprintf(temp, sizeof(temp), "\n");
        return 0;
}

#endif // __HD_UTILS__