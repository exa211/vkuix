#define VMA_IMPLEMENTATION
#include "vulkan_backend.h"

#include <ranges>
#include <set>

void VkBackend::setupInstance(Instance &instance, const std::vector<const char *> &extensions) {
  LOG(D, "Creating VkInstance.");
  // Application info and api version of Vulkan
  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.apiVersion = VK_API_VERSION_1_3;
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.pEngineName = "vkuix-engine";

  // DEBUG CALLBACK
  VkDebugUtilsMessengerCreateInfoEXT debugInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  debugInfo.pfnUserCallback = debugCallback;
  debugInfo.pUserData = nullptr; // Maybe needed in the future

  VkInstanceCreateInfo instanceCreateInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  instanceCreateInfo.pApplicationInfo = &appInfo;
  instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
  instanceCreateInfo.enabledExtensionCount = extensions.size();
  instanceCreateInfo.ppEnabledLayerNames = VAL_LAYERS.data();
  instanceCreateInfo.enabledLayerCount = VAL_LAYERS.size();
  instanceCreateInfo.pNext = &debugInfo; // Custom debugger

  // Create VkInstance with custom debugger
  if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance.vkInstance) != VK_SUCCESS) {
    LOG(F, "Could not create VkInstance.");
  }
  LOG(S, "Created VkInstance.");
}

void VkBackend::setupSurface(Instance &instance, GLFWwindow *pWin) {
  // Get window render surface
  if (glfwCreateWindowSurface(instance.vkInstance, pWin, nullptr, &instance.surface) != VK_SUCCESS) {
    LOG(F, "Could not create window surface using glfw.");
  }
}

void VkBackend::setupDevices(Instance &instance) {
  // Choose gpu
  u32 deviceCount = 0;
  vkEnumeratePhysicalDevices(instance.vkInstance, &deviceCount, nullptr);
  std::vector<VkPhysicalDevice> physDevices{deviceCount};
  vkEnumeratePhysicalDevices(instance.vkInstance, &deviceCount, physDevices.data());
  if (deviceCount == 0)
    LOG(F, "Found no physical rendering device (GPU)");
  instance.physDevice = physDevices.at(0);

  setupQueues(instance);

  // Device Queues
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  const std::set uQueueFamilies = {instance.queueFamilies.graphicsFamily.value(), instance.queueFamilies.presentFamily.value()};
  constexpr float queuePrio = 1;
  for (const u32 queueFamily: uQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePrio;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  // DEVICE EXTENSIONS
  const std::vector deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  // Logical Device
  VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceInfo.enabledExtensionCount = deviceExtensions.size();
  deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
  deviceInfo.queueCreateInfoCount = queueCreateInfos.size();
  deviceInfo.pQueueCreateInfos = queueCreateInfos.data();

  if (vkCreateDevice(instance.physDevice, &deviceInfo, nullptr, &instance.device) != VK_SUCCESS) {
    LOG(F, "Could not create VkDevice.");
  }
}

void VkBackend::setupVMA(Instance &instance) {
  VmaAllocatorCreateInfo allocCreateInfo{};
  allocCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
  allocCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
  allocCreateInfo.physicalDevice = instance.physDevice;
  allocCreateInfo.device = instance.device;
  allocCreateInfo.instance = instance.vkInstance;
  if (vmaCreateAllocator(&allocCreateInfo, &instance.allocator) != VK_SUCCESS) {
    LOG(F, "Could not create VmaAllocator.");
  }
}

void VkBackend::setupQueues(Instance &instance) {
  u32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(instance.physDevice, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(instance.physDevice, &queueFamilyCount, queueFamilies.data());

  int i = 0;
  for (const VkQueueFamilyProperties &qf: queueFamilies) {
    if (qf.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(instance.physDevice, i, instance.surface, &presentSupport);
      if (presentSupport)
        instance.queueFamilies.presentFamily = i;
      instance.queueFamilies.graphicsFamily = i;
      if (instance.queueFamilies.presentFamily && instance.queueFamilies.graphicsFamily)
        break;
    }
    ++i;
  }
}

void VkBackend::setupSwapchain(const Window &window, Instance &instance, const bool resize) {
  // If resize, free resources from old swapchain first.
  if (resize) {
    for (const auto image: instance.swapchain.images) {
      vkDestroyImageView(instance.device, image.view, nullptr);
      vmaDestroyImage(instance.allocator, image.vkImage, image.alloc);
    }
    vkDestroySwapchainKHR(instance.device, instance.swapchain.swapchain, nullptr);
  }

  VkSurfaceCapabilitiesKHR caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(instance.physDevice, instance.surface, &caps);

  // Query available surface formats on gpu.
  u32 formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(instance.physDevice, instance.surface, &formatCount, nullptr);
  if (formatCount == 0)
    LOG(F, "Physical Device does not support any surface formats.");

  std::vector<VkSurfaceFormatKHR> formats{formatCount};
  vkGetPhysicalDeviceSurfaceFormatsKHR(instance.physDevice, instance.surface, &formatCount, formats.data());

  for (const VkSurfaceFormatKHR &format: formats) {
    if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      instance.swapchain.surfaceFormat = format;
      break;
    }
  }

  instance.swapchain.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  instance.swapchain.extent = window.getExtent();

  u32 imageCount = caps.minImageCount;
  if (caps.maxImageCount > 0 && caps.minImageCount > caps.maxImageCount)
    imageCount = caps.maxImageCount;

  VkSwapchainCreateInfoKHR swapchainCreateInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchainCreateInfo.surface = instance.surface;
  swapchainCreateInfo.imageFormat = instance.swapchain.surfaceFormat.format;
  swapchainCreateInfo.imageColorSpace = instance.swapchain.surfaceFormat.colorSpace;
  swapchainCreateInfo.imageExtent = instance.swapchain.extent;
  swapchainCreateInfo.minImageCount = imageCount;
  swapchainCreateInfo.imageArrayLayers = 1;
  swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyInfo &queueInfo = instance.queueFamilies;

  std::vector queueIndices = {queueInfo.graphicsFamily.value(), queueInfo.presentFamily.value()};

  if (queueInfo.graphicsFamily != queueInfo.presentFamily) {
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchainCreateInfo.queueFamilyIndexCount = queueIndices.size();
    swapchainCreateInfo.pQueueFamilyIndices = queueIndices.data();
  } else {
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  swapchainCreateInfo.preTransform = caps.currentTransform;
  swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainCreateInfo.presentMode = instance.swapchain.presentMode;
  swapchainCreateInfo.clipped = VK_TRUE;

  // This can later be changed if we want to use the old swapchain to create a new one.
  // Vulkan uses this mainly for performance reasons because swapchain creation can be expensive in certain cases.
  swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(instance.device, &swapchainCreateInfo, nullptr, &instance.swapchain.swapchain) != VK_SUCCESS) {
    LOG(F, "Could not create VkSwapchain.");
  }

  vkGetSwapchainImagesKHR(instance.device, instance.swapchain.swapchain, &imageCount, nullptr);
  instance.swapchain.images.resize(imageCount);
  std::vector<VkImage> tempSwapchainImages(imageCount);
  vkGetSwapchainImagesKHR(instance.device, instance.swapchain.swapchain, &imageCount, tempSwapchainImages.data());

  for (int i = 0; i < imageCount; ++i) {
    instance.swapchain.images[i].vkImage = tempSwapchainImages[i]; // Copy assignment

    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = instance.swapchain.images[i].vkImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = instance.swapchain.surfaceFormat.format;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(instance.device, &viewInfo, nullptr, &instance.swapchain.images[i].view) != VK_SUCCESS) {
      LOG(F, "Could not create the corresponding VkImageView for the swapchain VkImage.");
    }
  }

  LOG(I, "Using Format: " + std::to_string(instance.swapchain.surfaceFormat.format)
                                .append(" and PresentMode: ")
                                .append(std::to_string(instance.swapchain.presentMode)));
}

void VkBackend::createCommandpool(
  Instance& instance,
  VkCommandPool &pool,
  const VkCommandPoolCreateFlags flags)
{
  const QueueFamilyInfo& queueInfo = instance.queueFamilies;

  vkGetDeviceQueue(instance.device, queueInfo.graphicsFamily.value(), 0, &instance.graphicsQueue);

  VkCommandPoolCreateInfo cmdPoolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  cmdPoolInfo.flags = flags;
  cmdPoolInfo.queueFamilyIndex = queueInfo.graphicsFamily.value();

  if (vkCreateCommandPool(instance.device, &cmdPoolInfo, nullptr, &pool) != VK_SUCCESS) {
    LOG(F, "Could not create VkCommandPool.");
  }
}

void VkBackend::createCommandbuffer(const Instance &instance, const VkCommandPool & pool, VkCommandBuffer &buffer) {
  VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool = pool;
  allocInfo.commandBufferCount = 1;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  if (vkAllocateCommandBuffers(instance.device, &allocInfo, &buffer) != VK_SUCCESS) {
    LOG(F, "Could not allocate VkCommandBuffer.");
  }
}


