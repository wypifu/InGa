#pragma once

// --- TYPES DE BASE (pour être sûr que U32, U64, etc. sont les mêmes partout) ---
#include <stdint.h>
#include <inttypes.h>

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t   I8;
typedef int16_t  I16;
typedef int32_t  I32;
typedef int64_t  I64;

typedef float    F32;
typedef double   F64;

// Booléen moteur (1 octet)
typedef uint8_t  B8;
#define INGA_TRUE  1
#define INGA_FALSE 0

// --- INCLUDES SYSTÈME ---
#include <stdio.h>
#include <stdlib.h>

// --- MACROS DE CRASH (HALT) ---
#if defined(_MSC_VER) // Microsoft Visual Studio
    #define INGA_HALT() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__) // Clang ou GCC
    #define INGA_HALT() __builtin_trap()
#else
    #define INGA_HALT() abort()
#endif

// --- ASSERT ---
#ifdef INGA_DEBUG
    #define INGA_ASSERT(condition, message) \
        if (!(condition)) { \
            printf("\n[ASSERT FAILED] %s\nFichier: %s, Ligne: %d\n", message, __FILE__, __LINE__); \
            INGA_HALT(); \
        }
#else
    #define INGA_ASSERT(condition, message) // Ne fait rien en mode Release
#endif

// --- MUTEX ---
#if defined(_WIN32)
    #include <windows.h>
    typedef CRITICAL_SECTION IngaMutex;
    #define INGA_MUTEX_INIT(m)    InitializeCriticalSection(m)
    #define INGA_MUTEX_LOCK(m)    EnterCriticalSection(m)
    #define INGA_MUTEX_UNLOCK(m)  LeaveCriticalSection(m)
    #define INGA_MUTEX_DESTROY(m) DeleteCriticalSection(m)
#else
    #include <pthread.h>
    typedef pthread_mutex_t IngaMutex;
    #define INGA_MUTEX_INIT(m)    pthread_mutex_init(m, NULL)
    #define INGA_MUTEX_LOCK(m)    pthread_mutex_lock(m)
    #define INGA_MUTEX_UNLOCK(m)  pthread_mutex_unlock(m)
    #define INGA_MUTEX_DESTROY(m) pthread_mutex_destroy(m)
#endif
