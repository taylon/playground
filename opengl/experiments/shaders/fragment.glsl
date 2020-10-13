#version 460 core

in vec3 vertexColor;
in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D containerTexture;
uniform sampler2D awesomeFaceTexture;

void main() {
  fragColor = mix(texture(containerTexture, texCoord), texture(awesomeFaceTexture, texCoord), 0.2);
}
