#include <InGa/InGa.h>

namespace Inga
{
    EngineConfig INGA_API getDefaultEngineConfig()
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
