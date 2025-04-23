#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Base64编码表
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// 将二进制数据转换为Base64
char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length) {
    *output_length = 4 * ((input_length + 2) / 3);
    
    char *encoded_data = malloc(*output_length + 1);
    if (encoded_data == NULL) return NULL;
    
    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;
        
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        
        encoded_data[j++] = base64_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 0 * 6) & 0x3F];
    }
    
    // 添加填充字符'='
    for (size_t i = 0; i < (3 - input_length % 3) % 3; i++) {
        encoded_data[*output_length - 1 - i] = '=';
    }
    
    encoded_data[*output_length] = '\0';
    return encoded_data;
}


// gcc -o image_to_base64 test_image_to_base64.c
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image_file>\n", argv[0]);
        return 1;
    }
    
    const char *filename = argv[1];
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 读取文件内容
    unsigned char *file_data = malloc(file_size);
    if (!file_data) {
        perror("Memory allocation failed");
        fclose(file);
        return 1;
    }
    
    if (fread(file_data, 1, file_size, file) != file_size) {
        perror("Failed to read file");
        free(file_data);
        fclose(file);
        return 1;
    }
    
    fclose(file);
    
    // 转换为Base64
    size_t base64_length;
    char *base64_data = base64_encode(file_data, file_size, &base64_length);
    if (!base64_data) {
        perror("Base64 encoding failed");
        free(file_data);
        return 1;
    }
    
    // 输出结果
    printf("Base64 encoded data:\n%s\n", base64_data);
    
    // 清理内存
    free(file_data);
    free(base64_data);
    
    return 0;
}