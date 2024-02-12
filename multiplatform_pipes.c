#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <pthread.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif


#ifdef __linux__

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t emptyOrFull = PTHREAD_COND_INITIALIZER;

#endif

#ifdef _WIN32

CONDITION_VARIABLE pipeFullOrEmpty = CONDITION_VARIABLE_INIT;
CRITICAL_SECTION pipeLock;

#endif

int wasEmpty = 0;
int wasFull = 0;

struct pipe
{
    unsigned char* buffer;
    unsigned int head;
    unsigned int tail;
    unsigned int bufferSize;
    unsigned int dataSize;
    unsigned int isOpen;
};

// Returns 1 byte from pipe's buffer

unsigned char getByte(struct pipe* pipe)
{
    unsigned int tail = pipe->tail;
    unsigned char byte = pipe->buffer[tail];

    if (tail < pipe->bufferSize - 1)
        pipe->tail++;

    else
        pipe->tail = 0;

    pipe->dataSize--;

    return byte;
}

// Writes 1 byte into pipe's buffer

void writeByte(struct pipe* pipe, unsigned char byte)
{
    unsigned int head = pipe->head;
    pipe->buffer[head] = byte;

    if (head < pipe->bufferSize - 1)
        pipe->head++;

    else
        pipe->head = 0;

    pipe->dataSize++;
}

// Function initializes pipe pointer and its variables

struct pipe* pipe_create(unsigned int size)
{
    struct pipe* pipe = (struct pipe*)malloc(sizeof(struct pipe));

    #ifdef _WIN32
    InitializeCriticalSection(&pipeLock);
    #endif

    pipe->buffer = (unsigned char*)malloc(size);
    pipe->head = 0;
    pipe->tail = 0;
    pipe->bufferSize = size;
    pipe->dataSize = 0;
    pipe->isOpen = 1;

    return pipe;
}

// Function writes as much bytes as it can into pipe buffer
// Function makes a thread sleep if pipe buffer is full
// I assumed that mutex locks/critical sections should be outside of for cycle
// Though should this be incorrect they could be easily placed inside the for cycle

unsigned int pipe_write(struct pipe* pipe, unsigned char* data, unsigned int size)
{
    unsigned int writtenBytes = 0;

    #ifdef __linux__
    pthread_mutex_lock(&lock);
    #endif

    #ifdef _WIN32
    EnterCriticalSection(&pipeLock);
    #endif

    while (writtenBytes != size && pipe->isOpen)
    {
        if (pipe->dataSize == pipe->bufferSize)
        {
            wasFull = 1;

            if (wasEmpty)
            {
                wasEmpty = 0;

                #ifdef __linux__
                pthread_cond_broadcast(&emptyOrFull);
                #endif

                #ifdef _WIN32
                WakeAllConditionVariable(&pipeFullOrEmpty);
                #endif    
            }

            #ifdef __linux__
            pthread_cond_wait(&emptyOrFull, &lock);
            #endif

            #ifdef _WIN32
            SleepConditionVariableCS(&pipeFullOrEmpty, &pipeLock, INFINITE);
            #endif
        }

        else
        {
            writeByte(pipe, data[writtenBytes]);
            writtenBytes++;
        }
    }

    #ifdef __linux__
    pthread_mutex_unlock(&lock);
    #endif

    #ifdef _WIN32
    LeaveCriticalSection(&pipeLock);
    #endif

    // I believe this prevents threads from sleeping indefinitely should such a case happen

    if (wasEmpty)
    {
        wasEmpty = 0;

    #ifdef __linux__
        pthread_cond_broadcast(&emptyOrFull);
    #endif

    #ifdef _WIN32
        WakeAllConditionVariable(&pipeFullOrEmpty);
    #endif
    }

    return writtenBytes;
}

// Function reads as much bytes as it can from pipe buffer
// Function makes a thread sleep if there is no data in pipe buffer
// I assumed that mutex locks/critical sections should be outside of for cycles
// Though should this be incorrect they could be easily placed inside the for cycle

unsigned int pipe_read(struct pipe* pipe, unsigned char* data, unsigned int size)
{
    unsigned int readBytes = 0;

    if (!pipe->isOpen)
    {
        int i = 0;

        int datasize = pipe->dataSize;

        while (i < size && i < datasize)
        {
            data[i] = getByte(pipe);

            i++;
        }

        return i;
    }

    #ifdef __linux__
    pthread_mutex_lock(&lock);
    #endif

    #ifdef _WIN32
    EnterCriticalSection(&pipeLock);
    #endif

    while (readBytes != size && pipe->isOpen)
    {
        if (pipe->dataSize == 0)
        {
            wasEmpty = 1;

            if (wasFull)
            {
                wasFull = 0;

                #ifdef __linux__
                pthread_cond_broadcast(&emptyOrFull);
                #endif

                #ifdef _WIN32
                WakeAllConditionVariable(&pipeFullOrEmpty);
                #endif
            }

            #ifdef __linux__
            pthread_cond_wait(&emptyOrFull, &lock);
            #endif

            #ifdef _WIN32
            SleepConditionVariableCS(&pipeFullOrEmpty, &pipeLock, INFINITE);
            #endif
        }

        else
        {
            data[readBytes] = getByte(pipe);

            readBytes++;
        }
    }

    #ifdef __linux__
    pthread_mutex_unlock(&lock);
    #endif

    #ifdef _WIN32
    LeaveCriticalSection(&pipeLock);
    #endif
    
    // I believe this prevents threads from sleeping indefinitely should such a case happen

    if (wasFull)
    {
        wasFull = 0;

        #ifdef __linux__
        pthread_cond_broadcast(&emptyOrFull);
        #endif

        #ifdef _WIN32
        WakeAllConditionVariable(&pipeFullOrEmpty);
        #endif
    }

    return readBytes;
}

// Closes pipe and sends signal to wake every thread

void pipe_close(struct pipe* pipe)
{
    pipe->isOpen = 0;

    #ifdef __linux__
    pthread_cond_broadcast(&emptyOrFull);
    #endif

    #ifdef _WIN32
    WakeAllConditionVariable(&pipeFullOrEmpty);
    #endif
}

// Function frees buffer and pipe structure
// Also deals with conditon variables and mutex locks 

void pipe_free(struct pipe* pipe)
{
    free(pipe->buffer);
    free(pipe);

    #ifdef __linux__

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&emptyOrFull);

    #endif

    #ifdef _WIN32
    DeleteCriticalSection(&pipeLock);
    #endif
}

int main()
{
    return 0;
}