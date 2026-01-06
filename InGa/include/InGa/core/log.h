#ifndef INGA_LOG_H
#define INGA_LOG_H

#include "export.h"
#include "inga_platform.h"

namespace Inga {

enum LogLevel { eVERBOSE, eDEBUG, eINFO, eWARNING, eERROR, eFATAL };
enum LogOutput { eTERMOUT = 1, eFILEOUT = 2, eALLOUT = 3 };

class INGA_API Log
{
public:
    static bool init(LogLevel level, LogOutput output, const char* folder = "logs");
    static void terminate();

    // La fonction de base
    static void message(LogLevel level, const char* tag, const char* color, 
                        const char* file, const char* func, int line, 
                        const char* fmt, ...);

    static void setMaxFileSize(U32 size);

private:
    static const char* getFileName(const char* path);
};

} // namespace Inga

// Macros de confort
#define INGA_LOG(level, tag, ...) Inga::Log::message(level, tag, nullptr, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define INGA_LOG_COLOR(level, tag, color, ...) Inga::Log::message(level, tag, color, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#endif
