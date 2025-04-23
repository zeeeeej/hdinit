
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "hd_safe_map.h"

// 哈希函数
static unsigned int hash(const char* key, int capacity) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % capacity;
}

// 初始化Map
ThreadSafeMap* hd_map_init(int capacity) {
    if (capacity <= 0) {
        return NULL;
    }
    
    ThreadSafeMap* map = (ThreadSafeMap*)malloc(sizeof(ThreadSafeMap));
    if (!map) {
        return NULL;
    }
    
    map->buckets = (MapNode**)calloc(capacity, sizeof(MapNode*));
    if (!map->buckets) {
        free(map);
        return NULL;
    }
    
    map->capacity = capacity;
    map->size = 0;
    pthread_rwlock_init(&map->lock, NULL);
    
    return map;
}

// 反初始化Map
void hd_map_deinit(ThreadSafeMap* map) {
    if (!map) return;
    
    // 获取写锁
    pthread_rwlock_wrlock(&map->lock);
    
    for (int i = 0; i < map->capacity; i++) {
        MapNode* node = map->buckets[i];
        while (node) {
            MapNode* temp = node;
            node = node->next;
            free(temp->key);
            free(temp);
        }
    }
    
    free(map->buckets);
    pthread_rwlock_unlock(&map->lock);
    pthread_rwlock_destroy(&map->lock);
    free(map);
}

// 添加键值对
int hd_map_put(ThreadSafeMap* map, const char* key, void* value) {
    if (!map || !key) return 0;
    
    // 获取写锁
    pthread_rwlock_wrlock(&map->lock);
    
    unsigned int index = hash(key, map->capacity);
    MapNode* node = map->buckets[index];
    
    // 检查key是否已存在
    while (node) {
        if (strcmp(node->key, key) == 0) {
            // 已存在，更新value
            node->value = value;
            pthread_rwlock_unlock(&map->lock);
            return 1;
        }
        node = node->next;
    }
    
    // 创建新节点
    MapNode* new_node = (MapNode*)malloc(sizeof(MapNode));
    if (!new_node) {
        pthread_rwlock_unlock(&map->lock);
        return 0;
    }
    
    new_node->key = strdup(key);
    if (!new_node->key) {
        free(new_node);
        pthread_rwlock_unlock(&map->lock);
        return 0;
    }
    
    new_node->value = value;
    new_node->next = map->buckets[index];
    map->buckets[index] = new_node;
    map->size++;
    
    pthread_rwlock_unlock(&map->lock);
    return 1;
}

// 删除键值对
int hd_map_remove(ThreadSafeMap* map, const char* key) {
    if (!map || !key) return 0;
    
    // 获取写锁
    pthread_rwlock_wrlock(&map->lock);
    
    unsigned int index = hash(key, map->capacity);
    MapNode* node = map->buckets[index];
    MapNode* prev = NULL;
    
    while (node) {
        if (strcmp(node->key, key) == 0) {
            if (prev) {
                prev->next = node->next;
            } else {
                map->buckets[index] = node->next;
            }
            
            free(node->key);
            free(node);
            map->size--;
            
            pthread_rwlock_unlock(&map->lock);
            return 1;
        }
        prev = node;
        node = node->next;
    }
    
    pthread_rwlock_unlock(&map->lock);
    return 0;
}

// 更新键值对
int hd_map_update(ThreadSafeMap* map, const char* key, void* value) {
    if (!map || !key) return 0;
    
    // 获取写锁
    pthread_rwlock_wrlock(&map->lock);
    
    unsigned int index = hash(key, map->capacity);
    MapNode* node = map->buckets[index];
    
    while (node) {
        if (strcmp(node->key, key) == 0) {
            node->value = value;
            pthread_rwlock_unlock(&map->lock);
            return 1;
        }
        node = node->next;
    }
    
    pthread_rwlock_unlock(&map->lock);
    return 0;
}

// 获取所有value
void** hd_map_get_all_values(ThreadSafeMap* map, int* count) {
    if (!map || !count) return NULL;
    
    // 获取读锁
    pthread_rwlock_rdlock(&map->lock);
    
    *count = map->size;
    if (map->size == 0) {
        pthread_rwlock_unlock(&map->lock);
        return NULL;
    }
    
    void** values = (void**)malloc(map->size * sizeof(void*));
    if (!values) {
        pthread_rwlock_unlock(&map->lock);
        return NULL;
    }
    
    int idx = 0;
    for (int i = 0; i < map->capacity; i++) {
        MapNode* node = map->buckets[i];
        while (node) {
            values[idx++] = node->value;
            node = node->next;
        }
    }
    
    pthread_rwlock_unlock(&map->lock);
    return values;
}

// 打印所有key-value
void hd_map_print_all(ThreadSafeMap* map, const char* format) {
    if (!map || !format) return;
    
    // 获取读锁
    pthread_rwlock_rdlock(&map->lock);
    
    printf("Map contents (%d items):\n", map->size);
    for (int i = 0; i < map->capacity; i++) {
        MapNode* node = map->buckets[i];
        while (node) {
            printf("Key: %s, Value: ", node->key);
            printf(format, node->value);
            printf("\n");
            node = node->next;
        }
    }
    
    pthread_rwlock_unlock(&map->lock);
}

// 检查是否包含key
int hd_map_contains_key(ThreadSafeMap* map, const char* key) {
    if (!map || !key) return 0;
    
    // 获取读锁
    pthread_rwlock_rdlock(&map->lock);
    
    unsigned int index = hash(key, map->capacity);
    MapNode* node = map->buckets[index];
    
    while (node) {
        if (strcmp(node->key, key) == 0) {
            pthread_rwlock_unlock(&map->lock);
            return 1;
        }
        node = node->next;
    }
    
    pthread_rwlock_unlock(&map->lock);
    return 0;
}

// 检查Map是否为空
int hd_map_is_empty(ThreadSafeMap* map) {
    if (!map) return 1;
    
    // 获取读锁
    pthread_rwlock_rdlock(&map->lock);
    int empty = (map->size == 0);
    pthread_rwlock_unlock(&map->lock);
    
    return empty;
}


void hd_map_for_each(ThreadSafeMap* map, MapForEachCallback callback, void* context) {
    if (!map || !callback) return;
    
    // 获取读锁
    pthread_rwlock_rdlock(&map->lock);
    
    for (int i = 0; i < map->capacity; i++) {
        MapNode* node = map->buckets[i];
        while (node) {
            callback(node->value, context);
            node = node->next;
        }
    }
    
    pthread_rwlock_unlock(&map->lock);
}

// 通过key获取value
void* hd_map_get(ThreadSafeMap* map, const char* key) {
    if (!map || !key) return NULL;
    
    // 获取读锁
    pthread_rwlock_rdlock(&map->lock);
    
    unsigned int index = hash(key, map->capacity);
    MapNode* node = map->buckets[index];
    
    while (node) {
        if (strcmp(node->key, key) == 0) {
            void* value = node->value;
            pthread_rwlock_unlock(&map->lock);
            return value;
        }
        node = node->next;
    }
    
    pthread_rwlock_unlock(&map->lock);
    return NULL;
}


   