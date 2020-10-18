#include <iostream>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shaders.h"

void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

auto yaw = -90.0f;
auto pitch = 0.0f;
auto lastX = 400.0f;
auto lastY = 300.0f;
void mouseCallback(GLFWwindow *window, double xpos, double ypos) {
  float xoffset = xpos - lastX;
  // reversed since y-coordinates range from bottom to top
  float yoffset = lastY - ypos;
  lastX = xpos;
  lastY = ypos;

  const float sensitivity = 0.1f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  yaw += xoffset;
  pitch += yoffset;

  if (pitch > 89.0f) {
    pitch = 89.0f;
  } else if (pitch < -89.0f) {
    pitch = -89.0f;
  }
}

auto fov = 60.0f;
void scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
  fov -= (float)yoffset;
  if (fov < 1.0f) {
    fov = 1.0f;
  } else if (fov > 45.0f) {
    fov = 45.0f;
  }
}

auto lastFrame = 0.0f;
float calculateCameraSpeed() {
  float currentFrame = glfwGetTime();
  auto deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;

  return deltaTime;
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

  glfwSetScrollCallback(window, scrollCallback);
  glfwSetCursorPosCallback(window, mouseCallback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // Setup OpenGL
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glEnable(GL_DEPTH_TEST);
  // ---

  auto shaderProgram = createShaderProgram();

  // Copy the vertices data to the GPU
  unsigned int vertexArrayObject;
  glGenVertexArrays(1, &vertexArrayObject);
  glBindVertexArray(vertexArrayObject);

  // clang-format off
  /* float vertices[] = { */
  /*   // positions          // colors           // texture coords */
  /*    0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right */
  /*    0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right */
  /*   -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left */
  /*   -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left */ 
  /* }; */

  float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
  // clang-format on

  unsigned int vertexBuffer;
  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  // --

  // set the vertex attributes pointers
  // position
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // texture coordinate
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

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
  auto *containerTextureData = stbi_load("../assets/textures/container.jpg",
                                         &width, &height, &nrChannels, 0);
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

  auto *awesomeFaceTextureData = stbi_load("../assets/textures/awesomeface.png",
                                           &width, &height, &nrChannels, 0);
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

  glm::vec3 cubePositions[] = {
      glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
      glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
      glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
      glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
      glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f)};

  auto cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
  auto cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
  auto cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    // tmp camera movement input
    const auto cameraSpeed = calculateCameraSpeed() * 2.5f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
      cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
      cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
      cameraPos -=
          glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
      cameraPos +=
          glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    // --

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, containerTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, awesomeFaceTexture);

    glUseProgram(shaderProgram);

    // camera
    auto cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    auto cameraDirection = glm::normalize(cameraPos - cameraTarget);
    auto up = glm::vec3(0.0f, 1.0f, 0.0f);
    auto cameraRight = glm::normalize(glm::cross(up, cameraDirection));
    auto cameraUp = glm::cross(cameraDirection, cameraRight);

    auto view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE,
                       glm::value_ptr(view));

    auto projection =
        glm::perspective(glm::radians(fov), 800.0f / 600.0f, 0.1f, 100.0f);
    /* auto projection = */
    /*     glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
     */
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1,
                       GL_FALSE, glm::value_ptr(projection));
    // ----

    glBindVertexArray(vertexArrayObject);
    for (auto i = 0; i < 10; i++) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);

      if (i % 3 == 0) {
        /* float angle = 20.0f * i; */
        /* model = glm::rotate(model, glm::radians(angle), */
        model = glm::rotate(model, (float)glfwGetTime(),
                            glm::vec3(1.0f, 0.3f, 0.5f));
      }

      auto angle = 20.0f * i;
      model =
          glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1,
                         GL_FALSE, glm::value_ptr(model));

      glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &vertexArrayObject);
  glDeleteBuffers(1, &vertexBuffer);

  glfwTerminate();

  return 0;
}
