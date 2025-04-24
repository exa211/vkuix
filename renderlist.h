#pragma once

#include "vulkan_backend.h"

namespace VKUIX {

  class RenderList {
  public:
    void rect(float x, float y, float w, float h, Color c);

    struct BorderRadius {

      BorderRadius(const float all) : topLeft(all), topRight(all), bottomLeft(all), bottomRight(all) {}
      BorderRadius(const float tl, const float tr, const float bl, const float br) : topLeft(tl), topRight(tr), bottomLeft(bl), bottomRight(br) {}

      float topLeft;
      float topRight;
      float bottomLeft;
      float bottomRight;
    };
    void roundRect(float x, float y, float w, float h, BorderRadius radis, int subdiv, Color c);

    std::vector<VkBackend::Vertex>& getVertices();

    void clear();
  private:
    std::vector<VkBackend::Vertex> vertices;
    std::vector<u32> indices;
  };

}