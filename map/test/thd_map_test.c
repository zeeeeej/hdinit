#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../hd_safe_map.h"

// 测试用例
int main() {
     // 初始化Map
     ThreadSafeMap* map = hd_map_init(16);
     if (!map) {
         printf("Failed to initialize map\n");
         return 1;
     }
     
     // 添加键值对
     int a = 10, b = 20, c = 30;
     hd_map_put(map, "first", &a);
     hd_map_put(map, "second", &b);
     hd_map_put(map, "third", &c);
     
     // 打印所有键值对
     hd_map_print_all(map, "%d");
     
     // 检查是否包含key
     printf("Contains 'second': %d\n", hd_map_contains_key(map, "second"));
     printf("Contains 'fourth': %d\n", hd_map_contains_key(map, "fourth"));
     
     // 更新值
     int new_b = 25;
     hd_map_update(map, "second", &new_b);
     
     // 获取所有值
     int count;
     void** values = hd_map_get_all_values(map, &count);
     printf("All values (%d): ", count);
     for (int i = 0; i < count; i++) {
         printf("%d ", *((int*)values[i]));
     }
     printf("\n");
     free(values);
     
     // 删除键值对
     hd_map_remove(map, "first");
     printf("After removal, contains 'first': %d\n", hd_map_contains_key(map, "first"));
     
     // 检查是否为空
     printf("Map is empty: %d\n", hd_map_is_empty(map));
     
     // 反初始化Map
     hd_map_deinit(map);
     
     return 0;
 }