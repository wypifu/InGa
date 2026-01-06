#ifndef INGA_WINDOW_H
#define INGA_WINDOW_H

#include <InGa/core/export.h>
#include <InGa/core/inga_platform.h>

namespace Inga
{
enum class EWindowBackend
    {
        Unknown = 0,
        Win32,
        X11,
        Wayland,
        XCB
    };

    class INGA_API Window
    {
    public:
        Window();
        ~Window();

        bool create(U32 width, U32 height, const char* title);
        void pollEvents();
        bool isOpen() const { return m_isOpen; }
    void close();

        // Getters for Vulkan Surface creation
        inline void* getNativeWindow()  const { return m_nativeWindow; }
        inline void* getNativeDisplay() const { return m_nativeDisplay; }
        inline EWindowBackend getBackend()       const { return m_backend; }
        inline U32            getWidth()         const { return m_width; }
        inline U32            getHeight()        const { return m_height; }

#if defined(INGA_PLATFORM_WINDOWS)
        inline void* getHInstance()     const { return m_hInstance; }
#endif

    private:
        void* m_nativeWindow  = nullptr;
        void* m_nativeDisplay = nullptr;
        EWindowBackend m_backend       = EWindowBackend::Unknown;
        U32            m_width         = 0;
        U32            m_height        = 0;
        bool           m_isOpen        = false;

#if defined(INGA_PLATFORM_WINDOWS)
        void* m_hInstance     = nullptr;
#endif
    };
}

#endif
