#include "api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_INPUT_SIZE (80)

char** get_args(int* argc, char* buffer) {
    *buffer = '\0';
    while (!*buffer) {
        printf("[simpfs]: ");
        char c = getchar();
        uint8_t i;
        for (i=0; i<MAX_INPUT_SIZE && c != '\n'; i++) {
            buffer[i] = c;
            c = getchar();
        }
        buffer[i] = '\0';
    }

    char* ptr = buffer;
    uint8_t count = 1;
    while (*ptr) {
        if (*ptr == ' ') {
            count++;
        }
        ptr++;
    }
    char** argv = malloc(count * sizeof(char*));
    *argc = count;

    char* current_arg = buffer;
    char* next_arg = strchr(current_arg, ' ');
    for (int i=0;; i++) {
        if (next_arg) {
            uint8_t length = next_arg - current_arg;
            argv[i] = malloc(length + 1);
            memcpy(argv[i], current_arg, length);
            argv[i][length] = '\0';
        } else {
            uint8_t length = strlen(current_arg);
            argv[i] = malloc(length + 1);
            memcpy(argv[i], current_arg, length);
            argv[i][length] = '\0';
            break;
        }
        current_arg = next_arg + 1;
        next_arg = strchr(current_arg, ' ');
    }

    return argv;
}

int main(int argc, char* argv[]) {
    // if (argc < 2) {
    //     printf("simpfs: missing operand\n");
    //     return 1;
    // }

    // if (!mount_device(argv[1])) {
    //     printf("simpfs: cannot find device\n");
    //     return 2;
    // }

    char input_buffer[MAX_INPUT_SIZE + 1];
    bool time_to_exit = false;
    
    while(!time_to_exit) {
        int simpfs_argc;
        char** simpfs_argv = get_args(&simpfs_argc, input_buffer);

        if (!strcmp(simpfs_argv[0], "exit")) {
            for (int i=0; i<simpfs_argc; i++) {
                free(simpfs_argv[i]);
            }
            free(simpfs_argv);
            break;
        }

        switch (strlen(simpfs_argv[0])) {
        case 4:
            if (!strcmp(simpfs_argv[0], "exit")) {
                time_to_exit = true;
            break;
        }
        break;
        default:
            break;
        }

        for (int i=0; i<simpfs_argc; i++) {
            free(simpfs_argv[i]);
        }
        free(simpfs_argv);
    }
}