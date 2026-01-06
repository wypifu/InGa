#include <InGa/core/clock.h>

namespace Inga
{
    Clock::Clock()
        : m_deltaTime(0.0f)
        , m_totalTime(0.0)
        , m_timeScale(1.0f)
    {
        m_startTime = HighResClock::now();
        m_lastTime = m_startTime;
    }

    void Clock::tick()
    {
        TimePoint currentTime = HighResClock::now();
        
        // Calcul de la durée depuis le dernier tick
        std::chrono::duration<float> elapsed = currentTime - m_lastTime;
        
        // On applique le timeScale au deltaTime pour les effets de ralenti
        m_deltaTime = elapsed.count() * m_timeScale;

        // Calcul du temps total écoulé (non affecté par le timeScale généralement)
        std::chrono::duration<double> total = currentTime - m_startTime;
        m_totalTime = total.count();

        m_lastTime = currentTime;
    }
}
