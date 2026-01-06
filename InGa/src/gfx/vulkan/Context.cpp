#include <InGa/gfx/Context.h>
#include <InGa/gfx/vulkan/vk_types.h>
#include <InGa/core/log.h>
#include <algorithm>


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

CContext::~CContext()
{
}

bool CContext::initialize(CRenderDevice * pSharedGPU, const SContextConf& info)
{
    m_renderDevice = pSharedGPU;
    m_config = info;
    if (!m_window.create(m_config.width, m_config.height, m_config.windowTitle))
    {
        INGA_LOG(eFATAL, "CONTEXT", "Failed to create window.");
        return false;
    }

    if (!m_renderDevice->isInitialized())
    {
		if (!m_renderDevice->initialize("appname", true))
		{
        INGA_LOG(eFATAL, "DEMO", "Failed to initialize render device.");
        return false;
		}

    }

    if (!setupSwapchain(m_window))
    {
        INGA_LOG(eFATAL, "CONTEXT", "Failed to setup swapchain context.");
        return false;
    }

    this->initCommandResources();
    return true;
}

bool CContext::setupSwapchain(const Window& window)
{
    if (!m_renderDevice) return false;
    VkDevice device =  m_renderDevice->getDevice();
    
    // 1. Création de la Surface (Multi-backend)
    if (!createSurface(window, m_renderDevice->getInstance()))
    {
        return false;
    }

    // Your "Old Code" - Works everywhere without #ifdef
    VkBool32 supported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        m_renderDevice->getPhysicalDevice(),
        m_renderDevice->getPresentQueueFamily(),
        m_swapchain.m_surface,
        &supported
    );

    if (!supported)
    {
        INGA_LOG(eFATAL, "VULKAN", "GPU cannot present to this surface.");
        return false;
    }


    // 2. Configuration (On pourra automatiser la sélection du format plus tard)
    m_swapchain.m_format = VK_FORMAT_B8G8R8A8_SRGB;
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_renderDevice->getPhysicalDevice(), m_swapchain.m_surface, &capabilities);
    if (capabilities.currentExtent.width != 0xFFFFFFFF)
    {
        m_swapchain.m_extent =  capabilities.currentExtent;
    }
    else
    {
        // If we have freedom, we clamp our desired size (1280x720) to the allowed bounds
        m_swapchain.m_extent.width = std::clamp(window.getWidth(), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        m_swapchain.m_extent.height = std::clamp(window.getHeight(), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

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

bool CContext::createSurface(const Window& window, VkInstance instance)
{
  #if defined(INGA_PLATFORM_WINDOWS)
    if (window.getBackend() == EWindowBackend::Win32)
    {
        VkWin32SurfaceCreateInfoKHR sci = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        sci.hwnd = (HWND)window.getNativeWindow();
        sci.hinstance = (HINSTANCE)window.getHInstance();
        
        return vkCreateWin32SurfaceKHR(instance, &sci, nullptr, &m_swapchain.m_surface) == VK_SUCCESS;
    }

#elif defined(INGA_PLATFORM_LINUX)
    EWindowBackend backend = window.getBackend();

    if (backend == EWindowBackend::Wayland)
    {
        VkWaylandSurfaceCreateInfoKHR sci{};
        sci.sType =  VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        sci.display = (struct wl_display*)window.getNativeDisplay();
        sci.surface = (struct wl_surface*)window.getNativeWindow();
        return vkCreateWaylandSurfaceKHR(instance, &sci, nullptr, &m_swapchain.m_surface) == VK_SUCCESS;
    }
    else if (backend == EWindowBackend::X11)
    {
        VkXlibSurfaceCreateInfoKHR sci;
        sci.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        sci.dpy = (Display*)window.getNativeDisplay();
        sci.window = (::Window)(uintptr_t)window.getNativeWindow();
        return vkCreateXlibSurfaceKHR(instance, &sci, nullptr, &m_swapchain.m_surface) == VK_SUCCESS;
    }
    else if (backend == EWindowBackend::XCB)
    {
        VkXlibSurfaceCreateInfoKHR sci;
        sci.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        sci.dpy = (Display*)window.getNativeDisplay();
        sci.window = (::Window)(uintptr_t)window.getNativeWindow();
        return vkCreateXlibSurfaceKHR(instance, &sci, nullptr, &m_swapchain.m_surface) == VK_SUCCESS;
    }
#endif

    INGA_LOG(eFATAL, "VULKAN", "Unsupported window backend or platform.");
    return false;
}

void CContext::setupSyncObjects()
{
    U32 imageCount = static_cast<U32>(m_swapchain.m_images.size());
    m_swapchain.m_maxImageInFlight = (imageCount > 1) ? imageCount - 1 : 1;

    m_swapchain.m_renderFinishedSemaphores.resize(imageCount);
    m_swapchain.m_imagesInFlight.resize(imageCount);

    VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};

    for (U32 i = 0; i < imageCount; i++)
    {
        vkCreateSemaphore(m_renderDevice->getDevice(), &semInfo, nullptr, &m_swapchain.m_renderFinishedSemaphores[i]);
    }
}

void CContext::initCommandResources()
{
    m_cmdPoolGFX.queueIndex = m_renderDevice->getGraphicsQueueFamily();
    m_cmdPoolGFX.resetable = 1;

    VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.queueFamilyIndex = m_cmdPoolGFX.queueIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_renderDevice->getDevice(), &poolInfo, nullptr, &m_cmdPoolGFX.commandPool) != VK_SUCCESS)
    {
		INGA_LOG(eINFO, "VULKAN", "Vulkan Context cleaned up successfully.");
        return;
    }

    // 2. Initialize the Frames (2 flights)
    U32 flightCount = m_swapchain.m_maxImageInFlight;
    m_cmdFrames.resize(flightCount);

    for (U32 i = 0; i < flightCount; i++)
    {
        SCommandBufferFrame& frame = m_cmdFrames[i];

        VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.commandPool = m_cmdPoolGFX.commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(m_renderDevice->getDevice(), &allocInfo, &frame.cmd);

        VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_renderDevice->getDevice(), &fenceInfo, nullptr, &frame.inFlight);

        // GPU Semaphore
        VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        vkCreateSemaphore(m_renderDevice->getDevice(), &semInfo, nullptr, &frame.imageAvailable);
    }
}

void CContext::cleanup()
{
    if (!m_renderDevice) return;

    VkDevice logicalDevice = m_renderDevice->getDevice();

    vkDeviceWaitIdle(logicalDevice);
    for (U32 i = 0; i < m_cmdFrames.size(); i++)
    {
        SCommandBufferFrame& frame = m_cmdFrames[i];

        vkDestroyFence(m_renderDevice->getDevice(), frame.inFlight, nullptr);
        vkDestroySemaphore(m_renderDevice->getDevice(), frame.imageAvailable, nullptr);
        vkFreeCommandBuffers(m_renderDevice->getDevice(), m_cmdPoolGFX.commandPool, 1, &frame.cmd);
    }

    // 3. Destroy the Pool
    vkDestroyCommandPool(m_renderDevice->getDevice(), m_cmdPoolGFX.commandPool, nullptr);

    m_swapchain.cleanup(logicalDevice);

    // We destroy the surface here using the stored handle
    // But we DO NOT destroy m_instance here, as it belongs to CRenderDevice
    if (m_swapchain.m_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_renderDevice->getInstance(), m_swapchain.m_surface, nullptr);
        m_swapchain.m_surface = VK_NULL_HANDLE;
    }

    INGA_LOG(eINFO, "VULKAN", "Vulkan Context cleaned up successfully.");
}

} // namespace Inga
