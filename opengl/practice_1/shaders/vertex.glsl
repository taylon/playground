#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aColor;

uniform float xOffset;

out vec3 vertexColor;

void main() {
  gl_Position = vec4(aPosition.x - xOffset, aPosition.y, aPosition.z, 1.0);

  vertexColor = aColor;
}
