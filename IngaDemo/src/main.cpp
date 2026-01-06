#include <InGa/core/Window.h>
#include <InGa/gfx/RenderDevice.h>
#include <InGa/gfx/Context.h>
#include <InGa/core/log.h>
#include <InGa/InGa.h>

using namespace Inga;

I32 main(I32 argc, char** argv)
{
    // 0. START THE ENGINE CORE
    // 64 groups, 16MB default page size
    Inga::EngineConfig conf = Inga::getDefaultEngineConfig();
    INGA_BEGIN(conf)

    INGA_LOG(eINFO, "CORE", "InGa Engine Core Started.");
    // 1. Create the OS Window
    // This is where m_hInstance (Win32) or Display* (Linux) are captured
    Inga::Window window;
    if (!window.create(1280, 720, "InGa Engine - Startup/Shutdown Demo"))
    {
        INGA_LOG(eFATAL, "DEMO", "Failed to create window.");
        return -1;
    }

    // 2. Initialize the Render Device
    // Creates the VkInstance and picks the Physical Device
    CRenderDevice device;
    if (!device.initialize("IngaDemo", true /* enable validation */))
    {
        INGA_LOG(eFATAL, "DEMO", "Failed to initialize render device.");
        return -1;
    }

    // 3. Setup the Context
    // Creates Surface, Logical Device, and Swapchain
    CContext context(&device);
    if (!context.setupSwapchain(window))
    {
        INGA_LOG(eFATAL, "DEMO", "Failed to setup swapchain context.");
        return -1;
    }

    INGA_LOG(eINFO, "DEMO", "Inga initialized successfully. Closing in 3 seconds...");

    // 4. Simple Wait Loop
    // We poll events to keep the window responsive during the wait
    U32 frameCount = 0;
    while (window.isOpen()) // Approx 3 seconds at 100fps
    {
        window.pollEvents();

        // No rendering yet, just keeping the window alive

        frameCount++;
        // Small sleep could go here to avoid 100% CPU usage
    }

    // 5. Shutdown Sequence (REVERSE ORDER)
    INGA_LOG(eINFO, "DEMO", "Beginning shutdown...");

    // Wait for GPU to be idle before destroying anything
    vkDeviceWaitIdle(device.getLogicalDevice());

    // Destroy Swapchain & Surface
    context.cleanup();

    // Destroy Logical Device & Instance
    device.shutdown();

    // Destroy OS Window
    window.close();

    INGA_LOG(eINFO, "DEMO", "Shutdown complete. Goodbye.");
    INGA_END()
    return 0;
}
