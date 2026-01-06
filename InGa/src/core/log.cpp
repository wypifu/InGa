#include <InGa/core/log.h> 
#include <InGa/core/allocator.h>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <filesystem>
#include <mutex>

namespace Inga
{
namespace fs = std::filesystem;

    struct LogInternal
    {
        char* buffer = nullptr;
        size_t bufferSize = 0;
        LogLevel minLevel = eINFO;
        LogOutput output = eTERMOUT;
        std::mutex mutex;
        bool inited = false;
        char filePath[512];
    } gLog;

    static const char* LevelStrings[] = 
    {
        "\x1b[1;35m VERBOSE \x1b[0m", "\x1b[1;34m  DEBUG  \x1b[0m",
        "\x1b[1;92m  INFO   \x1b[0m", "\x1b[0;33m WARNING \x1b[0m",
        "\x1b[0;31m  ERROR  \x1b[0m", "\x1b[7;31m  FATAL  \x1b[0m"
    };

    static const char* LevelStringsPlain[] = 
    {
        "VERBOSE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
    };

    bool Log::init(LogLevel level, LogOutput output, const char* folder)
    {
        std::lock_guard<std::mutex> lock(gLog.mutex);
        
        gLog.minLevel = level;
        gLog.output = output;
        gLog.bufferSize = 1024;
        gLog.buffer = INGA_NEW char[gLog.bufferSize];

        // Protection : Création du dossier si inexistant
        try 
        {
            if (!fs::exists(folder))
            {
                fs::create_directories(folder);
            }
        }
        catch (const fs::filesystem_error& e)
        {
            fprintf(stderr, "[LOG ERROR] Erreur filesystem : %s\n", e.what());
        }

        time_t now = time(nullptr);
        struct tm* ts = localtime(&now);
        char timeBuf[64];
        strftime(timeBuf, sizeof(timeBuf), "%Y_%m_%d_%H_%M_%S", ts);
        
        snprintf(gLog.filePath, sizeof(gLog.filePath), "%s/inga_%s.log", folder, timeBuf);
        
        FILE* f = fopen(gLog.filePath, "wt");
        if (f)
        {
            fprintf(f, "--- INGA ENGINE LOG START ---\n");
            fclose(f);
            gLog.inited = true;
            return true;
        }

        fprintf(stderr, "[LOG ERROR] Impossible de creer le fichier : %s\n", gLog.filePath);
        gLog.inited = true; 
        return true;
    }

    void Log::terminate()
    {
        std::lock_guard<std::mutex> lock(gLog.mutex);
        if (gLog.buffer)
        {
            delete[] gLog.buffer;
            gLog.buffer = nullptr;
        }
        gLog.inited = false;
    }

    void Log::message(LogLevel level, const char* tag, const char* color, 
                      const char* file, const char* func, int line, 
                      const char* fmt, ...)
    {
        if (!gLog.inited || level < gLog.minLevel) return;

        std::lock_guard<std::mutex> lock(gLog.mutex);

        va_list args;
        va_start(args, fmt);
        int needed = vsnprintf(nullptr, 0, fmt, args);
        va_end(args);

        if (needed >= (int)gLog.bufferSize)
        {
            delete[] gLog.buffer;
            gLog.bufferSize = needed + 1;
            gLog.buffer = INGA_NEW char[gLog.bufferSize];
        }

        va_start(args, fmt);
        vsnprintf(gLog.buffer, gLog.bufferSize, fmt, args);
        va_end(args);

        const char* fileName = getFileName(file);
        
        // --- Sortie Console ---
        if (gLog.output & eTERMOUT)
        {
            // On a ajouté func ici après le nom du fichier
            if (color)
                printf("[%s][%s] (%s | %s:%d) %s%s\x1b[0m\n", LevelStrings[level], tag, func, fileName, line, color, gLog.buffer);
            else
                printf("[%s][%s] (%s | %s:%d) %s\n", LevelStrings[level], tag, func, fileName, line, gLog.buffer);
        }

        // --- Sortie Fichier ---
        if (gLog.output & eFILEOUT)
        {
            FILE* f = fopen(gLog.filePath, "at");
            if (f)
            {
                time_t now = time(nullptr);
                struct tm* ts = localtime(&now);
                char tBuf[10];
                strftime(tBuf, sizeof(tBuf), "%H:%M:%S", ts);

                fprintf(f, "[%s][%s][%s] (%s | %s:%d) %s\n", 
                        tBuf, LevelStringsPlain[level], tag, func, fileName, line, gLog.buffer);
                fclose(f);
            }
        }

        if (level == eFATAL)
        {
            fprintf(stderr, "[FATAL ERROR] Hitting the wall. Check log: %s\n", gLog.filePath);
            *(volatile int*)0 = 0; 
        }
    }

    const char* Log::getFileName(const char* path)
    {
        const char* lastSlash = strrchr(path, '/');
        if (!lastSlash) lastSlash = strrchr(path, '\\');
        return lastSlash ? lastSlash + 1 : path;
    }
}
