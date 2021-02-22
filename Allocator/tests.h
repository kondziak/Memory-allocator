#ifndef TESTS_H
#define TESTS_H

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "malloc_implementation.h"
#include "custom_unistd.h"

#define FOO_SIZE 10
#define heap_malloc(count) heap_malloc_debug(count,__LINE__,__FILE__)
#define heap_calloc(count,size) heap_calloc_debug(count,size,__LINE__,__FILE__)
#define heap_realloc(memblock,size) heap_realloc_debug(memblock,size,__LINE__,__FILE__)
#define heap_malloc_aligned(count) heap_malloc_aligned_debug(count,__LINE__,__FILE__)
#define heap_calloc_aligned(number,size) heap_calloc_aligned_debug(number,size,__LINE__,__FILE__)
#define heap_realloc_aligned(memblock,size) heap_realloc_aligned_debug(memblock,size,__LINE__,__FILE__)

pthread_mutex_t m;

void * foo(void *arg)
{
        int *p = (int *) heap_malloc(sizeof(int));
        assert(p != (void*)0);
        *p = 9;
        printf("%d\n",*p);
        heap_free(p);
    return NULL;
}
void *foo2(void *arg)
{
        float *f = (float*) heap_malloc(sizeof(int)*FOO_SIZE);
        *f = 5.3;
        printf("%f\n",*f);
        assert(f != (void*)0);
        heap_free(f);
        return NULL;
}

void mallocTest(){
    //multiple malloc allocations on different types of variables
    printf("Starting malloc test \n");
    assert(heap_setup() == SUCCESS);
    size_t size = 10;
    int *ptr;
    double *ptr2;
    unsigned int *ptr3;
    
    ptr = (int*) heap_malloc(sizeof(int) * size);
    assert(ptr != NULL);
    ptr2 = (double*) heap_malloc(sizeof(double) * size);
    assert(ptr2 != NULL);
    
    ptr3 = (unsigned int*) heap_malloc(sizeof(unsigned int) * size);
    assert(ptr3 != NULL);
    
    for(unsigned int i = 0; i < size; i++){
        *(ptr + i) = rand()%1024 - 500;
        *(ptr2 + i)= 2.5 + i;
        *(ptr3 + i) = rand()%255;
    }
    printf("Print data: ");
    for(unsigned int i = 0; i < size; i++){
        printf("%d %lf %u\n",*(ptr + i),*(ptr2 + i),*(ptr3 + i));
    }
    heap_free(ptr);
    heap_free(ptr2);
    heap_free(ptr3);
    heap_dump_debug_information();
    printf("Test passed\n");
    printf("Length between beginning and ending node : %zu\n",getDistnace(Heap));
    destroyHeap();
}

void differentAllocationTest(){
    //Using same space for different allocations
    printf("Starting different allocation types test \n");
    assert(heap_setup() == SUCCESS);
    size_t size = 10;
    intptr_t addressTable[10];
    enum pointer_type_t type;
    unsigned int **ptr = (unsigned int**) heap_malloc(sizeof(unsigned int*) * size) ;
    assert(ptr != NULL);
    
    for(unsigned int i = 0; i < size; i++){
        *(ptr + i) = (unsigned int *) heap_malloc(sizeof(unsigned int) * (size/2));
        assert(*(ptr + i) != NULL);
    }
    
    for(unsigned int i = 0; i < size; i++){
        addressTable[i]= (intptr_t)(*(ptr + i));
        heap_free(*(ptr+i));
        assert((type = get_pointer_type(*(ptr + i))) != pointer_valid);
    }
    for(unsigned int i = 0; i < size; i++){
        *(ptr + i) = (unsigned int *) heap_malloc(sizeof(unsigned int) * (size/2));
        assert(*(ptr + i) != NULL);
        assert(addressTable[i] == (intptr_t)(*(ptr + i)));
    }
    for(unsigned int i = 0; i < size ; i++){
        heap_free(*(ptr + i));
    }
    heap_dump_debug_information();
    printf("Test passed\n");
   printf("Length between beginning and ending node : %zu\n",getDistnace(Heap));
    destroyHeap();
}
void invalidFencesTest(){
    
    //Testing calloc and checking fences. User tried to write value to address which is in right fence
    printf("Starting invalid fences test \n");
    assert(heap_setup() == SUCCESS);
    size_t size = 10;
    int *ptr = (int*) heap_calloc(sizeof(int),size);
    assert(ptr != NULL);
    for(int i = 0; i < size; i++){
        *(ptr + i) = rand()%25;
    }
    assert(heap_validate() == 0);
    *(ptr + size) = size;
    assert(heap_validate() == -1);
    heap_free(ptr);
    printf("Test passed\n");
    printf("Length between beginning and ending node : %zu\n",getDistnace(Heap));
    destroyHeap();
}

void reallocTest(){
    
    //Testing realloc function
    printf("Starting realloc test\n");
    assert(heap_setup() == SUCCESS);
    size_t size = 10;
    enum pointer_type_t type;
    int * ptr = (int*) heap_realloc(NULL,sizeof(int) * (size/2));
    assert(ptr != (void*)0);
    for(int i = 0 ; i < (size/2); i++){
        *(ptr + i) = i;
    }
    int *newPtr;
    newPtr = (int*) heap_realloc(ptr,sizeof(int) * size);
    assert(newPtr != (void*)0);
    for(int i = 0; i < size ; i++){
        if(i < (size/2))
            *(newPtr + i) = *(ptr + i);
        else
            *(newPtr + i) = i;
        printf("%d ",*(newPtr + i));
    }
    heap_realloc(ptr,0);
    assert((type = get_pointer_type(ptr)) != pointer_valid);
    assert(heap_validate() == 0);
    assert(heap_get_used_blocks_count() == 1);
    heap_realloc(newPtr,0);
    assert((type = get_pointer_type(newPtr)) != pointer_valid);
    assert(heap_validate() == 0);
    assert(heap_get_used_blocks_count() == 0);
    heap_dump_debug_information();
    printf("Test passed\n");
    printf("Length between beginning and ending node : %zu\n",getDistnace(Heap));
    destroyHeap();
}

void mallocCallocAlignedTest(){
    //testing malloc and calloc aligned
    printf("Starting malloc & calloc aligned test\n");
    assert(heap_setup() == SUCCESS);
    size_t size = 9;
    int **ptr = (int**) (heap_malloc_aligned(sizeof(int*) * size));
    assert(ptr != (void*)0);
    for(int i = 0; i < size; i++){
        *(ptr + i) = (int *) heap_malloc_aligned(sizeof(int) *(size/2));
        assert(((intptr_t)ptr[i] & (intptr_t)(4096 - 1)) == 0);
        heap_free(*(ptr + i));
        *(ptr + i) = (int *) heap_calloc_aligned(sizeof(int),(size/2));
        assert(((intptr_t)ptr[i] & (intptr_t)(4096 - 1)) == 0);
        for(int j = 0; j < size/2; j++)
            assert((*(*(ptr + i) + j)) == 0);
        heap_free(*(ptr + i));
    }
    printf("\n");
    
    heap_dump_debug_information();
    heap_free(ptr);
    heap_dump_debug_information();
    printf("Test passed\n");
    printf("Length between beginning and ending node : %zu\n",getDistnace(Heap));
    destroyHeap();
}

void reallocAlignedTest(){
    // testing realloc aligned
    printf("Starting realloc aligned test \n");
    assert(heap_setup() == SUCCESS);
    size_t size = 9;
    int *ptr = (int *) heap_realloc_aligned(NULL,sizeof(int) * size);
    assert(ptr != (void*)0);
    //assert(((intptr_t)ptr & (intptr_t)(4096 - 1)) == 0);
    for(int i = 0; i < size; i++){
        *(ptr + i) = i;
        printf("%d ",*(ptr+i));
    }
    putchar('\n');
    int *newPtr = (int*) heap_realloc_aligned(ptr,sizeof(int)* size * 2);
    assert(newPtr != NULL);
    //assert(((intptr_t)newPtr & (intptr_t)(4096 - 1)) == 0);
    for(int i = 0; i < (size * 2) ; i++){
        if(i < size)
            *(newPtr + i) = *(ptr + i);
        else
            *(newPtr + i) = i;
        printf("%d ",*(newPtr + i));
    }     
    printf("\n");
    heap_realloc_aligned(newPtr,0);
    heap_dump_debug_information();
    printf("\nTest passed\n");
    printf("Length between beginning and ending node : %zu\n",getDistnace(Heap));
    destroyHeap();
}
void multithreadingTest(){
    //Testowanie wielowątkowości Czy nie uszkodzi ona Sterty
    printf("Starting multithreading test \n");
    assert(heap_setup() == SUCCESS);
    pthread_t arr[4];
    srand(time(0));
    for(int i = 0; i < 4; i+=2){
        pthread_create(&arr[i],NULL,foo,NULL);
        usleep(100000);
        pthread_create(&arr[i+1],NULL,foo2,NULL);
        usleep(100000);
    }
    for(int i = 0; i < 4; i++){
        pthread_join(arr[i],NULL);
    }
    assert(heap_validate() != -1);
    printf("Test passed\n");
    printf("Length between beginning and ending node : %zu\n",getDistnace(Heap));
    destroyHeap();
}
void overflowTest(){
    printf("\nStarting overflow test \n");
    int status = heap_setup();
	assert(status == 0);
	void* p1 = heap_malloc(8 * 1024 * 1024); // 8MB
	void* p2 = heap_malloc(8 * 1024 * 1024); // 8MB
	void* p3 = heap_malloc(8 * 1024 * 1024); // 8MB
	void* p4 = heap_malloc(45 * 1024 * 1024); // 45MB
	assert(p1 != NULL); // malloc musi się udać
	assert(p2 != NULL); // malloc musi się udać
	assert(p3 != NULL); // malloc musi się udać
	assert(p4 == NULL); // nie ma prawa zadziałać
    // Ostatnia alokacja, na 45MB nie może się powieść,
    // ponieważ sterta nie może być aż tak 
    // wielka (brak pamięci w systemie operacyjnym).
	status = heap_validate();
	assert(status == 0); 
	assert(heap_get_used_blocks_count() == 3);
	// zajęto 24MB sterty; te 2000 bajtów powinno
    // wystarczyć na wewnętrzne struktury sterty
	assert(
        heap_get_used_space() >= 24 * 1024 * 1024 &&
        heap_get_used_space() <= 24 * 1024 * 1024 + 2000
        );
	heap_free(p1);
	heap_free(p2);
	heap_free(p3);
	assert(heap_get_used_blocks_count() == 0);
    printf("Length between beginning and ending node : %zu\n",getDistnace(Heap));
    destroyHeap();
    printf("Test passed\n");
}

void chooseTest()
{
    int testsNum = 8;
    for(int choice = 1; choice <= testsNum; choice++){
        switch(choice){
            case 1:
                mallocTest();
                break;
            case 2: 
                differentAllocationTest();
                break;
            case 3:
                invalidFencesTest();
                break;
            case 4:
                reallocTest();
                break;
            case 5:
                mallocCallocAlignedTest();
                break;
            case 6:
                reallocAlignedTest();
                break;
            case 7:
                multithreadingTest();
                break;
            case 8:
                overflowTest();
                break;
        }
    }
}


#endif