#pragma once

#include <vma/vk_mem_alloc.h>
#include "vulkan/vulkan.h"

struct Image {
  VkImage vkImage{};
  VkImageView view{};
  VkExtent2D extent{};
  VkFormat format = VK_FORMAT_R8G8B8A8_SNORM;
  VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VmaAllocation alloc{};
};