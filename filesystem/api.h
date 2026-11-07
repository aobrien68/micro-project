#pragma once

#include <stdint.h>
#include <stdbool.h>

bool mount_device(char* path);
void unmount_device();
bool simpfs_format(uint64_t sectors_per_volume);
bool simpfs_mount();
void simpfs_unmount();