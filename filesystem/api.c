#include "disk.h"
#include "string.h"
#include "stdlib.h"

#define SECTOR_SIZE 512

typedef int64_t super_time_t;

enum simpfs_flags {
    SFS_IS_DIRECTORY = 0x01,
    SFS_IS_EXECUTABLE = 0x02,
    SFS_IS_READ_ONLY = 0x04,
    SFS_IS_USER_READABLE = 0x08,
    SFS_IS_USER_WRITABLE = 0x10
};

struct __attribute__((packed)) simpfs_entry {
    uint8_t flags;
    uint8_t name[29];
    uint16_t last_sector_bytes_used;
    uint32_t start_lba;
    uint32_t length;
    super_time_t created_date;
    super_time_t modified_date;
    super_time_t accesed_date;
};

struct __attribute__((packed)) simpfs_info {
    uint32_t max_sector_index;
    uint32_t bitmap_length;
    uint32_t root_directory_length;
};

struct simpfs_file {
    uint8_t buffer[SECTOR_SIZE];
    uint32_t current_sector;
    uint16_t position;
};

FILE* disk;
struct simpfs_info fs_info;
uint8_t* bitmap;


void simpfs_init() {
    disk = fopen("disk", "r+b");
    uint8_t buffer[SECTOR_SIZE];
    disk_read(disk, 0, buffer);
    memcpy(&fs_info, buffer, sizeof(struct simpfs_info));
    printf("Sectors Per Volume: %u\n", fs_info.max_sector_index + 1);
    printf("Sectors Per Bitmap: %u\n", fs_info.bitmap_length);
    printf("Sectors Per Root: %u\n", fs_info.root_directory_length);
    fclose(disk);
}