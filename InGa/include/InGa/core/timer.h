#ifndef INGA_TIMER_H
#define INGA_TIMER_H

#include "export.h"
#include "inga_platform.h"

namespace Inga
{
    class INGA_API ScopedTimer
    {
    public:
        ScopedTimer(const char* name);
        ~ScopedTimer();

    private:
        const char* m_name;
        U64 m_startTime; // Stocké en microsecondes
    };
}

// Macros pour l'utilisation
#define INGA_PROFILE_BLOCK(name) Inga::ScopedTimer timer_##__LINE__(name)
#define INGA_PROFILE_FUNC() INGA_PROFILE_BLOCK(__FUNCTION__)

#endif
