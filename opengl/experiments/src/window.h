#include <GLFW/glfw3.h>

GLFWwindow *createWindow(int width, int height,
                         GLFWframebuffersizefun framebufferSizeCallback,
                         GLFWscrollfun scrollCallback,
                         GLFWcursorposfun cursorPositionCallback);
