#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>

char *readShaderFile(const char *filePath) {
  auto file = fopen(filePath, "r");
  if (file == NULL) {
    fprintf(stderr, "unable to open the file: %s\n", filePath);
    exit(EXIT_FAILURE);
  }

  fseek(file, 0, SEEK_END);
  auto fileLength = ftell(file);
  fseek(file, 0, SEEK_SET);

  auto buffer = (char *)malloc(fileLength + 1);
  buffer[fileLength] = '\0';
  fread(buffer, sizeof(char), fileLength, file);

  fclose(file);
  return buffer;
}

unsigned int createShader(unsigned int type, const char *source) {
  auto shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    int length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    auto message = (char *)alloca(length * sizeof(char));
    glGetShaderInfoLog(shader, length, &length, message);
    fprintf(stderr, "unable to create shader: %s\n", message);
  }

  return shader;
}

unsigned int createVertexShader(const char *filePath) {
  auto fileContent = readShaderFile(filePath);
  auto shader = createShader(GL_VERTEX_SHADER, fileContent);

  free(fileContent);
  return shader;
}

unsigned int createFragmentShader(const char *filePath) {
  auto fileContent = readShaderFile(filePath);
  auto shader = createShader(GL_FRAGMENT_SHADER, fileContent);

  free(fileContent);
  return shader;
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
    fprintf(stderr, "unable to create shader program: %s\n", message);
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return shaderProgram;
}
