#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

struct pair
{
    void * ar;
    void * dr;
};

#define PAIR_SIZE sizeof(struct pair)
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define PAIRS_PER_PAGE PAGE_SIZE / PAIR_SIZE

// Linked list for keeping free blocks of memory

struct pair * pairPool = NULL;
struct pair * pairPoolHead = NULL;

unsigned int pairPoolInitialized = 0;

// Function maps memory and initializes linked list
// I used a singly linked list because I found it simpler
// Hopefully this is correct

void memPoolInit()
{
    int fd = open("/dev/zero", O_RDWR);

    pairPool = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    
    close(fd);

    pairPoolHead = pairPool;

    for(int i = 0; i < PAIRS_PER_PAGE - 1; i++)
    {
        pairPool[i].ar = i;
        pairPool[i].dr = &pairPool[i + 1]; 
    }

    pairPool[PAIRS_PER_PAGE - 1].ar = PAIRS_PER_PAGE - 1;
    pairPool[PAIRS_PER_PAGE - 1].dr = NULL;
}

struct pair * lalloc()
{
    // If lalloc is called for the first time initializes linked list
    if(! pairPoolInitialized)
    {
        memPoolInit();

        pairPoolInitialized = 1;
    }

    // No free memory blocks are availible

    if(pairPoolHead == NULL)
        return NULL;

    struct pair * allocatedPair = pairPoolHead;
    pairPoolHead = pairPoolHead->dr;

    return allocatedPair;
}

void lfree(struct pair * p)
{
    // Makes pointer p a new head of linked list
    struct pair * prevHead = pairPoolHead;

    pairPoolHead = p;
    pairPoolHead->dr = prevHead;
}

int main()
{
    return 0;
}