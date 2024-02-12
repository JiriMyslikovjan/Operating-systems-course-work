#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>

#ifdef __linux__

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#endif

#ifdef _WIN32

#include <windows.h>
#include <tchar.h>
#include "atlstr.h"
#include "comutil.h"

// I needed to disable VSC warnings in order to use some unsafe functions

#pragma warning(disable:4996)

#endif

#define INT_MAX_DIGITS 10

// Struct containing all relevant data to process a PGM file

typedef struct PGMIMAGE
{
#ifdef __linux__

    int fd;
    long dataLen;

#endif

#ifdef _WIN32

    HANDLE fd;
    DWORD dataLen;
    HANDLE hMapping;
    LPVOID fileData;

#endif

    char* data;
    int width;
    int height;
    int depth;
} pgmImage;

// This is some sort of conversion to TCHAR and string appending
// I had problems with passing a path to a file so this had to be done

#ifdef _WIN32

TCHAR* getFilePath(char* path)
{
    USES_CONVERSION;

    TCHAR* winPathHelper = A2T(path);


    TCHAR* currDir = _tgetcwd(0, 0);

    currDir = _tcscat(currDir, _T("\\"));
    currDir = _tcscat(currDir, winPathHelper);

    return currDir;
}

#endif

// Gets head information of PGM file (height, width and gray depth)
// Function assumes that a PGM file has correct form
// Finds first 4 string, ignores first string (P2) and extracts information 

void getPgmHeadInfo(char* imgData, int* width, int* height, int* grayDepth)
{
    int strCnt = 0, strFound = 0, strLen = 0, isComment = 0;
    char widthStr[INT_MAX_DIGITS + 1];
    char heightStr[INT_MAX_DIGITS + 1];
    char grayDepthStr[INT_MAX_DIGITS + 1];
    char currChar = ' ';

    for (int i = 0; 4 > strCnt; i++)
    {
        currChar = imgData[i];

        if (currChar == '#')
            isComment = 1;

        if (isspace(currChar) && !isComment)
        {
            if (strFound)
            {
                strLen = 0;
                strCnt++;
            }

            strFound = 0;
        }

        if ((isalpha(currChar) || isdigit(currChar)) && !isComment)
            strFound = 1;

        if (strFound && !isComment)
        {
            switch (strCnt)
            {
            case 1:
                widthStr[strLen] = currChar;
                break;

            case 2:
                heightStr[strLen] = currChar;
                break;

            case 3:
                grayDepthStr[strLen] = currChar;
                break;

            default:
                break;
            }

            strLen++;
        }

        if (isComment && currChar == '\n')
            isComment = 0;
    }

    *width = atoi(widthStr);
    *height = atoi(heightStr);
    *grayDepth = atoi(grayDepthStr);
}

// Function opens a PGM file, then maps it into memory

pgmImage getPgmFile(char* path)
{
    pgmImage img;

#ifdef __linux__
    img.fd = open(path, O_RDONLY);
#endif

#ifdef _WIN32

    TCHAR* winPath = getFilePath(path);

    img.fd = CreateFile(winPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

#endif

#ifdef __linux__
    if (!img.fd)
#endif

#ifdef _WIN32
        if (img.fd == INVALID_HANDLE_VALUE)
#endif
        {
            printf("Failed to open file!\n");

            return img;
        }

#ifdef __linux__

    struct stat st;

    fstat(img.fd, &st);

    img.dataLen = st.st_size;

    img.data = mmap(NULL, img.dataLen, PROT_READ, MAP_PRIVATE, img.fd, 0);

#endif

#ifdef _WIN32

    img.dataLen = GetFileSize(img.fd, NULL);

    img.hMapping = CreateFileMapping(img.fd, NULL, PAGE_READONLY, 0, 0, NULL);

    if (img.hMapping == NULL)
    {
        printf("Unable to create file mapping\n");

        return img;
    }

    img.fileData = MapViewOfFile(img.hMapping, FILE_MAP_READ, 0, 0, img.dataLen);

#endif

#ifdef __linux__
    if (img.data == MAP_FAILED)
#endif

#ifdef _WIN32
        if (img.fd == INVALID_HANDLE_VALUE)
#endif
        {
            printf("Mapping failed\n");

            return img;
        }

#ifdef _WIN32
    img.data = (char*)img.fileData;
#endif

    getPgmHeadInfo(img.data, &img.width, &img.height, &img.depth);

    return img;
}

// Unmaps and closes PGM file

void closePgmFile(pgmImage img)
{
#ifdef __linux__

    munmap(img.data, img.dataLen);
    close(img.fd);

#endif

#ifdef _WIN32

    UnmapViewOfFile(img.fileData);
    CloseHandle(img.hMapping);
    CloseHandle(img.fd);

#endif
}

int digitsToInt(int* digits, int digCnt)
{
    int digitPlace = 1;
    int result = 0;

    for (int i = 1; i < digCnt; i++)
        digitPlace *= 10;

    for (int i = 0; i < digCnt; i++)
    {
        result += digits[i] * digitPlace;

        digitPlace /= 10;
    }

    return result;
}

// Opens a file and maps it into memory 
// then writes character from data buffer into said file

int writeToTextFile(char* path, char* data, int len)
{
#ifdef __linux__
    int fd = open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
#endif

#ifdef _WIN32

    TCHAR* winPath = getFilePath(path);

    HANDLE fd = CreateFile(winPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);

#endif

#ifdef __linux__
    if (!fd)
#endif

#ifdef _WIN32
        if (fd == INVALID_HANDLE_VALUE)
#endif
        {
            printf("Failed to open file!\n");

            return 1;
        }

#ifdef __linux__

    ftruncate(fd, len);

    char* fileData = mmap(NULL, len, PROT_WRITE, MAP_SHARED, fd, 0);

#endif

#ifdef _WIN32

    DWORD fileLen = (DWORD)len;

    HANDLE hMapping = CreateFileMapping(fd, NULL, PAGE_READWRITE, 0, fileLen, NULL);

    if (hMapping == NULL)
    {
        printf("Unable to create file mapping\n");

        return 1;
    }

    LPVOID fileData = MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, fileLen);

#endif

#ifdef __linux__
    if (fileData == MAP_FAILED)
#endif

#ifdef _WIN32
        if (fileData == NULL)
#endif
        {
            printf("Mapping failed\n");

            return 1;
        }

#ifdef __linux__

    strcpy(fileData, data);

    munmap(fileData, len);

    close(fd);

#endif

#ifdef _WIN32

    memcpy(fileData, data, (size_t)len);

    UnmapViewOfFile(fileData);
    CloseHandle(hMapping);
    CloseHandle(fd);

#endif

    return 0;

}

// Function skips first 4 strings the reads height * width string 
// and converts them to ascii characters based on a provided pallette 
// Function assumes that a PGM file has correct form

char* createASCIIArt(pgmImage img, char* pallette)
{
    int valCnt = 0, valFound = 0, digCnt = 0, isComment = 0, indentCnt = 0, valsIndex = 0;
    char* result = NULL, currChar = ' ';
    int digits[INT_MAX_DIGITS + 1];

    result = (char*)malloc((img.width + 1) * img.height);

    for (int i = 0; i < img.dataLen; i++)
    {
        currChar = img.data[i];

        if (currChar == '#')
            isComment = 1;

        if (isspace(currChar) && !isComment)
        {
            if (valFound)
            {
                valCnt++;

                if (valCnt > 4)
                {
                    int palletteIndex = digitsToInt(digits, digCnt);

                    result[valsIndex] = pallette[palletteIndex];

                    valsIndex++;
                    indentCnt++;

                    if (indentCnt % img.width == 0)
                    {
                        result[valsIndex] = '\n';

                        valsIndex++;
                    }
                }

                digCnt = 0;
            }

            valFound = 0;
        }

        if ((isdigit(currChar)) && !isComment)
            valFound = 1;

        if (valFound && !isComment && valCnt >= 4)
        {
            digits[digCnt] = currChar - '0';

            digCnt++;
        }

        if (isComment && currChar == '\n')
            isComment = 0;
    }

    return result;
}

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        printf("Too many or too few arguments!\n");

        return 1;
    }

    char* pgmFile = argv[1];
    char* txtFile = argv[2];
    char* pallette = argv[3];
    char* buffer = NULL;

    int palletteDepth = strlen(pallette);

    pgmImage img = getPgmFile(pgmFile);

    if (img.depth > palletteDepth)
    {
        printf("Pallette too short! Needs to be at least %d characters long\n", img.depth);

        return 1;
    }

    int ASCIIimgLen = (img.width + 1) * img.height;

    buffer = createASCIIArt(img, pallette);

    closePgmFile(img);

    writeToTextFile(txtFile, buffer, ASCIIimgLen);

    free(buffer);

    return 0;
}