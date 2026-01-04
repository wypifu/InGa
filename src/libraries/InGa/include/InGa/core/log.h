#pragma once
#include <stdio.h>
#include <stdarg.h>
#include "inga_types.h"

namespace Inga 
{
    namespace Log 
    {
        #define ANSI_COLOR_RED     "\x1b[31m"
        #define ANSI_COLOR_GREEN   "\x1b[32m"
        #define ANSI_COLOR_YELLOW  "\x1b[33m"
        #define ANSI_COLOR_BLUE    "\x1b[34m"
        #define ANSI_COLOR_RESET   "\x1b[0m"

        inline void Message(LogLevel level, const char* category, const char* fmt, ...) 
        {
            const char* color = ANSI_COLOR_RESET;
            const char* prefix = "INFO";

            switch (level) 
            {
                case LogLevel::Warning: 
                    color = ANSI_COLOR_YELLOW; 
                    prefix = "WARN"; 
                    break;
                case LogLevel::Error:   
                    color = ANSI_COLOR_RED;    
                    prefix = "ERR "; 
                    break;
                case LogLevel::Fatal:   
                    color = ANSI_COLOR_RED;    
                    prefix = "FATL"; 
                    break;
                default:                
                    color = ANSI_COLOR_BLUE;   
                    prefix = "INFO"; 
                    break;
            }

            fprintf(stdout, "%s[%s][%s] ", color, category, prefix);
            
            va_list args;
            va_start(args, fmt);
            vprintf(fmt, args);
            va_end(args);

            fprintf(stdout, "%s\n", ANSI_COLOR_RESET);
        }
    }
}

#define INGA_LOG_INFO(cat, fmt, ...)  Inga::Log::Message(Inga::LogLevel::Info, cat, fmt, ##__VA_ARGS__)
#define INGA_LOG_ERROR(cat, fmt, ...) Inga::Log::Message(Inga::LogLevel::Error, cat, fmt, ##__VA_ARGS__)
