#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "malloc_implementation.h"

#define PAGE 4096 // 4 Kb
doubly_linked_list_t Heap;


int checkSum(struct node_t * node)
{
    int sum = 0;
    char* ptr = (char*)node;
    for(unsigned int i = 0; i < sizeof(node_t); i++,ptr++) sum += *ptr;
    return sum;
}
int heapSum(doubly_linked_list_t list){
    char *ptr = (char*) Heap.start_brk;
    int sum = 0;
    for(unsigned int i = 0; i < sizeof(Heap); i++, ptr++) sum += *ptr;
    return sum;
}

int heap_setup(void){
    pthread_mutexattr_init(&recursive);
    pthread_mutexattr_settype(&recursive,PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex,&recursive);
    pthread_mutex_lock(&mutex);
    
    Heap.start_brk = (intptr_t)custom_sbrk(sizeof(node_t));
    if((void*)Heap.start_brk == (void*)0){
        return pthread_mutex_unlock(&mutex),INVALID_HEAP;
    }
    
    node_t * node = (node_t *)(Heap.start_brk);
    node->next = node->prev = (void*)0;
    node->isOccupied = true;
    node->blockLength = 0;
    node->sumOfEveryBytes = 0;
    node->sumOfEveryBytes = checkSum(node);
    Heap.HeapBytes = 0;
    Heap.HeapBytes = heapSum(Heap);
    Heap.end_brk = Heap.start_brk + sizeof(node_t); // Wskaźnik na koniec pamięci start_brk    

    if(heap_validate() != SUCCESS){
        return pthread_mutex_unlock(&mutex), INVALID_HEAP;
    }
    return pthread_mutex_unlock(&mutex), SUCCESS;
}


void* heap_malloc(size_t count)
{
    if(!count){
        return NULL;
    }
    pthread_mutex_lock(&mutex);
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    
    node_t * node = (node_t *) Heap.start_brk;
    node = getBlock(node,count);
    if(node){
        intptr_t temp = Heap.end_brk - sizeof(node_t);
        node = (node_t *)temp;
        Heap.end_brk = (intptr_t) custom_sbrk(2* sizeof(fenceSize) + count + sizeof(node_t));
        
        if((void*)Heap.end_brk == (void*)0){
            return pthread_mutex_unlock(&mutex),NULL;
        }
        else if((void*)Heap.end_brk == (void*)-1){
            Heap.end_brk = (intptr_t) custom_sbrk(-(2* sizeof(fenceSize) + count + sizeof(node_t)));
            return pthread_mutex_unlock(&mutex),NULL;
        }
        
        Heap.end_brk += count + sizeof(fenceSize) * 2;
        node_t *tempNode= (node_t *)Heap.end_brk;
        tempNode->blockLength = 0;
        tempNode->isOccupied = true;
        tempNode->prev = node;
        tempNode->next = NULL;
        tempNode->sourceFile = NULL;
        tempNode->fileLine = 0;
        
        node->blockLength = count;
        node->next = tempNode;
        node->sourceFile = NULL;
        node->fileLine = 0;
        node->isOccupied = true;
        
        tempNode->sumOfEveryBytes = checkSum(tempNode);
        if(node->prev){
            tempNode = node->prev;
            tempNode->sumOfEveryBytes = 0;
            tempNode->sumOfEveryBytes = checkSum(tempNode);
        }
        tempNode = node;
        tempNode->sumOfEveryBytes = 0;
        tempNode->sumOfEveryBytes = checkSum(tempNode);
        Heap.HeapBytes = 0;
        Heap.HeapBytes = heapSum(Heap);
        copyFences(node,count);
        Heap.end_brk += sizeof(node_t);
        
        if(heap_validate() == INVALID_HEAP){
            return pthread_mutex_unlock(&mutex),NULL;
        }
        return pthread_mutex_unlock(&mutex),(void*)((intptr_t)node + sizeof(node_t) + sizeof(fenceSize));
    }
    
    if(node->blockLength >= (count + sizeof(fenceSize)*2 + sizeof(node_t))){
        intptr_t jump_address = (intptr_t)node + sizeof(fenceSize)*2 + sizeof(node_t) + count;
        node_t *jump = (node_t*)(jump_address);
        jump->next = node->next;
        node->next->prev = jump;
        jump->prev = node;
        node->next = jump;
        jump->blockLength = node->blockLength - (sizeof(node_t) + count+ 2 * sizeof(fenceSize));
        jump->isOccupied = false;
        jump->sourceFile = NULL;
        jump->fileLine = 0;
        
        node_t * tempNode = jump->next;
        tempNode->sumOfEveryBytes = 0;
        tempNode->sumOfEveryBytes = checkSum(tempNode);
            
        tempNode = jump->prev;
        tempNode->sumOfEveryBytes = 0;
        tempNode->sumOfEveryBytes = checkSum(tempNode);
        
        tempNode = jump;
        tempNode->sumOfEveryBytes = 0;
        tempNode->sumOfEveryBytes = checkSum(tempNode);

        Heap.HeapBytes = 0;
        Heap.HeapBytes = heapSum(Heap);
    }
    node->blockLength = (int)((intptr_t)node->next - ((intptr_t)node - sizeof(fenceSize) * 2 - sizeof(node_t)));
    node->isOccupied = true;
    node->sourceFile = NULL;
    node->fileLine = 0;
    copyFences(node,node->blockLength);
    node_t * tempNode = node->next;
    tempNode->sumOfEveryBytes = 0;
    tempNode->sumOfEveryBytes = checkSum(tempNode);
        
    if(node->prev){
        tempNode = node->prev;
        tempNode->sumOfEveryBytes = 0;
        tempNode->sumOfEveryBytes = checkSum(tempNode);
    }
    tempNode = node;
    tempNode->sumOfEveryBytes = 0;
    tempNode->sumOfEveryBytes = checkSum(tempNode);

    Heap.HeapBytes = 0;
    Heap.HeapBytes = heapSum(Heap);
            
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL; 
    }
    
    pthread_mutex_unlock(&mutex);
    return (void*)((intptr_t)node + sizeof(node_t) + sizeof(fenceSize));
}
void* heap_calloc(size_t number, size_t size)
{
    pthread_mutex_lock(&mutex);
    if(!number || !size || heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    void * ptr = heap_malloc(number * size);
    if(heap_validate() == INVALID_HEAP || ptr == (void*)0){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    memset(ptr,0,number*size);
    return pthread_mutex_unlock(&mutex),ptr;
}
void* heap_realloc(void* memblock, size_t size)
{
    pthread_mutex_lock(&mutex);
    
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    if(memblock == NULL){
        return pthread_mutex_unlock(&mutex),heap_malloc(size);
    }
    else if(!size){
        heap_free(memblock);
        return pthread_mutex_unlock(&mutex),NULL;
    }
    node_t *node = (node_t *)((intptr_t)memblock - sizeof(fenceSize) - sizeof(node_t));
    int newSize = size;
    if(node->blockLength <= size){
        newSize = node->blockLength;
    }
    void *ptr = heap_malloc(size);
    if(ptr == (void*)0){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    else{
        memcpy(ptr,memblock,newSize);
    }
    heap_free(memblock);
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    
    return pthread_mutex_unlock(&mutex),ptr;
}
void heap_free(void* memblock)
{
    pthread_mutex_lock(&mutex);
    if(get_pointer_type(memblock) != pointer_valid || heap_validate() == INVALID_HEAP){
        pthread_mutex_unlock(&mutex);
        return;
    }
    node_t *node = (node_t *)((intptr_t)memblock - sizeof(fenceSize) - sizeof(node_t));
    node->isOccupied = false;
    node = coalescing(node);
    
    node->blockLength = ((intptr_t)(node->next) - (intptr_t)(node) - sizeof(node_t));
    node_t * tempNode = node->next;
    tempNode->sumOfEveryBytes = 0;
    tempNode->sumOfEveryBytes = checkSum(tempNode);
        
    if(node->prev){            
        tempNode = node->prev;
        tempNode->sumOfEveryBytes = 0;
        tempNode->sumOfEveryBytes = checkSum(tempNode);
    }

    node->sumOfEveryBytes = 0;
    node->sumOfEveryBytes = checkSum(node);
    Heap.HeapBytes = 0;
    Heap.HeapBytes = heapSum(Heap);
    
    pthread_mutex_unlock(&mutex);
}
node_t* coalescing(node_t *node)
{
    if(node->next){
        if(node->next->isOccupied == false){
            node->next = node->next->next;
            node->next->prev = node;
        }
    }
    else if(node->prev){
        if(node->prev->isOccupied == false){
            node->prev->next = node->next;
            node->next->prev = node->prev;
            node = node->prev;
        }
    }
    return node;
}
void copyFences(node_t * node,size_t count)
{
    memcpy((void*)((intptr_t)(node)+sizeof(node_t)),fenceSize,sizeof(fenceSize));
    memcpy((void*)((intptr_t)(node)+sizeof(node_t)+ sizeof(fenceSize)+count),fenceSize,sizeof(fenceSize));
}
void* getBlock(node_t *node, size_t count)
{
    for(;node;node=node->next){
        if(node->isOccupied == false){
            if(node->blockLength == (count + sizeof(fenceSize) * 2)){
                
                return node;
            }
            else if(node->blockLength >= (count + sizeof(fenceSize) * 2 + sizeof(node_t))){
                
                return node;
            }
        }
    }
    return NULL;
}
void* heap_malloc_debug(size_t count, int fileline, const char* filename)
{
    if(!count){
        return NULL;
    }
    
    pthread_mutex_lock(&mutex);
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
        
    node_t * node = (node_t *) Heap.start_brk;
    node = getBlock(node,count);
    if(node == NULL){
        intptr_t temp = Heap.end_brk - sizeof(node_t);
        node = (node_t *)temp;
        Heap.end_brk = (intptr_t) custom_sbrk(2* sizeof(fenceSize) + count + sizeof(node_t));
        if((void*)Heap.end_brk == (void*)0){
            return pthread_mutex_unlock(&mutex),NULL;
        }
        else if((void*)Heap.end_brk == (void*)-1){
            Heap.end_brk = (intptr_t) custom_sbrk(-(2* sizeof(fenceSize) + count + sizeof(node_t)));
            return pthread_mutex_unlock(&mutex),NULL;
        }
        Heap.end_brk += count + sizeof(fenceSize) * 2;
        
        node_t * tempNode = (node_t *)Heap.end_brk;
        tempNode->blockLength = 0;
        tempNode->isOccupied = true;
        tempNode->prev = node;
        tempNode->next = NULL;
        tempNode->sourceFile = (char*)filename;
        tempNode->fileLine = fileline;
        
        node->blockLength = count;
        node->next = tempNode;
        node->sourceFile = (char*)filename;
        node->fileLine = fileline;
        node->isOccupied = true;
        
        tempNode->sumOfEveryBytes = 0;
        tempNode->sumOfEveryBytes = checkSum(tempNode);
            
        if(node->prev){
            tempNode = node->prev;
            tempNode->sumOfEveryBytes = 0;
            tempNode->sumOfEveryBytes = checkSum(tempNode);
        }
        tempNode = node;
        tempNode->sumOfEveryBytes = 0;
        tempNode->sumOfEveryBytes = checkSum(tempNode);

        Heap.HeapBytes = 0;
        Heap.HeapBytes = heapSum(Heap);
        copyFences(node,count);
        Heap.end_brk += sizeof(node_t);
        if(heap_validate() == INVALID_HEAP){
            return pthread_mutex_unlock(&mutex),NULL;
        }
        return pthread_mutex_unlock(&mutex),(void*)((intptr_t)node + sizeof(node_t) + sizeof(fenceSize));
    }
    if(node->blockLength >= (count + sizeof(fenceSize)*2 + sizeof(node_t))){
        intptr_t jump_address = (intptr_t)node + sizeof(fenceSize)*2 + sizeof(node_t) + count;
        node_t *jump = (node_t*)(jump_address);
        jump->next = node->next;
        node->next->prev = jump;
        jump->prev = node;
        node->next = jump;
        jump->blockLength = node->blockLength - (sizeof(node_t) + count+ 2 * sizeof(fenceSize));
        jump->isOccupied = false;
        jump->sourceFile = (char*)filename;
        jump->fileLine = fileline;
        
        node_t * tempNode = jump->next;
        tempNode->sumOfEveryBytes = 0;
        tempNode->sumOfEveryBytes = checkSum(tempNode);
            
        tempNode = jump->prev;
        tempNode->sumOfEveryBytes = 0;
        tempNode->sumOfEveryBytes = checkSum(tempNode);
        
        tempNode = jump;
        tempNode->sumOfEveryBytes = 0;
        tempNode->sumOfEveryBytes = checkSum(tempNode);
        
        Heap.HeapBytes = 0;
        Heap.HeapBytes = heapSum(Heap);
    }
    node->blockLength = (int)((intptr_t)node->next - ((intptr_t)node - sizeof(fenceSize) * 2 - sizeof(node_t)));
    node->isOccupied = true;
    node->sourceFile = (char*)(filename);
    node->fileLine = fileline;
    copyFences(node,node->blockLength);
    node_t *tempNode = node->next;
    tempNode->sumOfEveryBytes = 0;
    tempNode->sumOfEveryBytes = checkSum(tempNode);
        
    if(node->prev){
        tempNode = node->prev;
        tempNode->sumOfEveryBytes = 0;
        tempNode->sumOfEveryBytes = checkSum(tempNode);
    }
    tempNode = node;
    tempNode->sumOfEveryBytes = 0;
    tempNode->sumOfEveryBytes = checkSum(tempNode);
    Heap.HeapBytes = 0;
    Heap.HeapBytes = heapSum(Heap);
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL; 
    }
    
    pthread_mutex_unlock(&mutex);
    return (void*)((intptr_t)node + sizeof(node_t) + sizeof(fenceSize));
}
void* heap_calloc_debug(size_t number, size_t size, int fileline,const char* filename)
{
    pthread_mutex_lock(&mutex);
    if(!number || !size || heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    void * ptr = heap_malloc_debug(number * size,fileline,filename);
    if(heap_validate() == INVALID_HEAP || ptr == (void*)0){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    memset(ptr,0,number*size);
    return pthread_mutex_unlock(&mutex),ptr;
}
void* heap_realloc_debug(void* memblock, size_t size, int fileline,const char* filename)
{
    pthread_mutex_lock(&mutex);
    
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    if(memblock == NULL){
        return pthread_mutex_unlock(&mutex),heap_malloc_debug(size,fileline,filename);
    }
    else if(!size){
        heap_free(memblock);
        return pthread_mutex_unlock(&mutex),NULL;
    }
    node_t *node = (node_t *)((intptr_t)memblock - sizeof(fenceSize) - sizeof(node_t));
    int newSize = size;
    if(node->blockLength <= size){
        newSize = node->blockLength;
    }
    
    void *ptr = heap_malloc_debug(size,fileline,filename);
    if(ptr == (void*)0){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    else{
        memcpy(ptr,memblock,newSize);
    }
    
    heap_free(memblock);
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    
    return pthread_mutex_unlock(&mutex),ptr;
}

void* heap_malloc_aligned(size_t count)
{
    if(count == 0)
        return NULL;
    pthread_mutex_lock(&mutex);
    
    if(heap_validate() == INVALID_HEAP)
        return pthread_mutex_unlock(&mutex),NULL;
        
    node_t *node = (node_t *)Heap.start_brk;
    node = getAlignedBlock(node,count);
    
    if(node == NULL){
        if(Heap.end_brk == (Heap.start_brk + sizeof(node_t))){ // Dodaję tutaj dodatkowy blok struktury. Robię to ze względu na
        // wyrównanie. Struktury będą zamieniały miejsce z wyrównaniem, a tak to mam pewność że head nie zamienia się
            Heap.end_brk = (intptr_t) custom_sbrk(sizeof(node_t));
            node_t * tempNode = (node_t *)Heap.start_brk, *tempEndNode = (node_t*)Heap.end_brk;
            tempNode->next = tempEndNode;
            tempEndNode->prev = tempNode;
            tempNode->prev = tempEndNode->next = NULL;
            Heap.end_brk += sizeof(node_t);
        }
        
        node = (node_t *)(Heap.end_brk - sizeof(node_t));
        size_t size = PAGE - ((getDistnace(Heap)) % PAGE);
        
        Heap.end_brk = (intptr_t) custom_sbrk(count + size + sizeof(node_t) + sizeof(fenceSize));
        if((void*)Heap.end_brk == (void*)0)
            return pthread_mutex_unlock(&mutex),NULL;
        else if((void*)Heap.end_brk == (void*)-1){
            Heap.end_brk = (intptr_t) custom_sbrk(-(count + size + sizeof(struct node_t) + sizeof(fenceSize)));
            return pthread_mutex_unlock(&mutex),NULL;
        }
        
        node_t * tempNode = NULL;
        if(node->prev)
            tempNode = node->prev;
            
        node = (node_t*)((intptr_t)node + size - sizeof(fenceSize));
        Heap.end_brk += count + size + sizeof(fenceSize);
        node_t * tempEndNode = ((node_t *)(Heap.end_brk));
        tempEndNode->next = NULL;
        tempEndNode->prev = node;
        tempEndNode->fileLine = 0;
        tempEndNode->sourceFile = NULL;
        tempEndNode->blockLength = 0;
        tempEndNode->isOccupied = true;
        tempEndNode->sumOfEveryBytes = 0;
        tempEndNode->sumOfEveryBytes = checkSum(tempEndNode);

        if(tempEndNode->prev){
            tempEndNode = tempEndNode->prev;
            tempEndNode-> sumOfEveryBytes = 0;
            tempEndNode->sumOfEveryBytes = checkSum(tempEndNode);
        }

        node->isOccupied = true;
        node->blockLength = count;
        node->fileLine = 0;
        node->sourceFile = NULL;
        node->prev = tempNode;
        node->next = (node_t *)Heap.end_brk;
        
        if(tempNode){
            tempNode->next = node;
        }
        tempEndNode = (node_t *)node->next;
        tempEndNode->sumOfEveryBytes = 0;
        tempEndNode->sumOfEveryBytes = checkSum(tempEndNode);
        
        tempEndNode = (node_t *)node;
        tempEndNode->sumOfEveryBytes = 0;
        tempEndNode->sumOfEveryBytes = checkSum(tempEndNode);
        
        if(tempEndNode->prev){
            tempEndNode = tempEndNode->prev;
            tempEndNode->sumOfEveryBytes = 0;
            tempEndNode->sumOfEveryBytes = checkSum(tempEndNode);
        }

        Heap.HeapBytes = 0;
        Heap.HeapBytes = heapSum(Heap);
        copyFences(node,count);
        Heap.end_brk += sizeof(node_t);
        
        if(heap_validate() == INVALID_HEAP){
            return pthread_mutex_unlock(&mutex),NULL;
        }
        
        return pthread_mutex_unlock(&mutex),(void*)((intptr_t)node + sizeof(node_t) + sizeof(fenceSize));
    }
    if(node->blockLength >= (count + sizeof(node_t) + sizeof(fenceSize) * 2)){
        intptr_t jump_address = (intptr_t)node + sizeof(fenceSize)*2 + sizeof(node_t) + count;
        node_t *jump = (node_t*)(jump_address);
        jump->next =node->next;
        jump->prev =node;
        node->next = (node_t*)jump_address;
        node->next->prev = (node_t*)jump_address;
        jump->blockLength = node->blockLength - (int)(sizeof(node_t) + count+ 2 * sizeof(fenceSize));
        jump->isOccupied = false;
        jump->sourceFile = NULL;
        jump->fileLine = 0;
        
        node_t * temp = jump->next;
        temp->sumOfEveryBytes = 0;
        temp->sumOfEveryBytes = checkSum(temp);
    }
    node->blockLength = (int)((intptr_t)node->next - ((intptr_t)node - sizeof(fenceSize) * 2 - sizeof(node_t)));
    node->isOccupied = true;
    node->sourceFile = NULL;
    node->fileLine = 0;
    copyFences(node,node->blockLength);
    
    node_t * temp = node->next;
    temp->sumOfEveryBytes = 0;
    temp->sumOfEveryBytes = checkSum(temp);
    
    if(node->prev){
        temp = node->prev;
        temp->sumOfEveryBytes = 0;
        temp->sumOfEveryBytes = checkSum(temp);
    }

    temp = node;
    temp->sumOfEveryBytes = 0;
    temp->sumOfEveryBytes = checkSum(temp);

    Heap.HeapBytes = 0;
    Heap.HeapBytes = heapSum(Heap);
            
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL; 
    }
    pthread_mutex_unlock(&mutex);
    
    return (void*)((intptr_t)node + sizeof(node_t) + sizeof(fenceSize));
}
void* heap_calloc_aligned(size_t number, size_t size)
{
    if(!number || !size){
        return NULL;
    }
    pthread_mutex_lock(&mutex);
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    
    void *ptr = heap_malloc_aligned(number * size);
    if(ptr == NULL){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    
    memset(ptr,0,size*number);
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    
    pthread_mutex_unlock(&mutex);
    return ptr;
}
void* heap_realloc_aligned(void* memblock, size_t size)
{
    pthread_mutex_lock(&mutex);
    if(!memblock){
        return heap_malloc_aligned(size);
    }
    else if(!size){
        return heap_free(memblock),NULL;
    }
    
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    
    node_t * node = (node_t *)((intptr_t)memblock - sizeof(node_t) - sizeof(fenceSize));
    size_t newSize = node->blockLength;
    if(node->blockLength > size){
            newSize = size;
    }
    heap_free(memblock);
    void *ptr = heap_malloc_aligned(size);
    if(ptr){
        memcpy(ptr,memblock,newSize);
    }
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    
    pthread_mutex_unlock(&mutex);
    return ptr;
}
void* getAlignedBlock(node_t *node, size_t count)
{
    for(;node; node = node->next){
        if(((node->blockLength == (count + sizeof(fenceSize)*2) || node->blockLength >= (count + sizeof(fenceSize) * 2 + sizeof(node_t)))&& 
            node->isOccupied == false)){
                if((((intptr_t)node + sizeof(node_t) + sizeof(fenceSize)) % PAGE == 0)) return node;
            }
    }
    return NULL;
}
void* heap_malloc_aligned_debug(size_t count, int fileline,const char* filename)
{
    if(count == 0){
        return NULL;
    }
    pthread_mutex_lock(&mutex);
    
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    
    node_t *node = (node_t *)Heap.start_brk;
    node = getAlignedBlock(node,count);
    if(node == NULL){
        if(Heap.end_brk == (Heap.start_brk + sizeof(node_t))){ // Dodaję tutaj dodatkowy blok struktury. Robię to ze względu na
        // wyrównanie. Struktury będą zamieniały miejsce z wyrównaniem, a tak to mam pewność że head nie zamienia się
            Heap.end_brk = (intptr_t) custom_sbrk(sizeof(node_t));
            node_t * firstNode = (node_t *)Heap.start_brk, *endNode = (node_t*)Heap.end_brk;
            firstNode->next = endNode;
            endNode->prev = firstNode;
            firstNode->prev = endNode->next = NULL;
            Heap.end_brk += sizeof(node_t);
        }
        
        node = (node_t *)(Heap.end_brk - sizeof(node_t));
        size_t size = PAGE - ((getDistnace(Heap)) % PAGE);
        
        Heap.end_brk = (intptr_t) custom_sbrk(count + size + sizeof(node_t) + sizeof(fenceSize));
        if((void*)Heap.end_brk == (void*)0){
            return pthread_mutex_unlock(&mutex),NULL;
        }
        else if((void*)Heap.end_brk == (void*)-1){
            Heap.end_brk = (intptr_t) custom_sbrk(-(count + size + sizeof(struct node_t) + sizeof(fenceSize)));
            return pthread_mutex_unlock(&mutex),NULL;
        }
        
        node_t * tempNode = NULL;
        if(node->prev){
            tempNode = node->prev;
        }
        
        node = (node_t*)((intptr_t)node + size - sizeof(fenceSize));
        Heap.end_brk += count + size + sizeof(fenceSize);
        node_t * endNode = ((node_t *)(Heap.end_brk));
        endNode->next = NULL;
        endNode->prev = node;
        endNode->fileLine = fileline;
        endNode->sourceFile = (char*) filename;
        endNode->blockLength = 0;
        endNode->isOccupied = true;
        endNode->sumOfEveryBytes = 0;
        endNode->sumOfEveryBytes = checkSum(endNode);

        if(endNode->prev){
            endNode = endNode->prev;
            endNode-> sumOfEveryBytes = 0;
            endNode->sumOfEveryBytes = checkSum(endNode);
        }

        node->isOccupied = true;
        node->blockLength = count;
        node->fileLine = fileline;
        node->sourceFile = (char*)filename;
        node->prev = tempNode;
        node->next = (node_t *)Heap.end_brk;
        if(tempNode)
            tempNode->next = node;
         
        endNode = (node_t *)node->next;
        endNode->sumOfEveryBytes = 0;
        endNode->sumOfEveryBytes = checkSum(endNode);
        
        endNode = (node_t *)node;
        endNode->sumOfEveryBytes = 0;
        endNode->sumOfEveryBytes = checkSum(endNode);
        
        if(endNode->prev){
            endNode = endNode->prev;
            endNode->sumOfEveryBytes = 0;
            endNode->sumOfEveryBytes = checkSum(endNode);
        }

        Heap.HeapBytes = 0;
        Heap.HeapBytes = heapSum(Heap);
        copyFences(node,count);
        Heap.end_brk += sizeof(node_t);
        
        if(heap_validate() == INVALID_HEAP){
            return pthread_mutex_unlock(&mutex),NULL;
        }
        return pthread_mutex_unlock(&mutex),(void*)((intptr_t)node + sizeof(node_t) + sizeof(fenceSize));
    }
    if(node->blockLength >= (count + sizeof(node_t) + sizeof(fenceSize) * 2)){
        intptr_t jump_address = (intptr_t)node + sizeof(fenceSize)*2 + sizeof(node_t) + count;
        node_t *jump = (node_t*)(jump_address);
        jump->next =node->next;
        jump->prev =node;
        node->next = (node_t*)jump_address;
        node->next->prev = (node_t*)jump_address;
        jump->blockLength = node->blockLength - (int)(sizeof(node_t) + count+ 2 * sizeof(fenceSize));
        jump->isOccupied = false;
        jump->sourceFile = (char*)filename;
        jump->fileLine = fileline;
        node_t * next = jump->next;
        next->sumOfEveryBytes = 0;
        next->sumOfEveryBytes = checkSum(next);
    }
    node->blockLength = (int)((intptr_t)node->next - ((intptr_t)node - sizeof(fenceSize) * 2 - sizeof(node_t)));
    node->isOccupied = true;
    node->sourceFile = (char*)(filename);
    node->fileLine = fileline;
    copyFences(node,node->blockLength);
    
    node_t * tempNode = node->next;
    tempNode->sumOfEveryBytes = 0;
    tempNode->sumOfEveryBytes = checkSum(tempNode);
    
    if(node->prev){
        tempNode = node->prev;
        tempNode->sumOfEveryBytes = 0;
        tempNode->sumOfEveryBytes = checkSum(tempNode);
    }

    tempNode = node;
    tempNode->sumOfEveryBytes = 0;
    tempNode->sumOfEveryBytes = checkSum(tempNode);
    Heap.HeapBytes = 0;
    Heap.HeapBytes = heapSum(Heap);
            
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL; 
    }
    
    pthread_mutex_unlock(&mutex);
    return (void*)((intptr_t)node + sizeof(node_t) + sizeof(fenceSize));
}
void* heap_calloc_aligned_debug(size_t number, size_t size, int fileline,const char* filename)
{
    if(!number || !size){
        return NULL;
    }
    pthread_mutex_lock(&mutex);
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    
    void *ptr = heap_malloc_aligned_debug(number * size,fileline,filename);
    if(ptr == NULL){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    memset(ptr,0,size*number);
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    pthread_mutex_unlock(&mutex);
    return ptr;
}
void* heap_realloc_aligned_debug(void* memblock, size_t size, int fileline,const char* filename)
{
    if(!memblock){
        return heap_malloc_aligned_debug(size,fileline,filename);
    }
    else if(!size){
        return heap_free(memblock),NULL;
    }
    
    pthread_mutex_lock(&mutex);    
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    node_t * node = (node_t *)((intptr_t)memblock - sizeof(node_t) - sizeof(fenceSize));
    size_t newSize = node->blockLength;
    if(node->blockLength > size){
            newSize = size;
    }
    heap_free(memblock);
    void *ptr = heap_malloc_aligned_debug(size,fileline,filename);
    if(ptr){
        memcpy(ptr,memblock,newSize);
    }
    if(heap_validate() == INVALID_HEAP){
        return pthread_mutex_unlock(&mutex),NULL;
    }
    
    pthread_mutex_unlock(&mutex);
    return ptr;
}

size_t heap_get_used_space(void)
{
    pthread_mutex_lock(&mutex);
    size_t usedSpace;
    struct node_t * node = (struct node_t *)Heap.start_brk;
    for(usedSpace = 0;node; node = node->next){
        if(node->blockLength > 0 && node->isOccupied == true){
            usedSpace = usedSpace + node->blockLength + sizeof(fenceSize) * 2 ;
        }
        usedSpace = usedSpace + sizeof(struct node_t);
    }
    pthread_mutex_unlock(&mutex);
    return usedSpace;
}
size_t heap_get_largest_used_block_size(void)
{
    pthread_mutex_lock(&mutex);
    
    size_t largestUsedBlock = 0;
    node_t * node;
    for(node = (node_t *)Heap.start_brk ;node; node = node->next){
            if(node->isOccupied == true){
                if(node->blockLength > largestUsedBlock){
                    largestUsedBlock = (size_t)node->blockLength;
                }
            }
    }
    pthread_mutex_unlock(&mutex);
    return largestUsedBlock;
}
uint64_t heap_get_used_blocks_count(void)
{
    pthread_mutex_lock(&mutex);
    
    uint64_t usedBlocksCount = 0;
    node_t * node;
    for(node = (node_t *)Heap.start_brk; node; node = node->next)
        if(node->isOccupied == true && node->blockLength > 0) usedBlocksCount++;
        
    pthread_mutex_unlock(&mutex);
    return usedBlocksCount;
}
size_t heap_get_free_space(void)
{
    pthread_mutex_lock(&mutex);
    
    size_t freeSpaceCount = 0;
    node_t * node;
    for(node = (node_t *)Heap.start_brk; node; node = node->next)
        if(node->isOccupied == false || !node->blockLength) freeSpaceCount+= node->blockLength;
        
    pthread_mutex_unlock(&mutex);
    return freeSpaceCount;
}
size_t heap_get_largest_free_area(void)
{
    pthread_mutex_lock(&mutex);
    
    size_t largestFreeBlock = 0, max = 0;
    node_t * node;
    for(node = (node_t *)Heap.start_brk ;node; node = node->next){
        if(node->blockLength > max && node->isOccupied == false){
            max = node->blockLength;
            largestFreeBlock = max;
        }
    }
    
    pthread_mutex_unlock(&mutex);
    return largestFreeBlock;
}
uint64_t heap_get_free_gaps_count(void)
{
    pthread_mutex_lock(&mutex);
    
    uint64_t freeGapsCount = 0;
    node_t * node;
    for(node = (node_t *)Heap.start_brk; node; node = node->next)
        if(node->isOccupied == false && node->blockLength >= (sizeof(void *) + sizeof(fenceSize)*2)) freeGapsCount++;
        
    pthread_mutex_unlock(&mutex);
    return freeGapsCount;
}

enum pointer_type_t get_pointer_type(const void* pointer)
{

    pthread_mutex_lock(&mutex);
    
    if(pointer == (void*)0) 
        return pthread_mutex_unlock(&mutex),pointer_null; // PRZEKAZANY WSKAŹNIK JEST PUSTY
    
    if((intptr_t)(pointer) < Heap.start_brk || (intptr_t)(pointer) > Heap.end_brk) 
        return pthread_mutex_unlock(&mutex),pointer_out_of_heap; // PRZEKASANY WSKAŹNIK LEŻY POZA STERTĄ
    
    if((intptr_t)pointer >= (Heap.end_brk - sizeof(node_t)) && (intptr_t)pointer < Heap.end_brk )
        return pthread_mutex_unlock(&mutex),pointer_control_block;
    
    node_t * node = (node_t *)Heap.start_brk;
    for(node = node->next;node; node = node->next){
        if((intptr_t)pointer < (intptr_t)node){
            node = node->prev;
            switch(node->isOccupied){
                case false:
                    if((intptr_t)pointer == ((intptr_t)node + sizeof(node_t)))
                        return pthread_mutex_unlock(&mutex), pointer_valid;// wskaźnik wskazuje na początek zaalokowanego bloku
                    else if((intptr_t)pointer >= (intptr_t)node && (intptr_t)pointer < ((intptr_t)node + sizeof(node_t)))
                        return pthread_mutex_unlock(&mutex), pointer_control_block;// wskaźnik wskazuje na obszar struktur zewnętrznych
                    else if((intptr_t)pointer >= (intptr_t)node+ node->blockLength + sizeof(node_t) + sizeof(fenceSize) &&
                    (intptr_t)pointer < (intptr_t)node->next )return pthread_mutex_unlock(&mutex), pointer_control_block;
                    break;
                    
                case true:
                    if((intptr_t)pointer > ((intptr_t)node + sizeof(node_t) + node->blockLength + sizeof(fenceSize)) 
                    && (intptr_t)pointer < ((intptr_t)node + sizeof(node_t) + sizeof(fenceSize) + node->blockLength ))
                        return pthread_mutex_unlock(&mutex), pointer_inside_data_block;// Wskaźnik wskazuje na obszar zaalakowany ale nie na jego początek
                    else if((intptr_t)pointer == ((intptr_t)node + sizeof(node_t) + sizeof(fenceSize)))
                        return pthread_mutex_unlock(&mutex), pointer_valid;// wskaźnik wskazuje na początek zaalokowanego bloku
                    else if((intptr_t)pointer >= (intptr_t)node &&  (intptr_t)pointer < ((intptr_t)node + sizeof(node_t) + sizeof(fenceSize)))
                        return pthread_mutex_unlock(&mutex), pointer_control_block;
                    break;
            }
            node = node->next;
        }
    }
    
        
    pthread_mutex_unlock(&mutex);
    return pointer_unallocated;
}

void* heap_get_data_block_start(const void* pointer)
{
    pthread_mutex_lock(&mutex);
    
    enum pointer_type_t pointerType = get_pointer_type(pointer);
    if(pointerType != pointer_inside_data_block && pointerType != pointer_valid)
        return pthread_mutex_unlock(&mutex),NULL;
    if(pointerType == pointer_valid)
        return pthread_mutex_unlock(&mutex),(void*)pointer;
        
    node_t * node;
    for(node = (node_t *)Heap.start_brk; node->next ; node = node->next){
        
        if((intptr_t)pointer >= (intptr_t)node && (intptr_t)pointer < (intptr_t)node->next)
            return pthread_mutex_unlock(&mutex),(void*)((intptr_t)node + sizeof(node_t) + sizeof(fenceSize));
    }
        
    pthread_mutex_unlock(&mutex);
    return NULL;
}

size_t heap_get_block_size(const void* memblock)
{
    pthread_mutex_lock(&mutex);
    
    if(!memblock || get_pointer_type(memblock) != pointer_valid)
        return pthread_mutex_unlock(&mutex),0;
    
    node_t * node = (node_t *)((intptr_t)memblock - sizeof(fenceSize) - sizeof(node_t));

    pthread_mutex_unlock(&mutex);
    return node->blockLength;
    
}
int heap_validate(void)
{
    pthread_mutex_lock(&mutex);
    
    if((void*)0 == (void*)Heap.start_brk || (void*)0 == (void*)Heap.end_brk)
        return pthread_mutex_unlock(&mutex), INVALID_HEAP; // -1   
        
    node_t * node;
    for(node = (node_t *)Heap.start_brk; node; node = node->next){
        if(node->isOccupied == true && node->blockLength >0){
            if(checkFences(node))
                return pthread_mutex_unlock(&mutex),INVALID_HEAP;
        }
        size_t oldControlSum,newControlSum = 0;
        newControlSum = heapSum(Heap);
        oldControlSum = Heap.HeapBytes;
        Heap.HeapBytes = newControlSum;
        if(oldControlSum != newControlSum){
            return pthread_mutex_unlock(&mutex),INVALID_HEAP;
        }
        
        int sumofEvery = node->sumOfEveryBytes;
        node->sumOfEveryBytes = 0;
        node->sumOfEveryBytes = checkSum(node);

        if(node->sumOfEveryBytes != sumofEvery){
            return pthread_mutex_unlock(&mutex),INVALID_HEAP;
        }
        else if((get_pointer_type(node->next) == pointer_out_of_heap && ((intptr_t)node + sizeof(node_t))!= Heap.end_brk)){
            return pthread_mutex_unlock(&mutex),INVALID_HEAP;
        }
        else if(get_pointer_type(node->prev) == pointer_out_of_heap){
            return pthread_mutex_unlock(&mutex),INVALID_HEAP;
        }
        
        Heap.HeapBytes = 0;
        Heap.HeapBytes = heapSum(Heap);
    }
    pthread_mutex_unlock(&mutex);
    return SUCCESS;
}
int checkFences(node_t *node)
{
    return memcmp(fenceSize,(void*)((intptr_t)node + sizeof(node_t)),sizeof(fenceSize))|| 
    memcmp(fenceSize,(void*)((intptr_t)node + sizeof(node_t) + node->blockLength + sizeof(fenceSize)),sizeof(fenceSize));
}
void heap_dump_debug_information(void)
{
    pthread_mutex_lock(&mutex);
    if(heap_validate() == INVALID_HEAP){
        printf("\n HEAP IS DAMAGED \n");
    }
    else{
        printf("\n$LIST OF MEMORY BLOCKS\n");
        node_t * node;
        for(node = (node_t *)Heap.start_brk; node ; node = node->next){
            printf("\nBlock Adress: %ld\n",(intptr_t)node);
            (node->isOccupied == true && node->blockLength > 0) ? printf("Allocated\n") : printf("Free\n"); 
            printf("Block Length in %ld bytes\n",node->blockLength);
            if(node->sourceFile)
                printf("Source File Name: %s\n",node->sourceFile);
            if(node->fileLine > 0)
                printf("Source File Line:%d\n",node->fileLine);
        }
        printf("\n$HEAP SIZE IN %zu BYTES\n",getDistnace(Heap));
        printf("$NUMBER OF USED SPACE IN BYTES %zu\n",heap_get_used_space());
        printf("$NUMBER OF FREE SPACE IN BYTES %zu\n",heap_get_free_space());
        printf("$HEAP LARGEST FREE SPACE: %zu\n",heap_get_largest_free_area());
    }
    pthread_mutex_unlock(&mutex);
}
intptr_t getDistnace(doubly_linked_list_t list)
{
    return list.end_brk - list.start_brk;
}
int destroyHeap()
{
    int difference = getDistnace(Heap);
    Heap.end_brk = (intptr_t)custom_sbrk(-difference);
    Heap.end_brk -= (intptr_t)difference;
    if((void*)Heap.end_brk == NULL || (void*)Heap.end_brk == (void*)-1)
        return INVALID_HEAP;
    return SUCCESS;
}