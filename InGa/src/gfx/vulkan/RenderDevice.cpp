#include <InGa/gfx/RenderDevice.h>
#include <InGa/core/log.h>
#include <InGa/core/container.h>
#include <cstring>

namespace Inga
{
    CRenderDevice::CRenderDevice() 
    {
        // Initialize handles to null
        m_instance = VK_NULL_HANDLE;
    }

    CRenderDevice::~CRenderDevice() 
    {
        if (m_instance != VK_NULL_HANDLE)
        {
            // If the user forgot to call shutdown(), we do it here.
            // Note: We can't log as safely here if the logger is already destroyed.
            shutdown();
        }
    }

  bool CRenderDevice::initialize(const char* appName, bool enableValidation)
    {

    if (!createInstance(appName, enableValidation))
        {
            INGA_LOG(eFATAL, "VULKAN", "Failed to create Vulkan instance.");
            return false;
        }

        // 2. Select the GPU
        if (!pickPhysicalDevice())
        {
            INGA_LOG(eFATAL, "VULKAN", "Failed to find a suitable GPU.");
            return false;
        }

        if (!createLogicalDevice())
        {
            INGA_LOG(eFATAL, "VULKAN", "Failed to create shared Logical Device.");
            return false;
        }

        INGA_LOG(eINFO, "VULKAN", "RenderDevice initialized successfully.");
        m_isInitialized = 1;
        return true;
    }

void CRenderDevice::shutdown()
{
	if (m_logicalDevice != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(m_logicalDevice);
        vkDestroyDevice(m_logicalDevice, nullptr);
        m_logicalDevice = VK_NULL_HANDLE;
	}
	if (m_instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
	}
}

bool CRenderDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        U32 extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        Vector<VkExtensionProperties> availableExtensions;
        availableExtensions.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // We specifically need the Swapchain extension
        bool swapchainSupported = false;
        for (const auto& ext : availableExtensions)
        {
            if (strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            {
                swapchainSupported = true;
                break;
            }
        }
        return swapchainSupported;
    }

bool CRenderDevice::createLogicalDevice()
{
  if (!checkDeviceQueueSupport(m_physicalDevice))
  {
      INGA_LOG(eFATAL, "VULKAN", "Physical device no longer supports required queues!");
      return false;
  }

  m_presentQueueFamily = 0xFFFFFFFF;

#if defined(INGA_PLATFORM_WINDOWS)
  // WINDOWS SPECIFIC: Check support without needing a VkSurfaceKHR
  if (vkGetPhysicalDeviceWin32PresentationSupportKHR(m_physicalDevice, m_graphicsQueueFamily))
  {
      m_presentQueueFamily = m_graphicsQueueFamily;
  }
#elif defined(INGA_PLATFORM_LINUX)
  // LINUX: We can't easily check without a surface handle (X11 Display or Wayland handle).
  // We assume the GFX queue is the one, and validate it LATER in CContext.
  m_presentQueueFamily = m_graphicsQueueFamily;
#endif

  U32 uniqueFamilies[4] = { m_graphicsQueueFamily, m_computeQueueFamily, m_transferQueueFamily, m_presentQueueFamily};
  U32 uniqueCount = 0;

  // Petit tri manuel pour identifier les indices uniques
  for(U32 i = 0; i < 4; ++i)
  {
    bool alreadyExists = false;
    for(U32 j = 0; j < uniqueCount; ++j)
    {
      if(uniqueFamilies[i] == uniqueFamilies[j]) { alreadyExists = true; break; }
    }
    if(!alreadyExists) uniqueFamilies[uniqueCount++] = uniqueFamilies[i];
  }

  Vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  queueCreateInfos.reserve(uniqueCount);
  F32 queuePriority = 1.0f;

  for (U32 i = 0; i < uniqueCount; ++i) 
  {
    VkDeviceQueueCreateInfo qInfo{};
    qInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qInfo.queueFamilyIndex = uniqueFamilies[i];
    qInfo.queueCount = 1;
    qInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(qInfo);
  }

  // 3. Activation des Features modernes (Vulkan 1.2+)
  // 3.1. Initialize all to zero first
  VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
  VkPhysicalDeviceBufferDeviceAddressFeatures bdaFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
  VkPhysicalDeviceFeatures2 deviceFeatures2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

  //3.2 set feature
  dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
  bdaFeatures.bufferDeviceAddress = VK_TRUE;

  // 3.3. Link the chain (Top -> Down)
  deviceFeatures2.pNext = &bdaFeatures;
  bdaFeatures.pNext = &dynamicRenderingFeatures;
  dynamicRenderingFeatures.pNext = nullptr; // Explicitly terminate

  // 4. Extensions (InGa::Vector utilisé ici)
  Vector<const char*> deviceExtensions;
  deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  deviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pNext = &deviceFeatures2;
  createInfo.queueCreateInfoCount = static_cast<U32>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.enabledExtensionCount = static_cast<U32>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();


  Vector<const char*> validationLayers;
  if (m_enableValidation)
  {
      validationLayers.push_back("VK_LAYER_KHRONOS_validation");
      createInfo.enabledLayerCount = static_cast<U32>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
  }
  else
  {
      createInfo.enabledLayerCount = 0;
  }
  
  if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice) != VK_SUCCESS) 
  {
    INGA_LOG(eFATAL, "VULKAN", "Failed to create logical device!");
    return false;
  }

  vkGetDeviceQueue(m_logicalDevice, m_graphicsQueueFamily, 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_logicalDevice, m_computeQueueFamily,  0, &m_computeQueue);
  vkGetDeviceQueue(m_logicalDevice, m_transferQueueFamily, 0, &m_transferQueue);
  vkGetDeviceQueue(m_logicalDevice, m_presentQueueFamily, 0, &m_presentQueue);

  INGA_LOG(eINFO, "VULKAN", "Logical Device Ready. G:%d C:%d T:%d", 
           m_graphicsQueueFamily, m_computeQueueFamily, m_transferQueueFamily);

  return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]]VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]]void* pUserData) 
{
    LogLevel level = eINFO;
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) level = eWARNING;
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   level = eFATAL;

    INGA_LOG(level, "VULKAN_VALIDATION", "%s", pCallbackData->pMessage);
    
    return VK_FALSE; // Toujours retourner False
}

bool CRenderDevice::createInstance(const char* app_name, bool enable_validation)
{
  // 1. Application Info
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "InGa Engine";
    app_info.engineVersion = VK_MAKE_VERSION(2026, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_3; // Targeting 1.3 for modern features

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    // 2. Extension Handling
    Vector<const char*> extensions;
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if defined(INGA_PLATFORM_WINDOWS)
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(INGA_PLATFORM_LINUX)
    U32 available_ext_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &available_ext_count, nullptr);
    Vector<VkExtensionProperties> available_exts;
    available_exts.resize(available_ext_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &available_ext_count, available_exts.data());

    auto has_ext = [&](const char* name) -> bool 
    {
        for (U32 i = 0; i < available_ext_count; ++i) 
        {
            if (strcmp(available_exts[i].extensionName, name) == 0) return true;
        }
        return false;
    };

    // Support all possible Linux backends
    if (has_ext(VK_KHR_XLIB_SURFACE_EXTENSION_NAME))    extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
    if (has_ext(VK_KHR_XCB_SURFACE_EXTENSION_NAME))     extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
    if (has_ext(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME)) extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif

    if (enable_validation)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    create_info.enabledExtensionCount = (U32)extensions.size();
    create_info.ppEnabledExtensionNames = extensions.data();

    // 3. Validation Layers
    Vector<const char*> layers;
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};

    if (enable_validation)
    {
        const char* validation_layer_name = "VK_LAYER_KHRONOS_validation";
        
        // Optional: Check if the layer is actually available on the system
        U32 layer_count = 0;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        Vector<VkLayerProperties> available_layers;
        available_layers.resize(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        bool layer_found = false;
        for (U32 i = 0; i < layer_count; i++)
        {
            if (strcmp(available_layers[i].layerName, validation_layer_name) == 0)
            {
                layer_found = true;
                break;
            }
        }

        if (layer_found)
        {
            layers.push_back(validation_layer_name);
            create_info.enabledLayerCount = (U32)layers.size();
            create_info.ppEnabledLayerNames = layers.data();

            // Setup temporary messenger for instance creation/destruction
            debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debug_create_info.pfnUserCallback = debugCallback; 
            create_info.pNext = &debug_create_info;
        }
    }

    INGA_LOG(eDEBUG, "VULKAN", "attempting vkCreateInstance");
    // 4. Final Creation
    VkResult result = vkCreateInstance(&create_info, nullptr, &m_instance);
    if (result != VK_SUCCESS)
    {
		INGA_LOG(eERROR, "VULKAN", "vkCreateInstance FAILED with VkResult: %d", result);
        return false;
    }

		INGA_LOG(eDEBUG, "VULKAN", "vkCreateInstance SUCCESS. m_instance vlaue : %p", (void*) m_instance);
    return true;
}

bool CRenderDevice::pickPhysicalDevice()
{
    U32 device_count = 0;
    vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

    if (device_count == 0)
    {
        INGA_LOG(eFATAL, "VULKAN", "Failed to find GPUs with Vulkan support!");
        return false;
    }

    Vector<VkPhysicalDevice> devices;
    devices.resize(device_count);
    vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

    VkPhysicalDevice best_device = VK_NULL_HANDLE;
    I32 best_score = -1;

    for (U32 i = 0; i < device_count; ++i)
    {
        I32 score = rateDeviceSuitability(devices[i]);
        if (score > best_score)
        {
            best_score = score;
            best_device = devices[i];
        }
    }

    if (best_device == VK_NULL_HANDLE || best_score < 0)
    {
        INGA_LOG(eFATAL, "VULKAN", "Failed to find a suitable GPU for InGa!");
        return false;
    }

    m_physicalDevice = best_device;

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
    INGA_LOG(eINFO, "VULKAN", "Selected GPU: %s (Score: %d)", props.deviceName, best_score);

    return true;
}

bool CRenderDevice::checkDeviceQueueSupport(VkPhysicalDevice device)
{
    U32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    Vector<VkQueueFamilyProperties> queue_families;
    queue_families.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    // On initialise avec des valeurs invalides
    m_graphicsQueueFamily = 0xFFFFFFFF;
    m_computeQueueFamily  = 0xFFFFFFFF;
    m_transferQueueFamily = 0xFFFFFFFF;

    // 1. Recherche de la Queue Graphics (souvent prioritaire)
    for (U32 i = 0; i < queue_family_count; ++i)
    {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            m_graphicsQueueFamily = i;
            break; 
        }
    }

    // 2. Recherche d'une Compute Queue DÉDIÉE (Async Compute)
    for (U32 i = 0; i < queue_family_count; ++i)
    {
        if ((queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && 
            !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            m_computeQueueFamily = i;
            break;
        }
    }

    // 3. Recherche d'une Transfer Queue DÉDIÉE (Async Transfer / DMA)
    for (U32 i = 0; i < queue_family_count; ++i)
    {
        if ((queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && 
            !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && 
            !(queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            m_transferQueueFamily = i;
            break;
        }
    }

    // Fallback : Si on n'a pas trouvé de queues dédiées, on partage
    if (m_computeQueueFamily == 0xFFFFFFFF) m_computeQueueFamily = m_graphicsQueueFamily;
    if (m_transferQueueFamily == 0xFFFFFFFF) m_transferQueueFamily = m_computeQueueFamily;

    // Le GPU est valide si on a au moins une queue Graphics
    return (m_graphicsQueueFamily != 0xFFFFFFFF);

}


I32 CRenderDevice::rateDeviceSuitability(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device, &props);
    vkGetPhysicalDeviceFeatures(device, &features);

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_features = {};
    dynamic_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    VkPhysicalDeviceBufferDeviceAddressFeatures bda_features {};
    bda_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bda_features.pNext = &dynamic_features;
    
    VkPhysicalDeviceFeatures2 device_features{};
    device_features.sType =  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    device_features.pNext = &bda_features;

    vkGetPhysicalDeviceFeatures2(device, &device_features);
    
    // Pas de Buffer Device Address = Pas de 3ème voie (Arena/Offset Allocator)
    if (!bda_features.bufferDeviceAddress)
    {
        INGA_LOG(eWARNING, "VULKAN", "GPU rejected: Missing Buffer Device Address support.");
        return -1;
    }

    // Pas de Dynamic Rendering = Pas de RenderGraph moderne
    if (!dynamic_features.dynamicRendering)
    {
        INGA_LOG(eWARNING, "VULKAN", "GPU rejected: Missing Dynamic Rendering support.");
        return -1;
    }

    I32 score = 0;

    // 1. Privilégier les GPU Discrets (Performance)
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += 100;
    }

    // 2. Vérification des Queues indispensables
    if (!checkDeviceQueueSupport(device))
    {
        return -1; // Éliminatoire
    }

    // 3. Vérification des extensions requises (Swapchain)
    if (!checkDeviceExtensionSupport(device))
    {
        return -1; // Éliminatoire
    }

    // 4. Features indispensables pour la "3ème voie" et RenderGraph
    // On pourrait vérifier ici le support de bufferDeviceAddress
    
    // Bonus pour la mémoire VRAM
    score += (I32)(props.limits.maxImageDimension2D / 1000);

    if (!checkDeviceQueueSupport(device)) 
    {
        return -1; 
    }

    return score;
}

}
