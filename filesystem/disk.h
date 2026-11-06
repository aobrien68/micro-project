#pragma once

#include <stdint.h>
#include <stdio.h>

uint8_t* disk_read(FILE* disk, int lba, uint8_t* buffer);
uint8_t* disk_write(FILE* disk, int lba, uint8_t* buffer);