#include <iostream>

#include <glad/glad.h>

#include <fstream>
#include <sstream>

std::string readShaderFile(std::string filePath) {
  std::string fileContent;
  std::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  try {
    file.open(filePath);

    std::stringstream stream;
    stream << file.rdbuf();
    file.close();

    fileContent = stream.str();
  } catch (std::ifstream::failure e) {
    std::cout << "unable to read shader file: " << filePath << " - " << e.what()
              << std::endl;
  }

  return fileContent.c_str();
}

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

unsigned int createVertexShader(std::string filePath) {
  return createShader(GL_VERTEX_SHADER, readShaderFile(filePath));
}

unsigned int createFragmentShader(std::string filePath) {
  return createShader(GL_FRAGMENT_SHADER, readShaderFile(filePath));
}

unsigned int createShaderProgram() {
  auto vertexShader = createVertexShader("../shaders/vertex.glsl");
  auto fragmentShader = createFragmentShader("../shaders/fragment.glsl");

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
