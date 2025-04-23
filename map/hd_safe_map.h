#ifndef __HD_SAFE_MAP__
#define __HD_SAFE_MAP__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Map节点结构
typedef struct MapNode {
    char* key;
    void* value;
    struct MapNode* next;
} MapNode;

// Map结构
typedef struct {
    MapNode** buckets;
    int capacity;
    int size;
    pthread_rwlock_t lock; // 读写锁
} ThreadSafeMap;

// 回调函数类型定义
typedef void (*MapForEachCallback)(void* value, void* context);

// 哈希函数
static unsigned int hash(const char* key, int capacity);

// 初始化Map
ThreadSafeMap* hd_map_init(int capacity);

// 反初始化Map
void hd_map_deinit(ThreadSafeMap* map);

// 添加键值对
int hd_map_put(ThreadSafeMap* map, const char* key, void* value);

// 删除键值对
int hd_map_remove(ThreadSafeMap* map, const char* key);

// 更新键值对
int hd_map_update(ThreadSafeMap* map, const char* key, void* value);

// 获取所有value
void** hd_map_get_all_values(ThreadSafeMap* map, int* count) ;

// 打印所有key-value
void hd_map_print_all(ThreadSafeMap* map, const char* format) ;

// 检查是否包含key
int hd_map_contains_key(ThreadSafeMap* map, const char* key) ;

// 检查Map是否为空
int hd_map_is_empty(ThreadSafeMap* map) ;


void hd_map_for_each(ThreadSafeMap* map, MapForEachCallback callback, void* context);

void* hd_map_get(ThreadSafeMap* map, const char* key) ;
#endif // __HD_SAFE_MAP__