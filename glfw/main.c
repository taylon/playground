#include "stdio.h"
#include "stdlib.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "./config.h"

void error_callback(int error, const char *description) {
  fprintf(stderr, "Error: %s\n", description);
}

int main(void) {
  glfwSetErrorCallback(error_callback);

  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  /* Create a windowed mode window and its OpenGL context */
  GLFWwindow *window = glfwCreateWindow(640, 480, "Testing", NULL, NULL);
  if (!window) {
    glfwTerminate();

    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSwapInterval(1);

  while (!glfwWindowShouldClose(window)) {
    /* Render */
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    double time = glfwGetTime();

    /* glClear(GL_COLOR_BUFFER_BIT); */

    glfwSwapBuffers(window);
    glfwPollEvents();
    /* glfwWaitEvents(); */
  }

  glfwTerminate();
  exit(EXIT_SUCCESS);
}
