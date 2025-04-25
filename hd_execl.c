#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program> [args...]\n", argv[0]);
        return 1;
    }

    // 设置环境变量数组（注意：会完全替换子进程的环境变量）
    char *env[] = {
        "QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event1",
        "QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0",
        "QT_QPA_FONTDIR=/usr/lib/fonts/",
        NULL  // 必须以NULL结尾
    };

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return 1;
    } else if (pid == 0) { // 子进程
        // 构造exec参数（保留原程序参数）
        char **exec_args = &argv[1];
        
        execle(exec_args[0], exec_args[0], (char *)NULL, env);
        // 只有exec失败才会执行到这里
        perror("execle failed");
        _exit(1);  // 使用_exit避免刷新stdio缓冲区
    } else { // 父进程
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Child exited with status %d\n", WEXITSTATUS(status));
            return WEXITSTATUS(status);
        } else {
            printf("Child terminated abnormally\n");
            return 1;
        }
    }
}
