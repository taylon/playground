#include <stdio.h>
#include <stdlib.h>

#include <GLES3/gl3.h>

GLuint create_shader(unsigned int type, const char *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLint length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    GLchar *message = malloc(length);
    glGetShaderInfoLog(shader, length, &length, message);
    fprintf(stderr, "unable to create shader: %s\n", message);
    free(message);
  }

  return shader;
}

const char *vertex_shader_source =
    "#version 320 es\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

const char *fragment_shader_source =
    "#version 320 es\n"
    "precision mediump float;"
    "out vec4 color;\n"
    "void main()\n"
    "{\n"
    "   color = vec4(0.380f, 0.686f, 0.937f, 1.0f);\n"
    "}\n\0";

int create_shader_program() {
  GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
  GLuint fragment_shader =
      create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertex_shader);
  glAttachShader(shaderProgram, fragment_shader);
  glLinkProgram(shaderProgram);

  GLint success;
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    GLint length;
    glGetShaderiv(shaderProgram, GL_INFO_LOG_LENGTH, &length);

    GLchar *message = malloc(length);
    glGetShaderInfoLog(shaderProgram, length, &length, message);
    fprintf(stderr, "unable to create shader program: %s\n", message);
    free(message);

    return -1;
  }

  return shaderProgram;
}
