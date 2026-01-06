#ifndef INGA_RENDER_DEVICE_H
#define INGA_RENDER_DEVICE_H

#include <InGa/core/export.h>
#include <InGa/core/inga_platform.h>
#include <InGa/gfx/vulkan/vk_types.h>

namespace Inga
{
    class INGA_API CRenderDevice
    {
    public:
        CRenderDevice();
        ~CRenderDevice();

        bool initialize(const char* appName, bool enableValidation);
        void shutdown();

        bool findPresentQueue(VkSurfaceKHR surface);

        // Getters
        VkDevice getLogicalDevice() const { return m_logicalDevice; }
        VkInstance getInstance() const { return m_instance; }
        VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
        
        inline U32 getGraphicsQueueFamily() const { return m_graphicsQueueFamily; }
        inline U32 getPresentQueueFamily()  const { return m_presentQueueFamily; }
        inline U32 getComputeQueueFamily()  const { return m_computeQueueFamily; }
        inline U32 getTransferQueueFamily() const { return m_transferQueueFamily; }

    private:
        bool createInstance(const char* appName, bool enableValidation);
        bool pickPhysicalDevice();
        bool createLogicalDevice();

        bool checkDeviceQueueSupport(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        I32  rateDeviceSuitability(VkPhysicalDevice device);

    private:
        VkInstance       m_instance       = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice         m_logicalDevice  = VK_NULL_HANDLE;

        U32 m_graphicsQueueFamily = 0xFFFFFFFF;
        U32 m_presentQueueFamily  = 0xFFFFFFFF;
        U32 m_computeQueueFamily  = 0xFFFFFFFF;
        U32 m_transferQueueFamily = 0xFFFFFFFF;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;
        VkQueue m_computeQueue  = VK_NULL_HANDLE;
        VkQueue m_transferQueue = VK_NULL_HANDLE;

        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
        bool m_enableValidation;
    };
}
#endif
