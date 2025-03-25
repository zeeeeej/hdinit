#include <stdio.h>
#include <dlfcn.h>
#include <signal.h>
#include "hdmain.h"

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    running = 0;
}

int main() {
    printf("hdmain service started (PID: %d)\n", getpid());
    signal(SIGTERM, handle_signal);

    void *plugin = dlopen("/usr/lib/hdmain/plugins/core_plugin.so", RTLD_NOW);
    if (!plugin) {
        fprintf(stderr, "Failed to load plugin: %s\n", dlerror());
        return 1;
    }

    plugin_entry_t entry = (plugin_entry_t)dlsym(plugin, PLUGIN_ENTRY_FUNC);
    if (!entry) {
        fprintf(stderr, "Failed to find entry: %s\n", dlerror());
        dlclose(plugin);
        return 1;
    }

    // 主循环交给插件完全控制
    while (running) {
        if (entry() == PLUGIN_EXIT) {
            break;
        }
    }

    dlclose(plugin);
    printf("hdmain service stopped\n");
    return 0;
}
