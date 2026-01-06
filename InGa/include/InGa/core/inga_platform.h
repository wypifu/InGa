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

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define INGA_PLATFORM_WINDOWS
    
    // Disable heavy Windows headers by default
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
#endif

// --- Linux Detection ---
#if defined(__linux__) || defined(__gnu_linux__)
    #define INGA_PLATFORM_LINUX
#endif

// --- Apple Detection ---
#if defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #define INGA_PLATFORM_MACOS
    #endif
#endif

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
// Assertion de base, ultra-stable, utilisable PARTOUT (même dans l'allocateur)
    #define INGA_ASSERT_RAW(condition, message) \
        if (!(condition)) { \
            fprintf(stderr, "\n[ASSERT FATAL] %s\nFile: %s, Line: %d\n", message, __FILE__, __LINE__); \
            INGA_HALT(); \
        }

    // Assertion "Moteur", qui utilise ton beau Logger (pour tout sauf l'allocateur)
    #define INGA_ASSERT(condition, message) \
        if (!(condition)) { \
            Inga::Log::message(Inga::eFATAL, "ASSERT", nullptr, __FILE__, __FUNCTION__, __LINE__, message); \
            INGA_HALT(); \
        }
#else
#define INGA_ASSERT_RAW(condition, message)
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
