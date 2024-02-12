#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_LINE_LEN 1000

// Structure for thread function, includes a part of an input buffer,
// entire input buffer, entire inputs buffer size, index of the 1st line in part,
// number of lines in the part, number of results and final result. 
typedef struct
{
    char ** part;
    char ** wholeInput;
    int wholeInputStart;
    int wholeInputSize;
    int partSize;
    int numOfResults;
    char ** result;
} thrdFuncInfo;

// Allocates an array with startRow rows each row can store MAX_LINE_LEN characters
char ** initBuffer(int startRows)
{
    char ** buffer = NULL;

    buffer = (char **)malloc(startRows * sizeof(char *));

    for (int i = 0; i < startRows; i++)
        buffer[i] = (char *)malloc(MAX_LINE_LEN);

    return buffer;
}

// Frees 2D array
void freeBuffer(char ** buffer, int maxRows)
{
    for(int i = 0; i < maxRows; i++)
        free(buffer[i]);

    free(buffer);
}

// Doubles the amount of rows allocated for an array
char ** resizeBuffer(char ** buffer, int * maxRows)
{
    int resizedRows = (* maxRows) * 2;

    buffer = realloc(buffer, resizedRows * sizeof(*buffer));

    for (int i = * maxRows; i < resizedRows; i++)
        buffer[i] = (char *)malloc(MAX_LINE_LEN);
    
    * maxRows = resizedRows;

    return buffer;
}

// Reads line by line form standard input, stores lines into a buffer, if needed resizes said buffer
char ** reader(char ** buffer, int * maxRows, int * linesRead )
{
    // Check for uninitialized buffer
    if(buffer == NULL)
        return NULL;
    
    int lineCount = *linesRead;

    while ((fgets(buffer[lineCount], MAX_LINE_LEN, stdin)))
    {
        // Resize buffer if limit is reached
        if(lineCount == (* maxRows) -1)
            buffer = resizeBuffer(buffer, maxRows);
        
        lineCount++;
    }

    * linesRead = lineCount;
    
    return buffer;
}

// Initializes arguments for thread function. 
thrdFuncInfo initInfo(int from, int to, int readLines, char ** buffer)
{
    thrdFuncInfo info;
    int currLine = 0;

    info.partSize = to - from;
    info.part = initBuffer(info.partSize);
    info.wholeInputStart = from;
    info.wholeInputSize = readLines;
    info.wholeInput = buffer;
    info.result = initBuffer(info.partSize);
    info.numOfResults = 0;

    // Copy corresponding lines from input buffer
    for (int i = from; i < to; i++)
    {
        strcpy(info.part[currLine], buffer[i]);

        currLine++;
    }

    return info;   
}

// Splits buffer into parts and saves prepares them for thread function
thrdFuncInfo * splitBuffer(int parts, int readLines, char ** buffer)
{
    // Calculate number of parts, 
    int startPartFrom = 0;
    int partSize = readLines / parts;
    int unevenLines = readLines % parts;
    thrdFuncInfo * partsInfo = (thrdFuncInfo *)malloc(parts * sizeof(thrdFuncInfo)); 

    // Split input buffer into parts of euqal or similar lenght
    for (int i = 0; i < parts; i++)
    {
        // Add extra line when spliting input if lines could not be distributed equally
        if(unevenLines > 0)
        {
            partsInfo[i] = initInfo(startPartFrom, startPartFrom + partSize + 1, readLines, buffer);

            startPartFrom += partSize + 1;

            unevenLines--;
        }
        // If not proceed normally
        else
        {
            partsInfo[i] = initInfo(startPartFrom, startPartFrom + partSize, readLines, buffer);

            startPartFrom += partSize;
        }
    }

    return partsInfo;
}

// Thread function to find unique strings, locates string in input buffer, 
// then iterates backwards to find duplicate strings 
void * threadFindUnique(void * args)
{
    thrdFuncInfo * info =  args;
    int foundStrings = 0, results = 0, wholeInputCurrLine = info->wholeInputStart - 1;
    
    // First cycle iterates over lines of a part
    for (int i = 0; i < info->partSize; i++)
    {
        // Second cycle iterates backwards over input buffer from index of compared line
        for (int j = wholeInputCurrLine; j >= 0; j--)
        {
            // Breaks cycle if duplicate is found
            if(strcmp(info->part[i], info->wholeInput[j]) == 0)
            {
                foundStrings++;

                break;
            }
        }

        wholeInputCurrLine++;

        // If no duplicates were found copy string into result
        if(foundStrings < 1)
        {
            strcpy(info->result[results], info->part[i]);
            
            results++;
        }
        
        foundStrings = 0;
    }

    info->numOfResults = results;  

    return 0;  
}

int runThreadsLinux(thrdFuncInfo * partsInfo, pthread_t * threadId, int numOfThreads)
{
    for(int i = 0; i < numOfThreads; i ++)
    {
        if(pthread_create(&threadId[i], NULL, threadFindUnique, &partsInfo[i]))
        {
            printf("An error has occured while creating threads\n");

            return 1;
        }
    }
    // Wait for all threads to finish
    for (int i = 0; i < numOfThreads; i++)
        pthread_join(threadId[i], NULL);

    return 0;

}

int main(int argc, char ** argv)
{
    int rows = 5, readLines = 0, numOfThreads = 1, parts = 0;
    // Initialize buffer for storing lines from standar input
    char ** buffer = initBuffer(rows);

    // If an argument was given then set number of threads to value of argument  
    if(argc > 1)
        numOfThreads = atoi(argv[1]);

    pthread_t * threadId = (pthread_t *)malloc(numOfThreads * sizeof(pthread_t));

    // Read lines from standad input
    if((buffer = reader(buffer, &rows, &readLines)) == NULL)
    {
        printf("Uninitialized buffer was passed to reader\n");

        return 1;
    }

    parts = numOfThreads;
    // Split input buffer into parts
    thrdFuncInfo * partsInfo = splitBuffer(parts, readLines, buffer);
   
    if(runThreadsLinux(partsInfo, threadId, numOfThreads) == 1)
        return 1;

    // Print results
    for (int i = 0; i < parts; i++)
    {
        for (int j = 0; j < partsInfo[i].numOfResults; j++)
            printf("%s", partsInfo[i].result[j]);   
    }

    //Free all allocated memory
    for (int i = 0; i < parts; i++)
    {
        freeBuffer(partsInfo[i].part, partsInfo[i].partSize);
        freeBuffer(partsInfo[i].result, partsInfo[i].numOfResults);
    }
    

    freeBuffer(buffer, rows);
    free(threadId);
    
    return 0;
}
