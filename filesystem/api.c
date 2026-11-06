#include "disk.h"
#include "api.h"
#include <string.h>
#include <stdlib.h>

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
    uint8_t name[29];
    uint8_t flags;
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
    uint16_t last_sector_bits_used;
};

struct simpfs_file {
    uint8_t buffer[SECTOR_SIZE];
    uint32_t current_sector;
    uint16_t position;
};

FILE* disk = NULL;
struct simpfs_info fs_info;
uint8_t* bitmap;

bool mount_device(char* path) {
    disk = fopen(path, "r+b");
    return disk != NULL;
}

void unmount_device() {
    fclose(disk);
}

bool simpfs_format(uint64_t sectors_per_volume, uint32_t sectors_per_root_directory) {
    if (sectors_per_volume < (2 + sectors_per_root_directory) || sectors_per_volume > 0x100000000) {
        printf("Error: sectors_per_volume at least 2 + sectors_per_directory and at most 2^32");
        return false;
    }
    fs_info.bitmap_length = (sectors_per_volume + (SECTOR_SIZE * 8) - 1) / (SECTOR_SIZE * 8);
    if (!sectors_per_root_directory || sectors_per_root_directory + fs_info.bitmap_length + 1 > sectors_per_volume) {
        printf("Error: sectors_per_root_directory is zero or too big");
        return false;
    }

    fs_info.last_sector_bits_used = sectors_per_volume % (SECTOR_SIZE * 8);
    if (!fs_info.last_sector_bits_used) {
        fs_info.last_sector_bits_used = (SECTOR_SIZE * 8);
    }

    fs_info.max_sector_index = sectors_per_volume - 1;
    fs_info.root_directory_length = sectors_per_root_directory;
    uint8_t buffer[SECTOR_SIZE];
    memset(buffer, 0, SECTOR_SIZE);
    memcpy(buffer, &fs_info, sizeof(struct simpfs_info));
    disk_write(disk, 0, buffer);



    // Reserve Sector 0 to End of Root and Free the rest
    // Reserve by sector
    uint32_t used_sectors = 1 + fs_info.bitmap_length + fs_info.root_directory_length;
    memset(buffer, 0xFF, SECTOR_SIZE);
    uint32_t bitmap_sector;
    uint32_t max_full_bitmap_sector = used_sectors / (SECTOR_SIZE * 8);
    for (bitmap_sector = 0; bitmap_sector < max_full_bitmap_sector; bitmap_sector++) {
        disk_write(disk, 1 + bitmap_sector, buffer);
    }

    // Reserve by byte
    used_sectors = used_sectors % (SECTOR_SIZE * 8);
    if (used_sectors) {
        memset(buffer, 0, SECTOR_SIZE);
        uint16_t bitmap_byte;
        uint16_t max_full_bitmap_byte = used_sectors / 8;
        for (bitmap_byte = 0; bitmap_byte < max_full_bitmap_byte; bitmap_byte++) {
            buffer[bitmap_byte] = 0xFF;
        }
    
        // Reserve by bit
        used_sectors = used_sectors % 8;
        if (used_sectors) {
            uint8_t byte = 0;
            for (uint8_t bitmap_bit = 0; bitmap_bit < used_sectors; bitmap_bit++) {
                byte |= 1 << bitmap_bit;
            }
            buffer[bitmap_byte] = byte;
        }
        disk_write(disk, 1 + bitmap_sector, buffer);
        bitmap_sector++;
    }

    // Free by sector
    memset(buffer, 0, SECTOR_SIZE);
    for (; bitmap_sector < fs_info.bitmap_length; bitmap_sector++) {
        disk_write(disk, 1 + bitmap_sector, buffer);
    }


    // Clear Root Directory
    memset(buffer, 0, SECTOR_SIZE);
    for (uint32_t i=0; i<fs_info.root_directory_length; i++) {
        disk_write(disk, 1 + fs_info.bitmap_length + i, buffer);
    }

    return true;
}

bool simpfs_mount() {
    uint8_t buffer[SECTOR_SIZE];
    disk_read(disk, 0, buffer);
    memcpy(&fs_info, buffer, sizeof(struct simpfs_info));

    bitmap = malloc(fs_info.bitmap_length * SECTOR_SIZE);
}

void simpfs_unmount() {
    free(bitmap);
}

// void simpfs_add_entry(struct ) {

// }