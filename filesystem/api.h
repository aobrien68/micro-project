#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define SECTOR_SIZE (512)
#define NAME_SIZE (29)

enum simpfs_flags {
    SFS_IS_VALID = 0x01,
    SFS_IS_DIRECTORY = 0x02,
    SFS_IS_EXECUTABLE = 0x04,
    SFS_IS_DEVICE = 0x08,
    SFS_IS_READ_ONLY = 0x10,
    SFS_IS_TOP_LEVEL = 0x20,
    SFS_IS_USER_READABLE = 0x40,
    SFS_IS_USER_WRITABLE = 0x80
};

struct __attribute__((packed)) simpfs_entry {
    uint8_t name[NAME_SIZE];
    uint8_t flags;
    uint16_t last_sector_bytes_used;
    uint32_t start_lba;
    uint32_t length;
    time_t created_date;
    time_t modified_date;
    time_t accesed_date;
};

bool mount_device(char* path);
void unmount_device();
uint64_t simpfs_get_entry_size(struct simpfs_entry* entry);
bool simpfs_format(uint64_t sectors_per_volume);
bool simpfs_mount();
void simpfs_unmount();
uint32_t simpfs_get_entries(struct simpfs_entry* directory, struct simpfs_entry* entries);
struct simpfs_entry simpfs_get_entry_recursive(char* path);
bool simpfs_create_file_from_path(char* path, uint8_t flags);

void test();