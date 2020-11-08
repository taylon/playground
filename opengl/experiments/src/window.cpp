#include <stdio.h>
#include <GLFW/glfw3.h>

GLFWwindow *createWindow(int width, int height,
                         GLFWframebuffersizefun framebufferSizeCallback,
                         GLFWscrollfun scrollCallback,
                         GLFWcursorposfun cursorPositionCallback) {
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  auto *window = glfwCreateWindow(width, height, "OpenGL Fun!", NULL, NULL);
  if (window != NULL) {
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glfwSetScrollCallback(window, scrollCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  return window;
}
