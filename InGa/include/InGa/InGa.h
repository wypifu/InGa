#ifndef INGA_H
#define INGA_H

#include "core/log.h"
#include <exception>

namespace Inga
{
    struct EngineConfig
    {
        U32 maxPageCount ;
        U64 pageSize  ;
        LogLevel logLevel ;
        LogOutput logOutput ;
        const char* logFile ;
    };

    EngineConfig INGA_API getDefaultEngineConfig();
}


#define INGA_BEGIN(conf) \
    if (!Inga::Allocator::start(conf.maxPageCount, conf.pageSize)) \
    { \
        return -1; \
    } \
    Inga::Log::init(conf.logLevel, conf.logOutput, conf.logFile); \
    try { \
        { 

#define INGA_END() \
        } \
    } \
    catch (const std::exception& e) { \
        INGA_LOG(eFATAL, "PANIC", "Unhandled Exception: %s", e.what()); \
    } \
    catch (...) { \
        INGA_LOG(eFATAL, "PANIC", "An unknown critical error occurred!"); \
    } \
    Inga::Log::terminate(); \
    Inga::Allocator::stop();
#endif
