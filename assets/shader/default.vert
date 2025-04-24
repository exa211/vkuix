#version 450

layout (location = 0) in vec2 vPos;
layout (location = 1) in vec4 vCol;

layout (location = 1) out vec4 outCol;

layout (push_constant) uniform constants {
  mat4 model;
  mat4 view;
  mat4 proj;
} Matrix;

void main() {
  gl_Position = Matrix.proj * Matrix.model * vec4(vPos, 0.0f, 1.0f);
  outCol = vCol;
}
