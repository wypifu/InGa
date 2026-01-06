#include <InGa/gfx/vulkan/vk_types.h>

namespace Inga
{
void SSwapchain::cleanup(VkDevice device)
{
    if (device == VK_NULL_HANDLE) return;

    // 1. Détruire les ImageViews
    for (U32 i = 0; i < m_imageViews.size(); ++i)
    {
        vkDestroyImageView(device, m_imageViews[i], nullptr);
    }

    // 2. Détruire la synchronisation
    for (U32 i = 0; i < m_inFlightFences.size(); ++i)
    {
        vkDestroySemaphore(device, m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, m_inFlightFences[i], nullptr);
    }

    // 3. Détruire la swapchain elle-même
    if (m_handle != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, m_handle, nullptr);
    }
    
    // Note: La surface est généralement détruite à la toute fin par le Context
}
}
