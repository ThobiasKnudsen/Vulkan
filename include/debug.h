#pragma once

#include <stddef.h>  // For size_t
#include <stdio.h>   // For printf and snprintf

// Function declarations
void DebugPrintf(const char* message, size_t line, const char* file);
void* DebugMalloc(size_t size, size_t line, const char* file);
void* DebugRealloc(void* ptr, size_t size, size_t line, char* file);
void DebugFree(void* ptr, size_t line, const char* file);
void DebugStart(size_t line, const char* file);
void DebugEnd();
size_t DebugGetSizeBytes(void* ptr);
void DebugPrintMemory();

#ifdef DEBUG

#define malloc(size)       DebugMalloc (size, __LINE__, __FILE__)
#define realloc(ptr, size) DebugRealloc(ptr, size, __LINE__, __FILE__)
#define free(ptr)          DebugFree   (ptr, __LINE__, __FILE__)
#define debug(code)        DebugStart(__LINE__, __FILE__); code; DebugEnd();
#define TRACK(code)        DebugStart(__LINE__, __FILE__); code; DebugEnd();
#define printf(fmt, ...)   do {char buffer[512]; snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); DebugPrintf(buffer, __LINE__, __FILE__); } while (0)
#define VERIFY(expr, fmt, ...) \
    do { \
        if (!(expr)) { \
        	char buffer_1[512]; \
        	char buffer_2[1024]; \
        	snprintf(buffer_1, sizeof(buffer_1), "VERIFY exiting | expr is false | %s\n", #expr); \
            DebugPrintf(buffer_1, __LINE__, __FILE__); \
            snprintf(buffer_1, sizeof(buffer_1), fmt, ##__VA_ARGS__); \
            snprintf(buffer_2, sizeof(buffer_2), "VERIFY exiting | exit message  | %s", buffer_1); \
            DebugPrintf(buffer_2, __LINE__, __FILE__); \
            exit(-1); \
        } \
    } while (0)

#else

#define debug(code) code
#define TRACK(code)	code 
#define VERIFY(expr, fmt, ...)  

#endif
