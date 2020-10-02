#include <iostream>

#include <glad/glad.h>

unsigned int createShader(unsigned int type, const std::string &source) {
  auto shader = glCreateShader(type);
  const char *src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    int length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    auto message = (char *)alloca(length * sizeof(char));
    glGetShaderInfoLog(shader, length, &length, message);
    std::cout << "unable to create shader: " << message << std::endl;
  }

  return shader;
}

unsigned int createShaderProgram() {
  std::string vertexShaderSource =
      "#version 460 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "void main()\n"
      "{\n"
      "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
      "}\0";
  auto vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSource);

  std::string fragmentShaderSource =
      "#version 460 core\n"
      "out vec4 color;\n"
      "void main()\n"
      "{\n"
      "   color = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
      "}\n\0";
  auto fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

  // shader program creation and linking
  auto shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  int success;
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    int length;
    auto message = (char *)alloca(length * sizeof(char));
    glGetShaderiv(shaderProgram, GL_INFO_LOG_LENGTH, &length);

    glGetShaderInfoLog(shaderProgram, length, &length, message);
    std::cout << "unable to create shader program: " << message << std::endl;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return shaderProgram;
}
