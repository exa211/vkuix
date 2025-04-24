#define VMA_IMPLEMENTATION
#include "vulkan_backend.h"

#include <ranges>
#include <set>

void VkBackend::setupInstance(Instance &instance, const std::vector<const char *>& extensions) {
  LOG(D, "Creating VkInstance.");
  // Application info and api version of Vulkan
  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.apiVersion = VK_API_VERSION_1_3;
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.pEngineName = "vkuix-engine";

  // DEBUG CALLBACK
  VkDebugUtilsMessengerCreateInfoEXT debugInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT;
  debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT;
  debugInfo.pfnUserCallback = debugCallback;
  debugInfo.pUserData = nullptr; // Maybe needed in the future

  VkInstanceCreateInfo instanceCreateInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  instanceCreateInfo.pApplicationInfo = &appInfo;
  instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
  instanceCreateInfo.enabledExtensionCount = extensions.size();
  if (VALIDATION) {
    instanceCreateInfo.ppEnabledLayerNames = VAL_LAYERS.data();
    instanceCreateInfo.enabledLayerCount = VAL_LAYERS.size();
  }
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
  LOG(D, "Created window surface.");
}

void VkBackend::setupDevices(Instance &instance) {
  // Choose gpu
  u32 deviceCount = 0;
  vkEnumeratePhysicalDevices(instance.vkInstance, &deviceCount, nullptr);
  std::vector<VkPhysicalDevice> physDevices{deviceCount};
  vkEnumeratePhysicalDevices(instance.vkInstance, &deviceCount, physDevices.data());
  if (deviceCount == 0)
    LOG(F, "Found no physical rendering device (GPU)");
  instance.physDevice = physDevices.at(0); // TODO: Hardcoded for the moment.

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

  // Device Features
  VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeat{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES};
  dynamicRenderingFeat.dynamicRendering = VK_TRUE;

  VkPhysicalDeviceSynchronization2Features sync2Feat{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES};
  sync2Feat.synchronization2 = VK_TRUE;
  sync2Feat.pNext = &dynamicRenderingFeat;

  // Logical Device
  VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceInfo.enabledExtensionCount = DEVICE_EXT.size();
  deviceInfo.ppEnabledExtensionNames = DEVICE_EXT.data();
  deviceInfo.queueCreateInfoCount = queueCreateInfos.size();
  deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceInfo.pNext = &sync2Feat;

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

void VkBackend::setupSwapchain(const sptr<VKUIX::Window>& window, Instance &instance, const bool resize) {
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
    if (format.format == VkBackend::COLOR_FORMAT && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      instance.swapchain.surfaceFormat = format;
      break;
    }
  }

  instance.swapchain.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  instance.swapchain.extent = window->getExtent();

  u32 imageCount = caps.minImageCount;
  if (caps.maxImageCount > 0 && caps.minImageCount > caps.maxImageCount)
    imageCount = caps.maxImageCount;
  instance.swapchain.framebufferingAmount = imageCount;

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
                                .append(std::to_string(instance.swapchain.presentMode))
                                .append("with " + std::to_string(imageCount) + " image count"));
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

void VkBackend::createFence(const Instance &instance, VkFence &fenceOut) {

  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  if (vkCreateFence(instance.device, &fenceInfo, nullptr, &fenceOut) != VK_SUCCESS) {
    LOG(W, "Could not create VkFence.");
  }

}

void VkBackend::createSemaphore(const Instance &instance, VkSemaphore &semaOut) {

  VkSemaphoreCreateInfo semaInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  vkCreateSemaphore(instance.device, &semaInfo, nullptr, &semaOut);

}

void VkBackend::createImage(const Instance &instance, Image &image) {

  VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageInfo.format = image.format;
  imageInfo.extent = {image.extent.width, image.extent.height, 1}; // Depth must be 1 lol otherwise it will not create
  imageInfo.samples = image.sampleCount;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.arrayLayers = 1;
  imageInfo.mipLevels = 1;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.usage = image.usageFlags;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vmaCreateImage(instance.allocator, &imageInfo, &allocInfo, &image.vkImage, &image.alloc, nullptr) != VK_SUCCESS) {
    LOG(W, "Could not create VkImage.");
  }

  VkImageViewCreateInfo imageViewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  imageViewInfo.image = image.vkImage;
  imageViewInfo.format = image.format;
  imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewInfo.subresourceRange.baseMipLevel = 0;
  imageViewInfo.subresourceRange.levelCount = 1;
  imageViewInfo.subresourceRange.baseArrayLayer = 0;
  imageViewInfo.subresourceRange.layerCount = 1;
  imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

  if (vkCreateImageView(instance.device, &imageViewInfo, nullptr, &image.view) != VK_SUCCESS) {
    LOG(W, "Could not create VkImageView from VkImage.");
  }

}


void VkBackend::transitionImage(VkCommandBuffer &cmdBuffer, VkImage &image,
  VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccessMask,
  VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccessMask,
  VkImageLayout oldLayout, VkImageLayout newLayout) {

  VkImageMemoryBarrier2 imageBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  imageBarrier.srcStageMask = srcStage;
  imageBarrier.srcAccessMask = srcAccessMask;

  imageBarrier.dstStageMask = dstStage;
  imageBarrier.dstAccessMask = dstAccessMask;

  imageBarrier.oldLayout = oldLayout;
  imageBarrier.newLayout = newLayout;
  imageBarrier.image = image;
  imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarrier.subresourceRange.baseMipLevel = 0;
  imageBarrier.subresourceRange.levelCount = 1;
  imageBarrier.subresourceRange.baseArrayLayer = 0;
  imageBarrier.subresourceRange.layerCount = 1;

  VkDependencyInfo depInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &imageBarrier;

  vkCmdPipelineBarrier2(cmdBuffer, &depInfo);

}


void VkBackend::createDescriptorPool(const Instance &instance, const DescriptorPoolInfo &poolInfo, VkDescriptorPool &poolOut) {

  VkDescriptorPoolCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  info.flags = poolInfo.flags;
  info.poolSizeCount = poolInfo.sizes.size();
  info.pPoolSizes = poolInfo.sizes.data();
  info.maxSets = poolInfo.maxSets;

  if (vkCreateDescriptorPool(instance.device, &info, nullptr, &poolOut) != VK_SUCCESS) {
    LOG(W, "Could not create VkDescriptorPool.");
  }

}

void VkBackend::createDescriptorLayout(const Instance &instance, const DescriptorSetLayoutInfo &layoutInfo, VkDescriptorSetLayout &descLayoutOut) {

  VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  info.flags = layoutInfo.createFlags;
  info.bindingCount = layoutInfo.layoutBindings.size();
  info.pBindings = layoutInfo.layoutBindings.data();
  info.pNext = layoutInfo.pExt;

  if (vkCreateDescriptorSetLayout(instance.device, &info, nullptr, &descLayoutOut) != VK_SUCCESS) {
    LOG(W, "Could not create VkDescriptorSetLayout.");
  }

}

void VkBackend::allocDescriptorSets(const Instance &instance, const DescriptorSetAllocInfo &allocInfo, VkDescriptorSet &descSetOut) {

  if (!allocInfo.pPool) {
    LOG(W, "DescriptorSetAllocInfo: DescriptorPool is nullptr. Aborting allocation.");
    return;
  }

  VkDescriptorSetAllocateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  info.descriptorPool = *allocInfo.pPool;
  info.descriptorSetCount = allocInfo.setCount;
  info.pSetLayouts = allocInfo.layouts.data();
  info.pNext = allocInfo.pExt;

  if (vkAllocateDescriptorSets(instance.device, &info, &descSetOut) != VK_SUCCESS) {
    LOG(F, "Could not create VkDescriptorSets.");
  }

}



void VkBackend::createRenderpass(const Instance &instance, RenderpassInfo &rpInfo, VkRenderPass& renderpass) {

  const u32 attachmentCount = rpInfo.colorDescriptions.size() + rpInfo.depthDescriptions.size();

  std::vector<VkAttachmentDescription> attachments{};
  attachments.insert(attachments.end(), rpInfo.colorDescriptions.begin(), rpInfo.colorDescriptions.end());
  attachments.insert(attachments.end(), rpInfo.depthDescriptions.begin(), rpInfo.depthDescriptions.end());

  VkRenderPassCreateInfo createInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  createInfo.attachmentCount = attachmentCount;
  createInfo.pAttachments = attachments.data();

  createInfo.subpassCount = rpInfo.subpassDescriptions.size();
  createInfo.pSubpasses = rpInfo.subpassDescriptions.data();

  createInfo.dependencyCount = rpInfo.subpassDependencies.size();
  createInfo.pDependencies = rpInfo.subpassDependencies.data();

  if (vkCreateRenderPass(instance.device, &createInfo, nullptr, &renderpass) != VK_SUCCESS) {
    LOG(F, "Could not create VkRenderpass.");
  }

}

void VkBackend::createDynamicGraphicsPipeline(const Instance &instance, Shader &shader,
  std::vector<VkDescriptorSetLayout> &layouts, VkPipeline &dynamicPipeline, VkPipelineLayout &pipelineLayout) {

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutInfo.setLayoutCount = layouts.size();
  pipelineLayoutInfo.pSetLayouts = layouts.data();

  VkPushConstantRange pushConstant{};
  pushConstant.offset = 0;
  pushConstant.size = sizeof(DefaultPushConstant);
  pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

  if (vkCreatePipelineLayout(instance.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    LOG(F, "Could not create corresponding VkPipelineLayout for VkPipeline. Aborting VkPipeline creation.");
  }

  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageInfos = shader.getShaderStageInfos();

  // Vertex Input Info eg. Position (XY later Z), Color (RGBA)
  VkPipelineVertexInputStateCreateInfo inputInfo{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  VertexInputDescription vertexInputDescription = Vertex::getVertexDescription();
  inputInfo.vertexBindingDescriptionCount = vertexInputDescription.bindings.size();
  inputInfo.pVertexBindingDescriptions = vertexInputDescription.bindings.data();
  inputInfo.vertexAttributeDescriptionCount = vertexInputDescription.attributes.size();
  inputInfo.pVertexAttributeDescriptions = vertexInputDescription.attributes.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  raster.depthClampEnable = VK_FALSE;
  raster.depthBiasEnable = VK_FALSE;
  raster.rasterizerDiscardEnable = VK_FALSE;
  raster.polygonMode = VK_POLYGON_MODE_FILL;
  raster.cullMode = VK_CULL_MODE_BACK_BIT;
  raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
  raster.lineWidth = 1.0f;

  VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisample.rasterizationSamples = ANTI_ALIASING_COUNT;

  VkPipelineDepthStencilStateCreateInfo depthStencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo colorBlend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  colorBlend.attachmentCount = 1;
  colorBlend.pAttachments = &colorBlendAttachment;

  VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewport.viewportCount = 1;
  viewport.scissorCount = 1;

  VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dynamicState.dynamicStateCount = std::size(dynamicStates);
  dynamicState.pDynamicStates = dynamicStates;

  VkPipelineRenderingCreateInfo renderingInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachmentFormats = &VkBackend::COLOR_FORMAT;
  renderingInfo.depthAttachmentFormat = depthFormat;
  renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

  VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  pipelineInfo.stageCount = shaderStageInfos.size();
  pipelineInfo.pStages = shaderStageInfos.data();
  pipelineInfo.pVertexInputState = &inputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pRasterizationState = &raster;
  pipelineInfo.pMultisampleState = &multisample;
  pipelineInfo.pViewportState = &viewport;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlend;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout; // TODO: Create a pipeline layout.
  pipelineInfo.renderPass = VK_NULL_HANDLE;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.pNext = &renderingInfo; // Use dynamic render info.

  if (vkCreateGraphicsPipelines(instance.device, nullptr, 1, &pipelineInfo, nullptr, &dynamicPipeline)
    != VK_SUCCESS) {
    LOG(F, "Could not create VkPipeline.");
  }

}




