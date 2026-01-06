#ifndef INGA_H
#define INGA_H

#include "core/log.h"
#include <exception>

namespace Inga
{
#ifndef _WIN32
    inline void signalHandler(int sig) {
        INGA_LOG(eFATAL, "PANIC", "Signal %d received (Hard Crash). Flushing logs...", sig);
        Log::terminate();
        Allocator::stop();
        std::exit(sig);
    }
#endif

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

#ifdef _WIN32
#include <windows.h>
#define INGA_PLATFORM_BEGIN \
        int _inga_result = 0; \
        __try { \
            _inga_result = [&]() -> int { \
                try { {

#define INGA_PLATFORM_END \
                    return 0; \
                } } \
                catch (const std::exception& e) { INGA_LOG(eFATAL, "PANIC", "Exception: %s", e.what()); return -1; } \
                catch (...) { INGA_LOG(eFATAL, "PANIC", "Unknown C++ Exception"); return -1; } \
            }(); \
        } \
        __except(EXCEPTION_EXECUTE_HANDLER) { \
            INGA_LOG(eFATAL, "PANIC", "SEH Hardware Fault"); \
            _inga_result = -1; \
        }
#else
#include <csignal>
#define INGA_PLATFORM_BEGIN \
        std::signal(SIGSEGV, Inga::signalHandler); \
        int _inga_result = [&]() -> int { \
            try { {

#define INGA_PLATFORM_END \
                return 0; \
            } } \
            catch (const std::exception& e) { INGA_LOG(eFATAL, "PANIC", "Exception: %s", e.what()); return -1; } \
            catch (...) { INGA_LOG(eFATAL, "PANIC", "Unknown Linux Crash"); return -1; } \
        }();
#endif

#define INGA_BEGIN(conf) \
    if (!Inga::Allocator::start(conf.maxPageCount, conf.pageSize)) return -1; \
    Inga::Log::init(conf.logLevel, conf.logOutput, conf.logFile); \
    INGA_PLATFORM_BEGIN

#define INGA_END() \
    INGA_PLATFORM_END \
    Inga::Log::terminate(); \
    Inga::Allocator::stop(); \
    return _inga_result;

#endif

/*
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
*/
