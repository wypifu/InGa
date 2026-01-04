#ifndef INGA_EXPORT_H
#define INGA_EXPORT_H

#if defined(_WIN32) || defined(_WIN64)
    #ifdef INGA_BUILD_DLL
        #define INGA_API __declspec(dllexport)
    #else
        #define INGA_API __declspec(dllimport)
    #endif
#else
    #if defined(__GNUC__) || defined(__clang__)
        #define INGA_API __attribute__((visibility("default")))
    #else
        #define INGA_API
    #endif
#endif

#endif // INGA_EXPORT_H
