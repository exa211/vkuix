#pragma once

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "common.h"

namespace Buffers {

  struct Buffer {
    VkBuffer buffer;
    VmaAllocation allocation;
  };

  template <typename T>
  void createBuffer(
    std::vector<T> &bufferData,
    const VmaAllocator &allocator,
    Buffer &bufferOut,
    const VkBufferUsageFlags usageFlags,
    const VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE) {

    if (bufferData.empty()) {
      LOG(W, "Buffer size 0.");
      return;
    }

    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = bufferData.size() * sizeof(bufferData[0]);
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = sharingMode;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &bufferOut.buffer, &bufferOut.allocation, nullptr) != VK_SUCCESS) {
      LOG(W, "Could not allocate Buffer.");
    }

  }

  template <typename T>
  void mapBuffer(std::vector<T> &data, Buffer &buffer, VmaAllocator allocator) {

    if (data.empty() || !buffer.buffer) {
      LOG(W, "Could not map data to VkBuffer. Aborting mapping.");
      return;
    }

    void* tempData;
    vmaMapMemory(allocator, buffer.allocation, &tempData);
    memcpy(tempData, data.data(), data.size() * sizeof(data[0]));
    vmaUnmapMemory(allocator, buffer.allocation);
  }

}