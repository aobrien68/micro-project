#include "disk.h"

uint8_t* disk_read(FILE* disk, int lba, uint8_t* buffer) {
    fseek(disk, lba * 512, SEEK_SET);
    fread(buffer, 1, 512, disk);
    return buffer;
}

uint8_t* disk_write(FILE* disk, int lba, uint8_t* buffer) {
    fseek(disk, lba * 512, SEEK_SET);
    fwrite(buffer, 1, 512, disk);
    return buffer;
}