#pragma once

#include <vma/vk_mem_alloc.h>
#include "vulkan/vulkan.h"

struct Image {
  VkImage vkImage{};
  VkImageView view{};
  VkExtent2D extent{};
  VkFormat format;
  VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
  VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VmaAllocation alloc{};
};