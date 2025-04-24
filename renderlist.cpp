#include "renderlist.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/constants.hpp>

void VKUIX::RenderList::rect(float x, float y, const float w, const float h, Color c) {
  std::vector<VkBackend::Vertex> rectVertices = {
      {{x, y}, c.glmDecimal()},
      {{x + w, y}, c.glmDecimal()},
      {{x + w, y + h}, c.glmDecimal()},

      {{x, y}, c.glmDecimal()},
      {{x + w, y + h}, c.glmDecimal()},
      {{x, y + h}, c.glmDecimal()},
  };
  vertices.insert(vertices.end(), rectVertices.begin(), rectVertices.end());
}

void VKUIX::RenderList::roundRect(float x, float y, float w, float h, BorderRadius radis, int subdiv, Color c) {
  // Ensure corner radius doesn't exceed half the smaller dimension
  radis.topLeft = glm::min(radis.topLeft, glm::min(w, h) / 2.0f);
  radis.topRight = glm::min(radis.topRight, glm::min(w, h) / 2.0f);
  radis.bottomLeft = glm::min(radis.bottomLeft, glm::min(w, h) / 2.0f);
  radis.bottomRight = glm::min(radis.bottomRight, glm::min(w, h) / 2.0f);

  const glm::vec4 col = c.glmDecimal();

  // Calculate bounds
  float left = x;
  float right = x + w;
  float bottom = y;
  float top = y + h;

  // Calculate inner rectangle bounds (where corners start)
  float innerLeft = left + radis.topLeft;
  float innerRight = right - radis.topRight;
  float innerBottom = bottom + radis.bottomLeft;
  float innerTop = top - radis.topLeft;

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
  for (auto corner : corners) {
    for (int i = 0; i < subdiv; i++) {
      float angle1 = corner.startAngle + (glm::half_pi<float>() * i / subdiv);
      float angle2 = corner.startAngle + (glm::half_pi<float>() * (i + 1) / subdiv);

      float x1 = corner.center.x + radis.topLeft * glm::cos(angle1);
      float y1 = corner.center.y + radis.topRight * glm::sin(angle1);
      float x2 = corner.center.x + radis.bottomLeft * glm::cos(angle2);
      float y2 = corner.center.y + radis.topLeft * glm::sin(angle2);

      // Add triangle (corner center, point1, point2)
      vertices.push_back({{corner.center}, col});
      vertices.push_back({{x1, y1}, col});
      vertices.push_back({{x2, y2}, col});
    }
  }

  // Inner rectangle (central part)
  vertices.push_back({{innerLeft, innerBottom}, col});
  vertices.push_back({{innerRight, innerBottom}, col});
  vertices.push_back({{innerRight, innerTop}, col});

  vertices.push_back({{innerLeft, innerBottom}, col});
  vertices.push_back({{innerRight, innerTop}, col});
  vertices.push_back({{innerLeft, innerTop}, col});

  // Bottom rectangle
  vertices.push_back({{innerLeft, bottom}, col});
  vertices.push_back({{innerRight, bottom}, col});
  vertices.push_back({{innerRight, innerBottom}, col});

  vertices.push_back({{innerLeft, bottom}, col});
  vertices.push_back({{innerRight, innerBottom}, col});
  vertices.push_back({{innerLeft, innerBottom}, col});

  // Right rectangle
  vertices.push_back({{innerRight, innerBottom}, col});
  vertices.push_back({{right, innerBottom}, col});
  vertices.push_back({{right, innerTop}, col});

  vertices.push_back({{innerRight, innerBottom}, col});
  vertices.push_back({{right, innerTop}, col});
  vertices.push_back({{innerRight, innerTop}, col});

  // Top rectangle
  vertices.push_back({{innerLeft, innerTop}, col});
  vertices.push_back({{innerRight, innerTop}, col});
  vertices.push_back({{innerRight, top}, col});

  vertices.push_back({{innerLeft, innerTop}, col});
  vertices.push_back({{innerRight, top}, col});
  vertices.push_back({{innerLeft, top}, col});

  // Left rectangle
  vertices.push_back({{left, innerBottom}, col});
  vertices.push_back({{innerLeft, innerBottom}, col});
  vertices.push_back({{innerLeft, innerTop}, col});

  vertices.push_back({{left, innerBottom}, col});
  vertices.push_back({{innerLeft, innerTop}, col});
  vertices.push_back({{left, innerTop}, col});
}

std::vector<VkBackend::Vertex> &VKUIX::RenderList::getVertices() {
  return vertices;
}

void VKUIX::RenderList::clear() {
  vertices.clear();
  vertices = std::vector<VkBackend::Vertex>();
}
