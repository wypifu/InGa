#include <InGa/core/window.h>
#include <InGa/core/log.h>
#include <cstring>

#if defined(INGA_PLATFORM_WINDOWS)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(INGA_PLATFORM_LINUX)
    #include <X11/Xlib.h>
    #include <xcb/xcb.h>
    #include <wayland-client.h>
    // Note: In a production environment, you'd generate xdg-shell headers.
    // Here we use the core wayland handles.
#endif

namespace Inga
{

#if defined(INGA_PLATFORM_WINDOWS)
LRESULT CALLBACK InGaWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CLOSE || uMsg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif

Window::Window()
{
}

Window::~Window()
{
    close();
}

bool Window::create(U32 width, U32 height, const char* title)
{
    m_width = width;
    m_height = height;

#if defined(INGA_PLATFORM_WINDOWS)
    m_backend = EWindowBackend::Win32;
    m_hInstance = GetModuleHandle(nullptr);

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = InGaWindowProc;
    wc.hInstance = (HINSTANCE)m_hInstance;
    wc.lpszClassName = "InGaWindowClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassExA(&wc)) return false;

    m_nativeWindow = CreateWindowExA(
        0, wc.lpszClassName, title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, (int)width, (int)height,
        nullptr, nullptr, (HINSTANCE)m_hInstance, nullptr
    );

    if (!m_nativeWindow) return false;

    ShowWindow((HWND)m_nativeWindow, SW_SHOW);
    m_isOpen = true;

#elif defined(INGA_PLATFORM_LINUX)
    // 1. Try Wayland First (Modern Standard)
    struct wl_display* display = wl_display_connect(nullptr);
    if (display)
    {
        m_backend = EWindowBackend::Wayland;
        m_nativeDisplay = display;
        
        struct wl_registry* registry = wl_display_get_registry(display);
        // In a full implementation, you'd add listeners here to get the compositor
        // For this bridge, we assume the user/loader handles the protocol setup
        m_nativeWindow = nullptr; // This would be the wl_surface*
        m_isOpen = true;
        INGA_LOG(eINFO, "WINDOW", "Wayland backend initialized.");
    }
    // 2. Try XCB
    else 
    {
        int screenNum;
        xcb_connection_t* connection = xcb_connect(nullptr, &screenNum);
        if (!xcb_connection_has_error(connection))
        {
            m_backend = EWindowBackend::XCB;
            m_nativeDisplay = connection;
            
            const xcb_setup_t* setup = xcb_get_setup(connection);
            xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
            for (int i = 0; i < screenNum; ++i) xcb_screen_next(&iter);
            xcb_screen_t* screen = iter.data;

            xcb_window_t win = xcb_generate_id(connection);
            U32 mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
            U32 values[2] = { screen->white_pixel, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY };

            xcb_create_window(connection, XCB_COPY_FROM_PARENT, win, screen->root,
                0, 0, (U16)width, (U16)height, 0,
                XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);

            xcb_map_window(connection, win);
            xcb_flush(connection);

            m_nativeWindow = (void*)(uintptr_t)win;
            m_isOpen = true;
            INGA_LOG(eINFO, "WINDOW", "XCB backend initialized.");
        }
        // 3. Fallback to Xlib
        else
        {
            m_backend = EWindowBackend::X11;
            Display* dpy = XOpenDisplay(nullptr);
            if (!dpy) return false;

            m_nativeDisplay = dpy;
            int scr = DefaultScreen(dpy);
            ::Window win = XCreateSimpleWindow(dpy, RootWindow(dpy, scr), 0, 0, width, height, 0, 0, 0);
            
            XStoreName(dpy, win, title);
            XSelectInput(dpy, win, StructureNotifyMask | ExposureMask);
            XMapWindow(dpy, win);
            
            m_nativeWindow = (void*)(uintptr_t)win;
            m_isOpen = true;
            INGA_LOG(eINFO, "WINDOW", "Xlib fallback initialized.");
        }
    }
#endif
    return m_isOpen;
}

void Window::pollEvents()
{
#if defined(INGA_PLATFORM_WINDOWS)
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT) m_isOpen = false;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#elif defined(INGA_PLATFORM_LINUX)
    if (m_backend == EWindowBackend::X11)
    {
        Display* dpy = (Display*)m_nativeDisplay;
        while (XPending(dpy))
        {
            XEvent ev; XNextEvent(dpy, &ev);
            if (ev.type == DestroyNotify) m_isOpen = false;
        }
    }
    else if (m_backend == EWindowBackend::XCB)
    {
        xcb_connection_t* conn = (xcb_connection_t*)m_nativeDisplay;
        xcb_generic_event_t* event;
        while ((event = xcb_poll_for_event(conn)))
        {
            if ((event->response_type & ~0x80) == XCB_DESTROY_NOTIFY) m_isOpen = false;
            free(event);
        }
    }
    else if (m_backend == EWindowBackend::Wayland)
    {
        wl_display_dispatch_pending((struct wl_display*)m_nativeDisplay);
    }
#endif
}

void Window::close()
{
    if (!m_isOpen) return;

#if defined(INGA_PLATFORM_WINDOWS)
    if (m_nativeWindow) DestroyWindow((HWND)m_nativeWindow);
#elif defined(INGA_PLATFORM_LINUX)
    if (m_backend == EWindowBackend::X11)
    {
        XDestroyWindow((Display*)m_nativeDisplay, (::Window)(uintptr_t)m_nativeWindow);
        XCloseDisplay((Display*)m_nativeDisplay);
    }
    else if (m_backend == EWindowBackend::XCB)
    {
        xcb_destroy_window((xcb_connection_t*)m_nativeDisplay, (xcb_window_t)(uintptr_t)m_nativeWindow);
        xcb_disconnect((xcb_connection_t*)m_nativeDisplay);
    }
    else if (m_backend == EWindowBackend::Wayland)
    {
        wl_display_disconnect((struct wl_display*)m_nativeDisplay);
    }
#endif
    m_nativeWindow = nullptr;
    m_nativeDisplay = nullptr;
    m_isOpen = false;
}

} // namespace Ing
