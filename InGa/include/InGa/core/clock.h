#ifndef INGA_CLOCK_H
#define INGA_CLOCK_H

#include <InGa/core/export.h>
#include <chrono>

namespace Inga
{
    class INGA_API Clock
    {
    public:
        Clock();

        void tick();

        // Temps écoulé depuis la dernière image (en secondes)
        // Utile pour : position += vitesse * deltaTime;
        float getDeltaTime() const { return m_deltaTime; }

        // Temps total depuis le démarrage du moteur
        double getTotalTime() const { return m_totalTime; }

        // Permet de ralentir ou d'accélérer le temps (ex: 0.5f pour un bullet time)
        void setTimeScale(float scale) { m_timeScale = scale; }
        float getTimeScale() const { return m_timeScale; }

    private:
        using HighResClock = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point<HighResClock>;

        TimePoint m_startTime;
        TimePoint m_lastTime;

        float m_deltaTime;
        double m_totalTime;
        float m_timeScale;
    };
}

#endif
