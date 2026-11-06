#include "disk.h"

int main() {
    FILE* disk = fopen("disk", "r+b");
    uint8_t buffer[512];
    disk_read(disk, 0, buffer);
    puts(buffer);
}