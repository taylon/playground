#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 vertexColor;
out vec2 texCoord;

uniform mat4 transform;

void main() {
  gl_Position = transform * vec4(aPosition, 1.0);

  vertexColor = aColor;
  texCoord = aTexCoord;
}
