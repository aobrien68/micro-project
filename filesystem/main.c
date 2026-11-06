#include "api.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("simpfs: missing operand\n");
        return 1;
    }

    if (!mount_device(argv[1])) {
        printf("simpfs: cannot find device\n");
        return 2;
    }

    simpfs_format(2048, 8);
    unmount_device();

    // while (1) {
    //     putchar(getchar());
    // }
}