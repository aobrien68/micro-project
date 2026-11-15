#include "disk.h"
#include "api.h"
#include <string.h>
#include <stdlib.h>

#define MAX_FILE_DESCRIPTORS (16)
#define min(a,b) (a<b ? a : b)

struct __attribute__((packed)) simpfs_info {
    struct simpfs_entry bitmap;
    struct simpfs_entry root_directory;
};

struct simpfs_file {
    bool is_open;
    uint16_t position;
    uint16_t last_sector_bytes_used;
    uint32_t start_lba;
    uint32_t length;
    uint32_t current_sector_offset;
    uint8_t buffer[SECTOR_SIZE];
};



FILE* disk = NULL;
struct simpfs_entry sector_zero_entry;
struct simpfs_info fs_info;
struct simpfs_file files[MAX_FILE_DESCRIPTORS];



bool mount_device(char* path) {
    disk = fopen(path, "r+b");
    return disk != NULL;
}

void unmount_device() {
    fclose(disk);
}

struct simpfs_entry new_simpfs_entry(char* name, uint8_t flags, uint32_t start_lba, uint32_t length, uint16_t last_sector_bytes_used) {
    struct simpfs_entry result;
    memset(result.name, 0, NAME_SIZE);
    strncpy(result.name, name, NAME_SIZE);
    flags |= SFS_IS_VALID;
    result.flags = flags;
    result.start_lba = start_lba;
    result.length = length;
    result.last_sector_bytes_used = last_sector_bytes_used;
    time_t current_time = time(NULL);
    result.created_date = current_time;
    result.modified_date = current_time;
    result.accesed_date = current_time;
    return result;
}

uint64_t simpfs_get_entry_size(struct simpfs_entry* entry) {
    return (entry->length - 1) * SECTOR_SIZE + entry->last_sector_bytes_used;
}

bool simpfs_format(uint64_t sectors_per_volume) {
    uint32_t bitmap_length = (sectors_per_volume + (SECTOR_SIZE * 8) - 1) / (SECTOR_SIZE * 8);

    uint16_t last_sector_bytes_used = (sectors_per_volume % (SECTOR_SIZE * 8)) / 8;
    if (!last_sector_bytes_used) {
        last_sector_bytes_used = SECTOR_SIZE;
    }
    
    fs_info.bitmap = new_simpfs_entry("bitmap", SFS_IS_TOP_LEVEL, 1, bitmap_length, last_sector_bytes_used);
    fs_info.root_directory = new_simpfs_entry("", SFS_IS_DIRECTORY | SFS_IS_TOP_LEVEL | SFS_IS_USER_READABLE, 1 + fs_info.bitmap.length, 1, 2 * sizeof(struct simpfs_entry));

    uint8_t buffer[SECTOR_SIZE];
    memset(buffer, 0, SECTOR_SIZE);
    memcpy(buffer, &fs_info, sizeof(struct simpfs_info));
    disk_write(disk, 0, buffer);



    // Reserve Sector 0 to End of Root and Free the rest
    // Reserve by sector
    uint32_t used_sectors = 1 + fs_info.bitmap.length + fs_info.root_directory.length;
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
    for (; bitmap_sector < fs_info.bitmap.length; bitmap_sector++) {
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
    sector_zero_entry = new_simpfs_entry("sector_zero", SFS_IS_DIRECTORY | SFS_IS_TOP_LEVEL, 0, 1, 2 * sizeof(struct simpfs_entry));

    for (int8_t i=0; i<MAX_FILE_DESCRIPTORS; i++) {
        files[i].is_open = false;
    }

    return true;
}

void simpfs_unmount() {
    
}

int8_t simpfs_open(struct simpfs_entry* entry) {
    for (int8_t i=0; i<MAX_FILE_DESCRIPTORS; i++) {
        if (!files[i].is_open) {
            files[i].is_open = true;

            disk_read(disk, entry->start_lba, files[i].buffer);
            files[i].position = 0;
            files[i].last_sector_bytes_used = entry->last_sector_bytes_used;
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

uint64_t simpfs_get_seek(int8_t descriptor) {
    return (files[descriptor].current_sector_offset * SECTOR_SIZE) + files[descriptor].position;
}

bool simpfs_seek(int8_t desciptor, uint64_t seek) {
    if (seek >= files[desciptor].length * SECTOR_SIZE) {
        return false;
    }
    uint32_t new_sector_offset = seek / SECTOR_SIZE;
    files[desciptor].position = seek % SECTOR_SIZE;
    if (new_sector_offset != files[desciptor].current_sector_offset) {
        disk_read(disk, files[desciptor].start_lba + new_sector_offset, files[desciptor].buffer);
        files[desciptor].current_sector_offset = new_sector_offset;
    }
    return true;
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
                file->current_sector_offset--;
                break;
            }
            file->position = 0;
            disk_read(disk, file->start_lba + file->current_sector_offset, file->buffer);
        }
    }

    return index;
}

uint32_t simpfs_write(int8_t descriptor, uint32_t n, uint8_t* buffer) {
    uint32_t index = 0;
    struct simpfs_file* file = &files[descriptor];

    if (file->current_sector_offset == file->length) {
        return 0;
    }

    while (n) {
        uint16_t left_in_buffer = SECTOR_SIZE - file->position;
        uint16_t put = min(n, left_in_buffer);
        memcpy(file->buffer + file->position, buffer + index, put);
        n -= put;
        index += put;
        file->position += put;
        if (file->position == SECTOR_SIZE) {
            disk_write(disk, file->start_lba + file->current_sector_offset, file->buffer);
            file->current_sector_offset++;
            if (file->current_sector_offset == file->length) {
                break;
            }
            file->position = 0;
        }
    }
    if (file->position) {
        disk_write(disk, file->start_lba + file->current_sector_offset, file->buffer);
    }

    return index;
}

uint32_t simpfs_fread(int8_t descriptor, uint32_t n, uint8_t* buffer) {
    uint64_t new_seek = simpfs_get_seek(descriptor);
    uint64_t max_seek = (files[descriptor].length - 1) * SECTOR_SIZE + files[descriptor].last_sector_bytes_used;
    if (new_seek == max_seek) {
        return 0;
    }
    new_seek += n;
    if (new_seek > max_seek) {
        uint32_t max_n = new_seek - max_seek;
        return simpfs_read(descriptor, min(n, max_n), buffer);
    }
    return simpfs_read(descriptor, n, buffer);
}

struct simpfs_entry simpfs_get_entry(struct simpfs_entry* directory, char* entry_name) {
    int8_t fd = simpfs_open(directory);
    uint64_t size = simpfs_get_entry_size(directory);
    struct simpfs_entry entry;
    for (uint64_t i=0; i<size; i+=sizeof(struct simpfs_entry)) {
        simpfs_read(fd, sizeof(struct simpfs_entry), (uint8_t*)&entry);
        if (!strncmp(entry.name, entry_name, NAME_SIZE)) {
            simpfs_close(fd);
            return entry;
        }
    }
    simpfs_close(fd);
    memset(&entry, 0, sizeof(struct simpfs_entry));
    return entry;
}

uint32_t simpfs_get_entries(struct simpfs_entry* directory, struct simpfs_entry* entries) {
    int8_t fd = simpfs_open(directory);
    uint64_t size = simpfs_get_entry_size(directory);
    struct simpfs_entry entry;
    uint32_t index = 0;
    for (uint64_t i=0; i<size; i+=sizeof(struct simpfs_entry)) {
        simpfs_read(fd, sizeof(struct simpfs_entry), (uint8_t*)&entry);
        if (entry.flags & SFS_IS_VALID) {
            entries[index] = entry;
            index++;
        }
    }
    simpfs_close(fd);
    return index;
}

struct simpfs_entry simpfs_get_entry_recursive(char* path) {
    if (*path == '/') {
        path++;
    }
    
    struct simpfs_entry current_entry = fs_info.root_directory;
    char buffer[NAME_SIZE + 1];
    char* slash = strchr(path, '/');
    while (slash) {
        memcpy(buffer, path, slash - path);
        buffer[slash - path] = '\0';
        current_entry = simpfs_get_entry(&current_entry, buffer);
        if (!(current_entry.flags & SFS_IS_VALID)) {
            return current_entry;
        }
        path = slash + 1;
        slash = strchr(path, '/');
    }
    if (!*path) {
        return current_entry;
    }
    return simpfs_get_entry(&current_entry, path);
}

bool simpfs_update_entry(struct simpfs_entry* directory, struct simpfs_entry* old_entry, struct simpfs_entry* new_entry) {
    int8_t fd = simpfs_open(directory);
    uint64_t size = simpfs_get_entry_size(directory);
    for (uint64_t seek = 0; seek < size; seek += sizeof(struct simpfs_entry)) {
        struct simpfs_entry read_entry;
        simpfs_read(fd, sizeof(struct simpfs_entry), (uint8_t*)&read_entry);
        if (!memcmp(old_entry->name, read_entry.name, NAME_SIZE)) {
            simpfs_seek(fd, seek);
            simpfs_write(fd, sizeof(struct simpfs_entry), (uint8_t*)new_entry);
            simpfs_close(fd);
            return true;
        }
    }
    simpfs_close(fd);
    return false;
}

// TODO support root maybe
bool simpfs_update_entry_recursive(struct simpfs_entry* directory, struct simpfs_entry* old_entry, struct simpfs_entry* new_entry) {
    simpfs_update_entry(directory, old_entry, new_entry);

    memset(new_entry->name, 0, NAME_SIZE);
    new_entry->name[0] = '.';
    memset(old_entry->name, 0, NAME_SIZE);
    old_entry->name[0] = '.';
    simpfs_update_entry(old_entry, old_entry, new_entry);

    struct simpfs_entry entries[simpfs_get_entry_size(old_entry) / sizeof(struct simpfs_entry)];
    uint32_t num_entries = simpfs_get_entries(old_entry, entries);
    new_entry->name[1] = '.';
    old_entry->name[1] = '.';
    for (uint32_t i=2; i<num_entries; i++) {
        if (entries[i].flags & SFS_IS_DIRECTORY) {
            simpfs_update_entry(&entries[i], old_entry, new_entry);
        }
    }

    return true;
}

// TODO fix adding entry needing to resize
bool simpfs_add_entry(struct simpfs_entry* directory, struct simpfs_entry* entry) {
    int8_t fd = simpfs_open(directory);
    uint64_t size = simpfs_get_entry_size(directory);
    for (uint64_t seek = 0; seek < size; seek += sizeof(struct simpfs_entry)) {
        struct simpfs_entry read_entry;
        simpfs_read(fd, sizeof(struct simpfs_entry), (uint8_t*)&read_entry);
        if (!(read_entry.flags & SFS_IS_VALID)) {
            simpfs_seek(fd, seek);
            bool result = simpfs_write(fd, sizeof(struct simpfs_entry), (uint8_t*)entry) != 0;
            simpfs_close(fd);
            return result;
        }
    }
    bool result = simpfs_write(fd, sizeof(struct simpfs_entry), (uint8_t*)entry) != 0;
    simpfs_close(fd);
    return result;
}

uint32_t simpfs_allocate(uint32_t sectors) {
    int8_t fd = simpfs_open(&fs_info.bitmap);


    simpfs_close(fd);
}

void test() {
    int8_t fd = simpfs_open(&fs_info.bitmap);
    uint8_t test_byte;
    printf("1=%u\n", simpfs_fread(fd, 1, &test_byte));
    printf("7=%x\n", test_byte);
    uint8_t buffer[256];
    printf("254=%u\n", simpfs_fread(fd, 254, buffer));
    printf("1=%u\n", simpfs_fread(fd, 2, buffer));
    printf("0=%u\n", simpfs_fread(fd, 1, buffer));
    simpfs_close(fd);
}