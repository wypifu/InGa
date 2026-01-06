#ifndef INGA_H
#define INGA_H

#include "core/Allocator.h"
#include "core/Log.h"
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

    EngineConfig getDefaultEngineConfig()
    {
        EngineConfig conf = {};
        conf.maxPageCount    = 64;
        conf.pageSize        = 16 * 1024 * 1024;
        conf.logLevel   = LogLevel::eVERBOSE;
        conf.logOutput = LogOutput::eALLOUT;
        conf.logFile = "logs/inga_latest.log";

        return conf;
    }
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
