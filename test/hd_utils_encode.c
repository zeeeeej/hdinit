#include <stdio.h>
#include "../hd_utils.h"


int main() {
   // 测试 encode
    char encoded[100];
    if (hd_child_info_encode(encoded, "Device1", 12345, "1.0.0") == 0) {
        printf("Encoded: %s\n", encoded); // 输出: "Device1,12345,1.0.0"
    }

    // 测试 decode（注意 s_id 需为指针）
    char name[50], version[50];
    int decoded_id;
    if (hd_child_info_decode("Device2,67890,2.0.1", name, &decoded_id, version) == 0) {
        printf("Decoded: Name=%s, ID=%d, Version=%s\n", name, decoded_id, version); // 输出: Name=Device2, ID=67890, Version=2.0.1
    }

    return 0;
}