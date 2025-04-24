#pragma once

#include <glm/gtc/constants.hpp>


#include "vulkan_backend.h"

namespace VKUIX {

  struct Dim {
    u32 width;
    u32 height;
  };

  class RenderList {
  public:
    void rect(float x, float y, float width, float height) {
      std::vector<VkBackend::Vertex> rectVertices = {
          {{x, y}, {0.0f, 0.0f, 0.0f, 1.0f}},
          {{x + width, y}, {0.0f, 0.0f, 0.0f, 1.0f}},
          {{x + width, y + height}, {0.0f, 0.0f, 0.0f, 1.0f}},

          {{x, y}, {0.0f, 0.0f, 0.0f, 1.0f}},
          {{x + width, y + height}, {0.0f, 0.0f, 0.0f, 1.0f}},
          {{x, y + height}, {0.0f, 0.0f, 0.0f, 1.0f}},
      };
      vertices.insert(vertices.end(), rectVertices.begin(), rectVertices.end());
    }

    void roundedRect(float x, float y, float width, float height, float radius, int subdiv) {
      // Ensure corner radius doesn't exceed half the smaller dimension
      radius = glm::min(radius, glm::min(width, height) / 2.0f);

      // Calculate bounds
      float left = x;
      float right = x + width;
      float bottom = y;
      float top = y + height;

      // Calculate inner rectangle bounds (where corners start)
      float innerLeft = left + radius;
      float innerRight = right - radius;
      float innerBottom = bottom + radius;
      float innerTop = top - radius;

      // Define corner centers and angle ranges
      struct Corner {
        glm::vec2 center;
        float startAngle;
      };

      Corner corners[4] = {
          {{innerLeft, innerBottom}, glm::pi<float>()}, // Bottom-left
          {{innerRight, innerBottom}, glm::pi<float>() * 1.5f}, // Bottom-right
          {{innerRight, innerTop}, 0.0f}, // Top-right
          {{innerLeft, innerTop}, glm::pi<float>() * 0.5f} // Top-left
      };

      // Generate corner arcs using triangles
      for (int c = 0; c < 4; c++) {
        const Corner &corner = corners[c];

        for (int i = 0; i < subdiv; i++) {
          float angle1 = corner.startAngle + (glm::half_pi<float>() * i / subdiv);
          float angle2 = corner.startAngle + (glm::half_pi<float>() * (i + 1) / subdiv);

          float x1 = corner.center.x + radius * glm::cos(angle1);
          float y1 = corner.center.y + radius * glm::sin(angle1);
          float x2 = corner.center.x + radius * glm::cos(angle2);
          float y2 = corner.center.y + radius * glm::sin(angle2);

          // Add triangle (corner center, point1, point2)
          vertices.push_back({corner.center});
          vertices.push_back({{x1, y1}});
          vertices.push_back({{x2, y2}});
        }
      }

      // Inner rectangle (central part)
      vertices.push_back({{innerLeft, innerBottom}});
      vertices.push_back({{innerRight, innerBottom}});
      vertices.push_back({{innerRight, innerTop}});

      vertices.push_back({{innerLeft, innerBottom}});
      vertices.push_back({{innerRight, innerTop}});
      vertices.push_back({{innerLeft, innerTop}});

      // Bottom rectangle
      vertices.push_back({{innerLeft, bottom}});
      vertices.push_back({{innerRight, bottom}});
      vertices.push_back({{innerRight, innerBottom}});

      vertices.push_back({{innerLeft, bottom}});
      vertices.push_back({{innerRight, innerBottom}});
      vertices.push_back({{innerLeft, innerBottom}});

      // Right rectangle
      vertices.push_back({{innerRight, innerBottom}});
      vertices.push_back({{right, innerBottom}});
      vertices.push_back({{right, innerTop}});

      vertices.push_back({{innerRight, innerBottom}});
      vertices.push_back({{right, innerTop}});
      vertices.push_back({{innerRight, innerTop}});

      // Top rectangle
      vertices.push_back({{innerLeft, innerTop}});
      vertices.push_back({{innerRight, innerTop}});
      vertices.push_back({{innerRight, top}});

      vertices.push_back({{innerLeft, innerTop}});
      vertices.push_back({{innerRight, top}});
      vertices.push_back({{innerLeft, top}});

      // Left rectangle
      vertices.push_back({{left, innerBottom}});
      vertices.push_back({{innerLeft, innerBottom}});
      vertices.push_back({{innerLeft, innerTop}});

      vertices.push_back({{left, innerBottom}});
      vertices.push_back({{innerLeft, innerTop}});
      vertices.push_back({{left, innerTop}});
    }

    std::vector<VkBackend::Vertex> &getVertices() {
      return vertices;
    }

    void clear() {
      vertices.clear();
    }

  private:
    std::vector<VkBackend::Vertex> vertices;
    std::vector<u32> indices;
  };

  struct Instance {
    VkBackend::Instance backend;

    VkCommandPool cmdPool{};
    VkCommandPool uploadPool{};
    VkCommandBuffer cmdBuffer{};

    VkDescriptorPool mainDescPool{};
    VkDescriptorSetLayout descLayoutUniform{};
    VkDescriptorSet mainDescriptor{};

    VkPipeline defaultPipeline{};
    VkPipelineLayout defaultPipelineLayout{};

    VkViewport viewport{};
    Image msaaImage{};

    std::vector<VkBackend::RenderFrame> renderFrames{};
    u32 frameIndex{0};

    uptr<RenderList> renderList{};
  };

  sptr<Window> createWindow(const char *title, Dim dimension);
  sptr<Instance> createInstance(const sptr<Window> &window, const std::vector<const char *> *additionalExtensions = nullptr);

  uptr<RenderList> &getRenderList(const sptr<Instance> &instance);
  void render(const sptr<Instance> &instance, const sptr<Window> &window);

} // namespace VKUIX