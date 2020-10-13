#include <iostream>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shaders.h"

void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

int main(void) {
  // init glfw
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  auto window_width = 800;
  auto window_height = 600;
  auto *window =
      glfwCreateWindow(window_width, window_height, "OpenGL Fun!", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
  // --

  // load OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }
  // --

  auto shaderProgram = createShaderProgram();

  // Bind vertex array object
  unsigned int vertexArrayObject;
  glGenVertexArrays(1, &vertexArrayObject);
  glBindVertexArray(vertexArrayObject);

  // copy the vertices data to the GPU
  // clang-format off
  float vertices[] = {
    // positions          // colors           // texture coords
     0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
     0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
    -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 
  };

  unsigned int indices[] = {  // note that we start from 0!
    0, 1, 3,   // first triangle
    1, 2, 3    // second triangle
  };
  // clang-format on

  unsigned int vertexBuffer;
  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  unsigned int elementBuffer;
  glGenBuffers(1, &elementBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);
  // --

  // set the vertex attributes pointers
  // position
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // color
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // texture coordinate
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  // Textures
  stbi_set_flip_vertically_on_load(true);

  // Container
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
  unsigned char *containerTextureData = stbi_load(
      "../assets/textures/container.jpg", &width, &height, &nrChannels, 0);
  if (!containerTextureData) {
    std::cout << "Failed to load container texture" << std::endl;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, containerTextureData);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(containerTextureData);
  // Container --

  // awesome face
  unsigned int awesomeFaceTexture;
  glGenTextures(1, &awesomeFaceTexture);
  glBindTexture(GL_TEXTURE_2D, awesomeFaceTexture);

  // wrapping
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // filtering
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  unsigned char *awesomeFaceTextureData = stbi_load(
      "../assets/textures/awesomeface.png", &width, &height, &nrChannels, 0);
  if (!awesomeFaceTextureData) {
    std::cout << "Failed to load awesomeFace texture" << std::endl;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, awesomeFaceTextureData);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(awesomeFaceTextureData);
  // awesome face

  glUseProgram(shaderProgram);
  glUniform1i(glGetUniformLocation(shaderProgram, "containerTexture"), 0);
  glUniform1i(glGetUniformLocation(shaderProgram, "awesomeFaceTexture"), 1);

  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, containerTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, awesomeFaceTexture);

    glUseProgram(shaderProgram);
    glBindVertexArray(vertexArrayObject);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    /* auto timeValue = glfwGetTime(); */
    /* auto greenValue = (sin(timeValue) / 2.0f) + 0.5f; */
    /* auto vertexColorLocation = glGetUniformLocation(shaderProgram,
     * "vertexColor"); */
    /* glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f); */

    /* glBindVertexArray(vertexArrayObject); */
    /* glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); */
    /* glDrawArrays(GL_TRIANGLES, 0, 3); */

    /* glBindTexture(GL_TEXTURE_2D, texture); */

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &vertexArrayObject);
  glDeleteBuffers(1, &vertexBuffer);
  glDeleteBuffers(1, &elementBuffer);

  glfwTerminate();

  return 0;
}
