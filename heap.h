//
// Created by root on 12/20/22.
//

#ifndef ALOKATOR_HEAP_H
#define ALOKATOR_HEAP_H
#include <stdio.h>

enum pointer_type_t
{
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};

int heap_setup(void);
void heap_clean(void);
//int update_checksum(struct memory_chunk_t* mem);
void* heap_malloc(size_t size);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t count);
void heap_free(void* memblock);
int heap_validate(void);
int checkFreeBlock(size_t size);
size_t checkHeapSize();
size_t heap_get_largest_used_block_size(void);
enum pointer_type_t get_pointer_type(const void* const pointer);
#endif //ALOKATOR_HEAP_H
