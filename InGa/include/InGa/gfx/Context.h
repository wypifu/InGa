#ifndef INGA_CONTEXT_H
#define INGA_CONTEXT_H

#include "vulkan/vk_types.h"
#include <InGa/core/export.h>
#include <InGa/core/Window.h>
#include <InGa/gfx/RenderDevice.h>

namespace Inga
{
    struct SContextConf
    {
        const char* windowTitle;
        U32 width;
        U32 height;

        // Configurable Vulkan settings
        VkPresentModeKHR preferredPresentMode;
        U8               enableValidation;
        U8               useHighPerformanceGpu; // If you have Integrated + Dedicated
    };

    class INGA_API CContext
    {
    public:
        // On passe le RenderDevice au constructeur (Injection de dépendance)
        CContext() = default;
        ~CContext();

        bool setupSwapchain(const Window& window);
        bool initialize(CRenderDevice * pSharedGPU, const SContextConf& info);
        void update();
        void draw();
        void shutdown();
        void setupSyncObjects(); 
        void cleanup();
private:

    bool createSurface(const Window & window, VkInstance instance);
    void initCommandResources();

    private:
		Window m_window;
        SContextConf m_config;
        CRenderDevice* m_renderDevice = nullptr;
        SSwapchain m_swapchain;
        SCommandPool m_cmdPoolGFX;
        Vector<SCommandBufferFrame> m_cmdFrames;
    };
}

#endif
