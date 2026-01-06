#include <InGa/gfx/Context.h>
#include <InGa/gfx/vulkan/vk_types.h>
#include <InGa/core/log.h>


#if defined(INGA_PLATFORM_LINUX)
    // 1. Include Native System Headers First
    #include <X11/Xlib.h>          // Defines 'Display', 'Window'
    #include <xcb/xcb.h>           // Defines 'xcb_connection_t', 'xcb_window_t'
    #include <wayland-client.h>    // Defines 'wl_display', 'wl_surface'

    // 2. Include Vulkan Platform Headers Second
    #include <vulkan/vulkan_xlib.h>
    #include <vulkan/vulkan_xcb.h>
    #include <vulkan/vulkan_wayland.h>

#elif defined(INGA_PLATFORM_WINDOWS)
    #define VK_USE_PLATFORM_WIN32_KHR
    #include <windows.h>           // Defines 'HWND', 'HINSTANCE'
    #include <vulkan/vulkan_win32.h>
#endif


namespace Inga
{

CContext::CContext(CRenderDevice * device)
{
  m_renderDevice = device;
}

CContext::~CContext()
{
}

bool CContext::setupSwapchain(const Window& window, CRenderDevice * cdevice)
{
    m_renderDevice = cdevice;
    VkDevice device =  cdevice->getLogicalDevice();
    
    // 1. Création de la Surface (Multi-backend)
    if (!createSurface(window))
    {
        return false;
    }

    // 2. Configuration (On pourra automatiser la sélection du format plus tard)
    m_swapchain.m_format = VK_FORMAT_B8G8R8A8_SRGB;
    m_swapchain.m_extent = { window.getWidth(), window.getHeight() };

  VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_swapchain.m_surface;
    createInfo.minImageCount = 3; 
    createInfo.imageFormat = m_swapchain.m_format;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = m_swapchain.m_extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    U32 graphicsFamily = m_renderDevice->getGraphicsQueueFamily();
    U32 presentFamily  = m_renderDevice->getPresentQueueFamily();
    U32 queueFamilyIndices[] = { graphicsFamily, presentFamily };
    
    if (queueFamilyIndices[0] != queueFamilyIndices[1])
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; 
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_swapchain.m_handle) != VK_SUCCESS)
    {
        //INGA_LOG_ERROR("Failed to create Swapchain");
        return false;
    }

    // 4. Images et Views
    U32 imageCount = 0;
    vkGetSwapchainImagesKHR(device, m_swapchain.m_handle, &imageCount, nullptr);
    m_swapchain.m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, m_swapchain.m_handle, &imageCount, m_swapchain.m_images.data());

    m_swapchain.m_imageViews.resize(imageCount);
    for (U32 i = 0; i < imageCount; i++)
    {
      VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_swapchain.m_images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_swapchain.m_format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(device, &viewInfo, nullptr, &m_swapchain.m_imageViews[i]);
    }

    // 5. Synchronisation
    setupSyncObjects();

    return true;
}

bool CRenderDevice::findPresentQueue(VkSurfaceKHR surface)
{
  VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, m_graphicsQueueFamily, surface, &presentSupport);

    if (presentSupport)
    {
        m_presentQueueFamily = m_graphicsQueueFamily;
    }
    else
    {
        // If graphics queue doesn't support present, search other families
        U32 count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, nullptr);
        
        for (U32 i = 0; i < count; i++)
        {
            vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, surface, &presentSupport);
            if (presentSupport)
            {
                m_presentQueueFamily = i;
                break;
            }
        }
    }
    if (m_presentQueueFamily == 0xFFFFFFFF)
    {
        INGA_LOG(eERROR, "VULKAN", "No queue family supports presentation to this surface");
        return false;
    }

    vkGetDeviceQueue(m_logicalDevice, m_presentQueueFamily, 0, &m_presentQueue);

    return true;
}

bool CContext::createSurface(const Window& window)
{
  #if defined(INGA_PLATFORM_WINDOWS)
    if (window.getBackend() == EWindowBackend::Win32)
    {
        VkWin32SurfaceCreateInfoKHR sci = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        sci.hwnd = (HWND)window.getNativeWindow();
        sci.hinstance = (HINSTANCE)window.getHInstance();
        
        return vkCreateWin32SurfaceKHR(m_instance, &sci, nullptr, &m_swapchain.m_surface) == VK_SUCCESS;
    }

#elif defined(INGA_PLATFORM_LINUX)
    EWindowBackend backend = window.getBackend();

    if (backend == EWindowBackend::Wayland)
    {
        VkWaylandSurfaceCreateInfoKHR sci{};
        sci.sType =  VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        sci.display = (struct wl_display*)window.getNativeDisplay();
        sci.surface = (struct wl_surface*)window.getNativeWindow();
        return vkCreateWaylandSurfaceKHR(m_instance, &sci, nullptr, &m_swapchain.m_surface) == VK_SUCCESS;
    }
    else if (backend == EWindowBackend::X11)
    {
        VkXlibSurfaceCreateInfoKHR sci;
        sci.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        sci.dpy = (Display*)window.getNativeDisplay();
        sci.window = (::Window)(uintptr_t)window.getNativeWindow();
        return vkCreateXlibSurfaceKHR(m_instance, &sci, nullptr, &m_swapchain.m_surface) == VK_SUCCESS;
    }
    else if (backend == EWindowBackend::XCB)
    {
        VkXlibSurfaceCreateInfoKHR sci;
        sci.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        sci.dpy = (Display*)window.getNativeDisplay();
        sci.window = (::Window)(uintptr_t)window.getNativeWindow();
        return vkCreateXlibSurfaceKHR(m_instance, &sci, nullptr, &m_swapchain.m_surface) == VK_SUCCESS;
    }
#endif

    INGA_LOG(eFATAL, "VULKAN", "Unsupported window backend or platform.");
    return false;
}

void CContext::setupSyncObjects()
{
    U32 imageCount = m_swapchain.m_images.size();
    // Ta logique : N-1 images en vol
    U32 maxFramesInFlight = (imageCount > 1) ? (imageCount - 1) : 1;

    m_swapchain.m_imageAvailableSemaphores.resize(maxFramesInFlight);
    m_swapchain.m_renderFinishedSemaphores.resize(maxFramesInFlight);
    m_swapchain.m_inFlightFences.resize(maxFramesInFlight);

    VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};
    VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO , nullptr, 0};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (U32 i = 0; i < maxFramesInFlight; i++)
    {
        vkCreateSemaphore(m_renderDevice->getLogicalDevice(), &semInfo, nullptr, &m_swapchain.m_imageAvailableSemaphores[i]);
        vkCreateSemaphore(m_renderDevice->getLogicalDevice(), &semInfo, nullptr, &m_swapchain.m_renderFinishedSemaphores[i]);
        vkCreateFence(m_renderDevice->getLogicalDevice(), &fenceInfo, nullptr, &m_swapchain.m_inFlightFences[i]);
    }
}

void CContext::cleanup()
{
    if (!m_renderDevice) return;

    VkDevice logicalDevice = m_renderDevice->getLogicalDevice();

    vkDeviceWaitIdle(logicalDevice);

    m_swapchain.cleanup(logicalDevice);

    for (U32 i = 0; i < m_swapchain.m_imageAvailableSemaphores.size(); i++)
    {
        vkDestroySemaphore(logicalDevice, m_swapchain.m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(logicalDevice, m_swapchain.m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(logicalDevice, m_swapchain.m_inFlightFences[i], nullptr);
    }

    // We destroy the surface here using the stored handle
    // But we DO NOT destroy m_instance here, as it belongs to CRenderDevice
    if (m_swapchain.m_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_instance, m_swapchain.m_surface, nullptr);
        m_swapchain.m_surface = VK_NULL_HANDLE;
    }

    INGA_LOG(eINFO, "VULKAN", "Vulkan Context cleaned up successfully.");
}

} // namespace Inga
