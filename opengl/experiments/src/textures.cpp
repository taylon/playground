#include <stdio.h>

#include <stb_image.h>
#include <glad/glad.h>

// @errorHandling
unsigned int buildAwesomeFaceTexture() {
  unsigned int awesomeFaceTexture;
  glGenTextures(1, &awesomeFaceTexture);
  glBindTexture(GL_TEXTURE_2D, awesomeFaceTexture);

  // wrapping
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // filtering
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int width, height, nrChannels;
  auto *awesomeFaceTextureData = stbi_load("../assets/textures/awesomeface.png",
                                           &width, &height, &nrChannels, 0);
  if (!awesomeFaceTextureData) {
    fprintf(stderr, "failed to load awesomeFace texture\n");
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, awesomeFaceTextureData);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(awesomeFaceTextureData);

  return awesomeFaceTexture;
}

unsigned int buildContanierTexture() {
  unsigned int containerTexture;
  glGenTextures(1, &containerTexture);
  glBindTexture(GL_TEXTURE_2D, containerTexture);

  // wrapping
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // filtering
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int width, height, nrChannels;
  auto *containerTextureData = stbi_load("../assets/textures/container.jpg",
                                         &width, &height, &nrChannels, 0);
  if (!containerTextureData) {
    fprintf(stderr, "failed to load container texture\n");
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, containerTextureData);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(containerTextureData);

  return containerTexture;
}
