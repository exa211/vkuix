#include "shader.h"

#include <fstream>

Shader::Shader(VkDevice device, const std::string &filename) {
  fragModule = loadModule(device, filename, TYPE_FRAG_SHADER);
  vertModule = loadModule(device, filename, TYPE_VERT_SHADER);
}
std::array<VkPipelineShaderStageCreateInfo, 2> Shader::getShaderStageInfos() {
  std::array<VkPipelineShaderStageCreateInfo, 2> infos{};

  infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  infos[0].stage = TYPE_FRAG_SHADER.shaderFlag;
  infos[0].module = fragModule;
  infos[0].pName = "main";

  infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  infos[1].stage = TYPE_VERT_SHADER.shaderFlag;
  infos[1].module = vertModule;
  infos[1].pName = "main";

  return infos;
}

VkShaderModule Shader::loadModule(const VkDevice device, const std::string &filename, const ShaderType type) {
  std::ifstream file{"../assets/shader/" + filename + "." + type.fileTypeName + ".spv", std::ios::ate | std::ios::binary};
  if (!file.is_open()) LOG(W, "Could not open shader file: " << filename);

  const size_t fileSize = file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  VkShaderModule module{};

  VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  info.codeSize = buffer.size();
  info.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

  if (vkCreateShaderModule(device, &info, nullptr, &module) != VK_SUCCESS) {
    LOG(W, "Could not create VkShaderModule.");
  }

  return module;
}