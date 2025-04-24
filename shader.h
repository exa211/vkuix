#pragma once

#include "vulkan/vulkan.h"
#include "common.h"

struct ShaderType {
  const char *fileTypeName;
  const VkShaderStageFlagBits shaderFlag;
};

inline constexpr ShaderType TYPE_VERT_SHADER{"frag", VK_SHADER_STAGE_FRAGMENT_BIT};
inline constexpr ShaderType TYPE_FRAG_SHADER{"vert", VK_SHADER_STAGE_VERTEX_BIT};

class Shader {
public:
  explicit Shader(VkDevice device, const std::string& filename);
  [[nodiscard]] std::array<VkPipelineShaderStageCreateInfo, 2> getShaderStageInfos();
private:
  VkShaderModule fragModule;
  VkShaderModule vertModule;

  VkShaderModule loadModule(VkDevice device, const std::string &filename, const ShaderType type);
};