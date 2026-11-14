#include "api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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

char* flag_str = "dxrtrw";

char* ls_line(struct simpfs_entry entry, bool is_human_readable) {
    char flags[8];
    for (uint8_t i=0; i<7; i++) {
        if (entry.flags & (2 << i)) {
            flags[i] = flag_str[i];
        } else {
            flags[i] = '-';
        }
    }
    flags[7] = '\0';

    uint64_t bytes = (entry.length - 1) * SECTOR_SIZE + entry.last_sector_bytes_used;

    char* time = ctime(&entry.modified_date);
    uint8_t time_length = strlen(time) - 1;
    time[time_length] = '\0';

    char name[30];
    strncpy(name, entry.name, 29);
    name[29] = '\0';

    uint32_t result_size = 7 + 1 + 20 + 1 + time_length + 1 + 29 + 1;
    char* result_str = malloc(result_size);
    memset(result_str, 0, result_size);
    sprintf(result_str, "%s %lu %s %s", flags, bytes, time, name);
    return result_str;
}

// TODO: Implement full and name buffering
void ls(char* path, bool flag_all, bool flag_line, bool flag_human_readable) {
    struct simpfs_entry directory = simpfs_get_entry_recursive(path);
    if (!(directory.flags & SFS_IS_VALID)) {
        printf("ls: could not find %s\n", path);
        return;
    }
    if (directory.flags & SFS_IS_DIRECTORY) {
        struct simpfs_entry entries[((directory.length - 1) * SECTOR_SIZE + directory.last_sector_bytes_used) / sizeof(struct simpfs_entry)];
        uint32_t length = simpfs_get_entries(&directory, entries);

        if (flag_line) {
            for (int i=0; i<length; i++) {
                if (entries[i].name[0] != '.' || flag_all) {
                    char* line = ls_line(entries[i], flag_human_readable);
                    puts(line);
                    free(line);
                }
            }
        } else {
            for (int i=0; i<length; i++) {
                if (entries[i].name[0] != '.' || flag_all) {
                    printf("%s ", entries[i].name);
                }
            }
            putchar('\n');
        }
    } else {
        puts(directory.name);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("simpfs: missing operand\n");
        return 1;
    }

    if (!mount_device(argv[1])) {
        printf("simpfs: cannot find device\n");
        return 2;
    }

    simpfs_mount();

    char input_buffer[MAX_INPUT_SIZE + 1];
    bool time_to_exit = false;

    while(!time_to_exit) {
        int simpfs_argc;
        char** simpfs_argv = get_args(&simpfs_argc, input_buffer);

        if (!strcmp(simpfs_argv[0], "exit")) {
            for (uint8_t i=0; i<simpfs_argc; i++) {
                free(simpfs_argv[i]);
            }
            free(simpfs_argv);
            break;
        }

        switch (strlen(simpfs_argv[0])) {
        case 2:
            if (!strcmp(simpfs_argv[0], "ls")) {
                switch (simpfs_argc) {
                case 2:
                    ls(simpfs_argv[1], true, true, false);
                    break;
                case 3:
                    break;
                default:
                    printf("ls: usage ls <path>\n");
                }
            } 
            break;
        case 4:
            if (!strcmp(simpfs_argv[0], "exit")) {
                time_to_exit = true;
            } else if (!strcmp(simpfs_argv[0], "test")) {
                test();
            }
            break;
        case 6:
            if (!strcmp(simpfs_argv[0], "format")) {
                if (simpfs_argc != 2) {
                    printf("format: usage format <number of sectors>\n");
                    break;
                }
                uint64_t num_sectors= atol(simpfs_argv[1]);
                if (!num_sectors && simpfs_argv[1][0] != '0') {
                    printf("format: usage format <number of sectors>\n");
                    break;
                }
                if (num_sectors < 3 || num_sectors > 0x100000000) {
                    printf("format: <number of sector> needs to be in range 2 < n <= 2^32\n");
                    break;
                }
                simpfs_format(num_sectors);
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

    unmount_device();
}