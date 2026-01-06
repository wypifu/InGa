#ifndef INGA_CONTEXT_H
#define INGA_CONTEXT_H

#include "vulkan/vk_types.h"
#include <InGa/core/export.h>
#include <InGa/core/Window.h>
#include <InGa/gfx/RenderDevice.h>

namespace Inga
{
    struct SContextInfo; // À définir selon tes besoins (taille fenêtre, titre, etc.)

    class INGA_API CContext
    {
    public:
        // On passe le RenderDevice au constructeur (Injection de dépendance)
        CContext(CRenderDevice* device);
        ~CContext();

        bool setupSwapchain(const Window& window);
        bool initialize(const SContextInfo& info);
        void update();
        void draw();
        void shutdown();
        void setupSyncObjects(); 
        void cleanup();
private:

    bool createSurface(const Window & window, VkInstance instance);

    private:
        CRenderDevice* m_renderDevice = nullptr;
        SSwapchain m_swapchain;
    };
}

#endif
