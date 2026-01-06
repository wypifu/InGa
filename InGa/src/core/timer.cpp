#include <InGa/core/timer.h>
#include <InGa/core/log.h>
#include <chrono>

namespace Inga
{
ScopedTimer::ScopedTimer(const char* name)
        : m_name(name)
    {
        // On récupère le temps actuel en microsecondes
        auto now = std::chrono::high_resolution_clock::now();
        m_startTime = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()
        ).count();
    }

    ScopedTimer::~ScopedTimer()
    {
        auto now = std::chrono::high_resolution_clock::now();
        U64 endTime = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()
        ).count();

        U64 duration = endTime - m_startTime;

        // Log avec précision
        // On affiche en microsecondes (us) et en millisecondes (ms)
        INGA_LOG(Inga::eDEBUG, "PROFILE", "Scope [%s] : %llu us (%.3f ms)", 
                 m_name, 
                 duration, 
                 static_cast<float>(duration) / 1000.0f);
    }
}
