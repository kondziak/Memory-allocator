#ifndef MALLOC_IMPLEMENTATION_H
#define MALLOC_IMPLEMENTATION_H

#include "custom_unistd.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

pthread_mutex_t mutex;
pthread_mutexattr_t recursive;
typedef struct doubly_linked_list_t doubly_linked_list_t;
typedef struct node_t node_t;

enum pointer_type_t{
    pointer_null,
    pointer_out_of_heap,
    pointer_control_block,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};
// blockLength - długość bloku w bajtach
// fileLine - nr. linii pliku źródłowego, w którym nastąpiła alokacja
// sourceFile - nazwa pliku źródłowego, w którym nastąpiła alokacja
// isOcuupied - Czy fragment(chunk) jest wolny czy zajęty

struct node_t{
    struct node_t *next,*prev;
    size_t blockLength;
    int fileLine;
    size_t sumOfEveryBytes;
    bool isOccupied;
    char* sourceFile;
};

// start_brk,end_brk - wskaźnik na początek sterty i wskaźnik na końcowe miejsce na stercie(head i tail)

struct doubly_linked_list_t{
    size_t HeapBytes;
    intptr_t start_brk,end_brk;
};

enum error_t{
    INVALID_HEAP = -1,
    SUCCESS = 0
};

extern doubly_linked_list_t Heap;
//Płoteczki Aaaaaaa
long long int fenceSize[8];

int heap_setup(void);
void* heap_malloc(size_t count);
void* heap_calloc(size_t number, size_t size);
void  heap_free(void* memblock);
void* heap_realloc(void* memblock, size_t size);

void* heap_malloc_debug(size_t count, int fileline, const char* filename);
void* heap_calloc_debug(size_t number, size_t size, int fileline,const char* filename);
void* heap_realloc_debug(void* memblock, size_t size, int fileline,const char* filename);

void* heap_malloc_aligned(size_t count);
void* heap_calloc_aligned(size_t number, size_t size);
void* heap_realloc_aligned(void* memblock, size_t size);

void* heap_malloc_aligned_debug(size_t count, int fileline,
const char* filename);
void* heap_calloc_aligned_debug(size_t number, size_t size, int fileline,
const char* filename);
void* heap_realloc_aligned_debug(void* memblock, size_t size, int fileline,
const char* filename);

size_t heap_get_used_space(void);
size_t heap_get_largest_used_block_size(void);
uint64_t heap_get_used_blocks_count(void);
size_t heap_get_free_space(void);
size_t heap_get_largest_free_area(void);
uint64_t heap_get_free_gaps_count(void);

enum pointer_type_t get_pointer_type(const void* pointer);

void* heap_get_data_block_start(const void* pointer);
size_t heap_get_block_size(const void* memblock);
int heap_validate(void);
void heap_dump_debug_information(void);

int checkSum(struct node_t * node);
int heapSum(doubly_linked_list_t list);
void copyFences(struct node_t * node,size_t count);
void* getAlignedBlock(struct node_t *node, size_t count);
void* getBlock(struct node_t *node, size_t value);
intptr_t getDistnace(doubly_linked_list_t list);
int checkFences(node_t *node);
int destroyHeap();

node_t* coalescing(node_t *node);
#endif