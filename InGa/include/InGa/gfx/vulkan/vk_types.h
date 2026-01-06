#ifndef INGA_VK_TYPE_H 
#define INGA_VK_TYPE_H

#include <InGa/core/inga_platform.h>
#include <InGa/core/container.h>

#if defined(INGA_PLATFORM_LINUX)
    #ifndef VK_USE_PLATFORM_XLIB_KHR
        #define VK_USE_PLATFORM_XLIB_KHR
    #endif
    #ifndef VK_USE_PLATFORM_XCB_KHR
        #define VK_USE_PLATFORM_XCB_KHR
    #endif
    #ifndef VK_USE_PLATFORM_WAYLAND_KHR
        #define VK_USE_PLATFORM_WAYLAND_KHR
    #endif
#elif defined(INGA_PLATFORM_WINDOWS)
    #ifndef VK_USE_PLATFORM_WIN32_KHR
        #define VK_USE_PLATFORM_WIN32_KHR
    #endif
#endif
#if defined(INGA_PLATFORM_LINUX)
    #include <X11/Xlib.h>
    #include <xcb/xcb.h>
    #include <wayland-client.h>
#elif defined(INGA_PLATFORM_WINDOWS)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#include <vulkan/vulkan.h>

namespace Inga
{
struct SSwapchain
{
  VkSurfaceKHR   m_surface = VK_NULL_HANDLE;
  VkSwapchainKHR m_handle = VK_NULL_HANDLE;

  VkFormat       m_format = VK_FORMAT_UNDEFINED;
  VkExtent2D     m_extent = {0, 0};

  // On utilise ici ton conteneur maison qui respecte ton allocateur
  Vector<VkImage>     m_images;
  Vector<VkImageView> m_imageViews;

  // Synchronisation par frame
  Vector<VkSemaphore> m_renderFinishedSemaphores;

  // Suivi des images utilisées par les frames en vol
  // On utilise des pointeurs ou des index pour lier les Fences aux images
  Vector<VkFence>     m_imagesInFlight; 
  U32 m_currentFrame =  0;
  U32 m_maxImageInFlight = 2;

  void cleanup(VkDevice device);
};

struct SCommandPool
{
    VkCommandPool 		commandPool;
    VkCommandPoolCreateFlags    flags;
    const void * pNext;
    U32  queueIndex;
    U8 resetable;
};

struct SCommandBufferFrame
{
    VkCommandBuffer cmd;
    VkFence         inFlight;
    VkSemaphore     imageAvailable;
};

struct SCommanBuffer
{
    SCommandPool* pool;
    VkCommandBuffer cmd;
    VkFence         inFlight;
};

}

#endif
