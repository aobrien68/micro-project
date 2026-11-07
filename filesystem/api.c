#include "disk.h"
#include "api.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define SECTOR_SIZE (512)
#define NAME_SIZE (29)
#define MAX_FILE_DESCRIPTORS (16)
#define min(a,b) (a<b ? a : b)

typedef int64_t super_time_t;

enum simpfs_flags {
    SFS_IS_DIRECTORY = 0x01,
    SFS_IS_EXECUTABLE = 0x02,
    SFS_IS_READ_ONLY = 0x04,
    SFS_IS_USER_READABLE = 0x08,
    SFS_IS_USER_WRITABLE = 0x10
};

struct __attribute__((packed)) simpfs_entry {
    uint8_t name[NAME_SIZE];
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
    uint16_t last_sector_bits_used;
    uint8_t reserved[54];
    struct simpfs_entry root_directory;
};


struct simpfs_file {
    bool is_open;
    uint16_t position;
    uint32_t start_lba;
    uint32_t length;
    uint32_t current_sector_offset;
    uint8_t buffer[SECTOR_SIZE];
};



FILE* disk = NULL;
struct simpfs_info fs_info;
uint8_t* bitmap;
struct simpfs_file files[MAX_FILE_DESCRIPTORS];



bool mount_device(char* path) {
    disk = fopen(path, "r+b");
    return disk != NULL;
}

void unmount_device() {
    fclose(disk);
}

bool simpfs_format(uint64_t sectors_per_volume) {
    fs_info.bitmap_length = (sectors_per_volume + (SECTOR_SIZE * 8) - 1) / (SECTOR_SIZE * 8);

    fs_info.last_sector_bits_used = sectors_per_volume % (SECTOR_SIZE * 8);
    if (!fs_info.last_sector_bits_used) {
        fs_info.last_sector_bits_used = (SECTOR_SIZE * 8);
    }

    fs_info.max_sector_index = sectors_per_volume - 1;
    
    // Create Root Directory Entry
    memset(&fs_info.root_directory.name, 0, NAME_SIZE);
    fs_info.root_directory.flags = SFS_IS_DIRECTORY | SFS_IS_USER_READABLE;
    fs_info.root_directory.last_sector_bytes_used = 2 * sizeof(struct simpfs_entry);
    fs_info.root_directory.start_lba = 1 + fs_info.bitmap_length;
    fs_info.root_directory.length = 1;
    fs_info.root_directory.created_date = time(NULL);
    fs_info.root_directory.modified_date = fs_info.root_directory.created_date;
    fs_info.root_directory.accesed_date = 0;


    uint8_t buffer[SECTOR_SIZE];
    memset(buffer, 0, SECTOR_SIZE);
    memcpy(buffer, &fs_info, sizeof(struct simpfs_info));
    disk_write(disk, 0, buffer);



    // Reserve Sector 0 to End of Root and Free the rest
    // Reserve by sector
    uint32_t used_sectors = 1 + fs_info.bitmap_length + fs_info.root_directory.length;
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


    // Create Root Directory
    memset(buffer, 0, SECTOR_SIZE);
    memcpy(buffer, &fs_info.root_directory, sizeof(struct simpfs_entry));
    memcpy(buffer + sizeof(struct simpfs_entry), &fs_info.root_directory, sizeof(struct simpfs_entry));
    struct simpfs_entry* entry = (struct simpfs_entry*)buffer;
    entry->name[0] = '.';
    entry = (struct simpfs_entry*)(buffer + sizeof(struct simpfs_entry));
    entry->name[0] = '.';
    entry->name[1] = '.';
    disk_write(disk, fs_info.root_directory.start_lba, buffer);
    memset(buffer, 0, SECTOR_SIZE);
    for (uint32_t i=1; i<fs_info.root_directory.length; i++) {
        disk_write(disk, fs_info.root_directory.start_lba + i, buffer);
    }

    return true;
}

bool simpfs_mount() {
    uint8_t buffer[SECTOR_SIZE];
    disk_read(disk, 0, buffer);
    memcpy(&fs_info, buffer, sizeof(struct simpfs_info));

    // bitmap = malloc(fs_info.bitmap_length * SECTOR_SIZE);
    // if (!bitmap) {
    //     return false;
    // }
}

void simpfs_unmount() {
    //free(bitmap);
}

int8_t simpfs_open(struct simpfs_entry* entry) {
    for (int8_t i=0; i<MAX_FILE_DESCRIPTORS; i++) {
        if (!files[i].is_open) {
            files[i].is_open = true;

            disk_read(disk, entry->start_lba, files[i].buffer);
            files[i].position = 0;
            files[i].start_lba = entry->start_lba;
            files[i].length = entry->length;
            files[i].current_sector_offset = 0;
            return i;
        }
    }

    return -1;
}

int8_t simpfs_close(int8_t descriptor) {
    files[descriptor].is_open = false;
}

uint32_t simpfs_read(int8_t descriptor, uint32_t n, uint8_t* buffer) {
    uint32_t index = 0;
    struct simpfs_file* file = &files[descriptor];

    if (file->current_sector_offset == file->length) {
        return 0;
    }

    while (n) {
        uint16_t left_in_buffer = SECTOR_SIZE - file->position;
        uint16_t take = min(n, left_in_buffer);
        memcpy(buffer + index, file->buffer + file->position, take);
        n -= take;
        index += take;
        file->position += take;
        if (file->position == SECTOR_SIZE) {
            file->current_sector_offset++;
            if (file->current_sector_offset == file->length) {
                break;
            }
            file->position = 0;
            disk_read(disk, file->start_lba + file->current_sector_offset, file->buffer);
        }
    }

    return index;
}

void test() {
    simpfs_mount();
    int8_t fd = simpfs_open(&fs_info.root_directory);
    uint8_t buffer[sizeof(struct simpfs_entry)];
    simpfs_read(fd, NAME_SIZE, buffer);
    puts(buffer);
    simpfs_read(fd, 1, buffer);
    printf("Flags: %x\n", buffer[0]);
    simpfs_read(fd, 2, buffer);
    uint16_t bytes = (buffer[1] << 8) + buffer[0];
    printf("Bytes: %u\n", bytes);
    simpfs_read(fd, 4, buffer);
    uint32_t u32 = (buffer[3] << 24) + (buffer[2] << 16) + (buffer[1] << 8) + buffer[0];
    printf("LBA: %u\n", u32);
    simpfs_read(fd, 4, buffer);
    u32 = (buffer[3] << 24) + (buffer[2] << 16) + (buffer[1] << 8) + buffer[0];
    printf("Length: %u\n", u32);
    simpfs_read(fd, 8, buffer);
    time_t time = (buffer[3] << 24) + (buffer[2] << 16) + (buffer[1] << 8) + buffer[0];
    printf("Time: %s", ctime(&time));
    simpfs_read(fd, 16, buffer);
    simpfs_read(fd, NAME_SIZE, buffer);
    puts(buffer);

    simpfs_close(fd);
}