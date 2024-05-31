//
// Created by root on 12/20/22.
//

#include "heap.h"
#include "custom_unistd.h"
#include <stdio.h>
#include <string.h>
//#include "tested_declarations.h"
//#include "rdebug.h"

#define PAGE_SIZE 4096
#define FENCE_SIZE 16
#define CTRL_BLOCK_SIZE sizeof(struct memory_chunk_t)
#define PADDING 4
#define ROUND(ptr) (ptr + PAGE_SIZE)/PAGE_SIZE*PAGE_SIZE
struct memory_manager_t{
    void *memory_start;
    size_t memory_size;
    struct memory_chunk_t *first_memory_chunk;
    int initialized;
};

struct memory_chunk_t{
    size_t size;
    struct memory_chunk_t *prev;
    struct memory_chunk_t *next;
    void *user_mem_end;
    int free;
    int checksum;
};

struct memory_manager_t memoryManager;

int heap_setup(void){
    void* memory_start = custom_sbrk(0);
    if(memory_start == (void*)-1) return -1;

    memoryManager.memory_start = memory_start;
    memoryManager.first_memory_chunk = NULL;
    memoryManager.initialized = 1;
    memoryManager.memory_size = 0;
    return 0;
}
void heap_clean(void){
    if(memoryManager.initialized){
        char* start = (char*)memoryManager.memory_start;
        for(long unsigned int i = 0; i < memoryManager.memory_size; i++){
            *(start + i) = 0;
        }
        custom_sbrk(-memoryManager.memory_size);
        memoryManager.memory_size = 0;
        memoryManager.memory_start = NULL;
        memoryManager.first_memory_chunk = NULL;
        memoryManager.initialized = 0;
    }
}
int update_checksum(struct memory_chunk_t* mem){
    int checksum = 0;
    for(long unsigned int i = 0; i < CTRL_BLOCK_SIZE - sizeof(int); i++){
        checksum += *((unsigned char*)mem + i);
    }
    return checksum;
}
size_t checkHeapSize(){
    struct memory_chunk_t* tmp = memoryManager.first_memory_chunk;
    size_t total = 0;
    while(1){
        if(tmp == NULL) break;
        total = total + CTRL_BLOCK_SIZE + tmp->size + FENCE_SIZE;
        tmp = tmp->next;
    }
    return total;
}
int checkFreeBlock(size_t size){
    struct memory_chunk_t* mem = memoryManager.first_memory_chunk;
    int found = -1;
    for(int i = 0;;i++){
        if(mem == NULL) break;
        if(mem->size - 16 >= size && mem->free == 1){
            found = i;
            return found;
        }
        mem = mem->next;
    }
    return found;
}
int heap_validate(void){
    if(!memoryManager.initialized) return 2;

    struct memory_chunk_t* tmp = memoryManager.first_memory_chunk;
    struct memory_chunk_t tmp2;
    void* block = NULL;
    void* endblock = NULL;
    int checksum;
    while(tmp){
        block = ((char*)tmp + CTRL_BLOCK_SIZE);

        if(((char*)tmp->user_mem_end < ((char*)tmp + CTRL_BLOCK_SIZE + FENCE_SIZE / 2) || (char*)tmp->user_mem_end >= ((char*)tmp + CTRL_BLOCK_SIZE + FENCE_SIZE + tmp->size))) return 3;

        endblock = ((char*)tmp->user_mem_end + 1);
        for(long unsigned int i = 0; i < FENCE_SIZE/2 ; i++){
            if((*((char*)block + i) != '#' || *((char*)endblock + i) != '#') && tmp->free == 0) return 1;
        }

        memcpy(&tmp2, tmp, CTRL_BLOCK_SIZE);
        block = &tmp2;
        checksum = 0;
        for(long unsigned int i = 0; i < CTRL_BLOCK_SIZE - sizeof(int); i++){
            checksum += *((unsigned char*)block + i);
        }
        if(checksum != tmp->checksum){
            return 3;
        }
        if(tmp->next){
            if(tmp->next->prev != tmp) return 3;
        }
        if(tmp->prev){
            if(tmp->prev->next != tmp) return 3;
        }
        tmp = tmp->next;
    }
    return 0;
}
void* heap_malloc(size_t size){
    size_t total = 0;
    struct memory_chunk_t* tmp = NULL;
    struct memory_chunk_t* mem = NULL;
    if(size < 1 || !memoryManager.initialized) return NULL;
    if(!memoryManager.first_memory_chunk){
        size_t full_size = 0;
        full_size = (size + CTRL_BLOCK_SIZE + FENCE_SIZE);
        mem = custom_sbrk(full_size);
        if(mem == (void*)-1) return NULL;
        mem->size = size;
        mem->next = NULL;
        mem->prev = NULL;
        mem->free = 0;
        mem->user_mem_end = ((char*)mem + CTRL_BLOCK_SIZE +FENCE_SIZE/2 + size - 1);
        mem->checksum = update_checksum(mem);
        memoryManager.memory_size += full_size;
        memoryManager.first_memory_chunk = mem;
    }
    else{
        if(heap_validate()) return NULL;
        mem = memoryManager.first_memory_chunk;
        int a = 0;
        while(1){
            if(mem == NULL) break;
            a = checkFreeBlock(size);
            if(a != -1){
                for(int i = 0; i < a; i++){
                    tmp = mem;
                    mem = mem->next;
                }
                mem->size = size;
                if(a){
                    tmp->next = mem;
                    mem->prev = tmp;
                }
                mem->free = 0;
                mem->user_mem_end = ((char*)mem + CTRL_BLOCK_SIZE + FENCE_SIZE/2 + size - 1);
                mem->checksum = update_checksum(mem);
                break;
            }
            else{
                size_t full_size = 0;
                full_size = (size + CTRL_BLOCK_SIZE + FENCE_SIZE);
                void *res = custom_sbrk(full_size);
                if(res == (void*)-1) return NULL;
                memoryManager.memory_size += full_size;
                for(int i = 0;;i++){
                    total = total + mem->size + CTRL_BLOCK_SIZE + FENCE_SIZE;
                    tmp = mem;
                    if(mem->next == NULL) break;
                    mem = mem->next;
                }
                mem = (struct memory_chunk_t*)((char*)mem + mem->size + CTRL_BLOCK_SIZE + FENCE_SIZE);

                tmp->next = mem;
                mem->prev = tmp;
                mem->next = NULL;
                mem->size = size;
                mem->user_mem_end = ((char*)mem + CTRL_BLOCK_SIZE + FENCE_SIZE/2 + size - 1);
                mem->free = 0;
                mem->checksum = update_checksum(mem);
                tmp->checksum = update_checksum(tmp);
            }
            break;
        }
    }
    char* end = (char*)((char*)mem + FENCE_SIZE - 1 + mem->size + CTRL_BLOCK_SIZE);
    for(int i = 0;i < FENCE_SIZE/2; i++){
        *((char*)mem + CTRL_BLOCK_SIZE + i) = '#';
        *(end - i) = '#';
    }
    return (void*)((char*)mem + CTRL_BLOCK_SIZE + 8);
}
void* heap_calloc(size_t number, size_t size){
    if(number < 1 || size < 1) return NULL;
    void* res = heap_malloc(number * size);
    if(!res) return NULL;
    for(int i = 0; i < (int)(number * size); i++){
        *((char*)res + i) = 0;
    }
    return res;
}
void* heap_realloc(void* memblock, size_t count){
    if(!memblock && count > 0) return heap_malloc(count);
    else if(count == 0){
        heap_free(memblock);
        return NULL;
    }
    else if(get_pointer_type(memblock) != pointer_valid) return NULL;

    struct memory_chunk_t* mem = (struct memory_chunk_t*)((char*)memblock - CTRL_BLOCK_SIZE - FENCE_SIZE/2);
    if(mem->size == count) return memblock;

    if(count < mem->size){
        char* tmp = mem->user_mem_end;
        tmp = tmp - (mem->size - count);
        mem->user_mem_end = tmp;
        mem->size = count;
        mem->checksum = update_checksum(mem);
        for(int i = 0;i < FENCE_SIZE/2; i++){
            *((char*)mem->user_mem_end + i + 1) = '#';
        }
        return memblock;
    }
    if(!mem->next){
        void *tmp = custom_sbrk(count - mem->size);
        if(tmp == (void*)-1) return NULL;
        memoryManager.memory_size += count - mem->size;
        char* tmp2 = mem->user_mem_end;
        tmp2 = tmp2 - (mem->size - count);
        mem->user_mem_end = tmp2;
        mem->size = count;
        mem->checksum = update_checksum(mem);
        for(int i = 0;i < FENCE_SIZE/2; i++){
            *((char*)mem->user_mem_end + i + 1) = '#';
        }
        return memblock;
    }
    else{
        if(mem->next->free){
            int empty_space = (int)(((char*)mem->next) - ((char*)mem + CTRL_BLOCK_SIZE + FENCE_SIZE) + (mem->next->size - FENCE_SIZE));
            if((int)count <= empty_space){
                char* tmp = mem->user_mem_end;
                tmp = tmp + (count - (mem->size));
                mem->user_mem_end = tmp;
                int check = (int)((char*)mem->user_mem_end - ((char*)mem + CTRL_BLOCK_SIZE + FENCE_SIZE/2));
                printf("%d", check);
                memcpy((char*)mem->user_mem_end + FENCE_SIZE/2 + 1, mem->next, CTRL_BLOCK_SIZE);
                mem->next = (struct memory_chunk_t*)((char*)mem->user_mem_end + FENCE_SIZE/2 + 1);
                if(mem->next->next){
                    mem->next->next->prev = mem->next;
                    mem->next->next->checksum = update_checksum(mem->next->next);
                }
                mem->next->size = (char*)mem->next->user_mem_end - (char*)mem->next - CTRL_BLOCK_SIZE;
                mem->size = count;
                mem->checksum = update_checksum(mem);
                mem->next->checksum = update_checksum(mem->next);
                for(int i = 0; i < FENCE_SIZE/2; i++){
                    *((char*)mem->user_mem_end + i + 1) = '#';
                    *((char*)mem->next + CTRL_BLOCK_SIZE + i) = '#';
                }
                return memblock;
            }
            else if(count <= mem->size + empty_space + CTRL_BLOCK_SIZE){
                heap_free((char*)mem + CTRL_BLOCK_SIZE + FENCE_SIZE/2);
                heap_malloc(count);
                return memblock;
            }
            else{
                char* tmp = heap_malloc(count);
                if(!tmp) return NULL;
                memcpy(tmp, memblock, mem->size);
                heap_free(memblock);
                return tmp;
            }
        }
        else{
            char* tmp = heap_malloc(count);
            if(!tmp) return NULL;
            memcpy(tmp, memblock, mem->size);
            heap_free(memblock);
            return tmp;
        }
    }
    return NULL;
}
void heap_free(void* memblock){
    struct memory_chunk_t* mem = memoryManager.first_memory_chunk;
    struct memory_chunk_t* tmp = (struct memory_chunk_t*)((char*)memblock - CTRL_BLOCK_SIZE - FENCE_SIZE/2);
    if(heap_validate()) return;
//    if(tmp == mem && tmp->next == NULL){
//        memoryManager.first_memory_chunk = NULL;
//    }
    //else{
        while(mem){
            if(mem == tmp){
                tmp->free = 1;
                if(tmp->next){
                    tmp->size = (char*)tmp->next - (char*)tmp - (char)CTRL_BLOCK_SIZE;
                }
                if(tmp->prev){
                    if(tmp->prev->free){
                        tmp->prev->size += tmp->size + CTRL_BLOCK_SIZE;
                        tmp->prev->next = tmp->next;
                        tmp->prev->user_mem_end = tmp->user_mem_end;
                        if(tmp->next){
                            tmp->next->prev = tmp->prev;
                            tmp->next->checksum = update_checksum(tmp->next);
                        }
                        tmp = tmp->prev;
                    }

                }
                if(tmp->next){
                    if(tmp->next->free){
                        tmp->size += tmp->next->size + CTRL_BLOCK_SIZE;
                        tmp->user_mem_end = tmp->next->user_mem_end;
                        if(tmp->next->next){
                            tmp->next->next->prev = tmp;
                            tmp->next->next->checksum = update_checksum(tmp->next->next);
                        }
                        tmp->next = tmp->next->next;
                    }
                }
                else{
                    if(tmp->prev){
                        tmp->prev->next = NULL;
                        tmp->prev->checksum = update_checksum(tmp->prev);
                    }
                    if(memoryManager.first_memory_chunk == tmp) {
                        memoryManager.first_memory_chunk = NULL;
                    }
                    memoryManager.memory_size -= tmp->size + CTRL_BLOCK_SIZE + FENCE_SIZE;
                    custom_sbrk(-(tmp->size + CTRL_BLOCK_SIZE + FENCE_SIZE));
                    tmp = NULL;

                }
                if(tmp) {
                    tmp->user_mem_end = ((char *) tmp + tmp->size + CTRL_BLOCK_SIZE - 1);
                    tmp->checksum = update_checksum(tmp);
                }
                break;
            }
            mem = mem->next;
        }
    //}
    mem = memoryManager.first_memory_chunk;
    if(mem){
        if(mem->next == NULL && mem->free){
            memoryManager.first_memory_chunk = NULL;
        }
        else if(mem->next){
            if(mem->next->next == NULL && mem->next->free) memoryManager.first_memory_chunk = NULL;
        }
    }
}


size_t heap_get_largest_used_block_size(void){
    if (memoryManager.initialized == 0) {
        return 0;
    }
    if(heap_validate()) return 0;

    size_t largest_block_size = 0;
    struct memory_chunk_t *chunk = memoryManager.first_memory_chunk;
    while (chunk != NULL) {
        if (chunk->size > largest_block_size && !chunk->free) {
            largest_block_size = chunk->size;
        }
        chunk = chunk->next;
    }

    return largest_block_size;
}
enum pointer_type_t get_pointer_type(const void* const pointer){
    if(!pointer) return pointer_null;
    if(heap_validate()) return pointer_heap_corrupted;
    struct memory_chunk_t* tmp = memoryManager.first_memory_chunk;
    //struct memory_chunk_t* tmp2 = NULL;
    while(tmp){
        if((struct memory_chunk_t*)pointer >= tmp && (char*)pointer < ((char*)tmp + CTRL_BLOCK_SIZE)) return pointer_control_block;
        //else if((char*)pointer >= ((char*)tmp + CTRL_BLOCK_SIZE + FENCE_SIZE/2 + 1) && tmp->next && (char*)pointer < ((char*)tmp->next - FENCE_SIZE/2)) return pointer_inside_data_block;
//        if(*((char*)pointer - 1) == '#' && *((char*)pointer) != '#'){
//            tmp2 = (struct memory_chunk_t*)((char*)pointer - CTRL_BLOCK_SIZE - FENCE_SIZE/2);
//            if(tmp2->free == 0) return pointer_valid;
//        }

        if((char*)pointer >= (char*)tmp && (char*)pointer < (char*)tmp + CTRL_BLOCK_SIZE)
            return pointer_control_block;
        else if((char*)pointer >= (char*)tmp + CTRL_BLOCK_SIZE && (char*)pointer < (char*)tmp + CTRL_BLOCK_SIZE + FENCE_SIZE/2){

            return tmp->free ? pointer_unallocated:pointer_inside_fences;

        }
        else if((char*)pointer == (char*)tmp + CTRL_BLOCK_SIZE + FENCE_SIZE/2){
            return tmp->free ? pointer_unallocated:pointer_valid;
        }
        else if((char*)pointer > (char*)tmp + CTRL_BLOCK_SIZE + FENCE_SIZE/2 && (char*)pointer <= (char*)tmp->user_mem_end){
            return tmp->free ? pointer_unallocated:pointer_inside_data_block;
        }
        else if((char*)pointer >= (char*)tmp->user_mem_end + 1 && (char*)pointer < (char*)tmp->user_mem_end + 1 + FENCE_SIZE/2){
            return tmp->free ? pointer_unallocated:pointer_inside_fences;
        }
        else if(tmp->next){
            if((char*)pointer >= (char*)tmp->user_mem_end + 1 + FENCE_SIZE/2 && (char*)pointer < (char*)tmp->next) return pointer_unallocated;
        }
        tmp = tmp->next;
    }
    return pointer_unallocated;
}
