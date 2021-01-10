#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <unistd.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <gbm.h>

#include "shaders.h"

drmModeConnector *find_connector(int graphics_card_fd, drmModeRes *resources) {
  // find the first available connector with modes
  drmModeConnector *connector;
  for (int i = 0; i < resources->count_connectors; ++i) {
    connector = drmModeGetConnector(graphics_card_fd, resources->connectors[i]);
    if (connector != NULL) {
      // check if the monitor is connected and has at least one valid mode
      if (connector->connection == DRM_MODE_CONNECTED &&
          connector->count_modes > 0) {
        return connector;
      }

      drmModeFreeConnector(connector);
    }
  }

  return NULL;
}

drmModeEncoder *find_encoder_by_id(int graphics_card_fd, drmModeRes *resources,
                                   uint32_t encoder_id) {
  drmModeEncoder *encoder;
  for (int i = 0; i < resources->count_encoders; ++i) {
    encoder = drmModeGetEncoder(graphics_card_fd, resources->encoders[i]);
    if (encoder != NULL) {
      if (encoder->encoder_id == encoder_id) {
        return encoder;
      }

      drmModeFreeEncoder(encoder);
    }
  }

  return NULL;
}

static void destroy_drm_framebuffer_callback(struct gbm_bo *bo, void *data) {
  uint32_t framebuffer_id = (uintptr_t)data;
  if (framebuffer_id) {
    drmModeRmFB(gbm_device_get_fd(gbm_bo_get_device(bo)), framebuffer_id);
  }
}

uint32_t get_framebuffer_id_from_bo(struct gbm_bo *gbm_bo) {
  uint32_t id = (uintptr_t)gbm_bo_get_user_data(gbm_bo);
  if (id) {
    return id;
  }

  // KMS requires all buffer object planes to have the same modifier
  uint64_t modifiers[4] = {0};
  modifiers[0] = gbm_bo_get_modifier(gbm_bo);

  uint32_t handles[4] = {0};
  uint32_t strides[4] = {0};
  uint32_t offsets[4] = {0};
  for (int i = 0; i < gbm_bo_get_plane_count(gbm_bo); i++) {
    handles[i] = gbm_bo_get_handle_for_plane(gbm_bo, i).u32;
    strides[i] = gbm_bo_get_stride_for_plane(gbm_bo, i);
    offsets[i] = gbm_bo_get_offset(gbm_bo, i);
    modifiers[i] = modifiers[0];
  }

  uint32_t width = gbm_bo_get_width(gbm_bo);
  uint32_t height = gbm_bo_get_height(gbm_bo);
  uint32_t format = gbm_bo_get_format(gbm_bo);
  int graphics_card_fd = gbm_device_get_fd(gbm_bo_get_device(gbm_bo));
  if (gbm_bo_get_modifier(gbm_bo) != DRM_FORMAT_MOD_INVALID) {
    if (drmModeAddFB2WithModifiers(graphics_card_fd, width, height, format,
                                   handles, strides, offsets, modifiers, &id,
                                   DRM_MODE_FB_MODIFIERS)) {
      fprintf(stderr, "unable to add DRM framebuffer with modifiers\n");
    }
  } else {
    if (drmModeAddFB2(graphics_card_fd, width, height, format, handles, strides,
                      offsets, &id, 0)) {
      fprintf(stderr, "unable to add DRM framebuffer\n");
    }
  }

  gbm_bo_set_user_data(gbm_bo, (void *)(uintptr_t)id,
                       destroy_drm_framebuffer_callback);

  return id;
}

int main(int argc, char *argv[]) {
  // init drm
  int graphics_card_fd = open("/dev/dri/card0", O_RDWR);
  if (graphics_card_fd < 0) {
    fprintf(stderr, "unable to open /dev/dri/card0: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  drmModeRes *drm_resources = drmModeGetResources(graphics_card_fd);
  if (drm_resources == NULL) {
    fprintf(stderr, "unable to retrieve DRM resources %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  drmModeConnector *drm_connector =
      find_connector(graphics_card_fd, drm_resources);
  if (drm_connector == NULL) {
    fprintf(stderr, "no valid connector found\n");
    return EXIT_FAILURE;
  }

  drmModeEncoder *drm_encoder = find_encoder_by_id(
      graphics_card_fd, drm_resources, drm_connector->encoder_id);
  if (drm_encoder == NULL) {
    fprintf(stderr, "no enconder matching with connector\n");
    return EXIT_FAILURE;
  }

  uint32_t crtc_id = drm_encoder->crtc_id;
  drmModeModeInfo drm_mode = drm_connector->modes[0];
  // -- init drm

  // init gbm
  struct gbm_device *gbm_device = gbm_create_device(graphics_card_fd);

  uint32_t rgb_format = DRM_FORMAT_XRGB8888;
  uint64_t gbm_surface_modifier = DRM_FORMAT_MOD_LINEAR;
  struct gbm_surface *gbm_surface = gbm_surface_create_with_modifiers(
      gbm_device, drm_mode.hdisplay, drm_mode.vdisplay, rgb_format,
      &gbm_surface_modifier, 1);
  if (gbm_surface == NULL) {
    fprintf(stderr, "unable to create gbm surface\n");
    return EXIT_FAILURE;
  }
  // -- init gbm

  // init egl
  EGLDisplay egl_display = eglGetDisplay((void *)gbm_device);
  if (!eglInitialize(egl_display, NULL, NULL)) {
    fprintf(stderr, "failed to initialize egl\n");
    return EXIT_FAILURE;
  }

  if (!eglBindAPI(EGL_OPENGL_ES_API)) {
    fprintf(stderr, "failed to bind EGL_OPENGL_ES_API\n");
    return EXIT_FAILURE;
  }

  EGLint egl_configs_count = 0;
  if (!eglGetConfigs(egl_display, NULL, 0, &egl_configs_count) ||
      egl_configs_count < 1) {
    fprintf(stderr, "no EGL configs to choose from\n");
    return EXIT_FAILURE;
  }

  EGLConfig *egl_configs = malloc(egl_configs_count * sizeof(*egl_configs));
  if (!egl_configs) {
    fprintf(stderr, "unable to allocate memmory for EGL configs\n");
    return EXIT_FAILURE;
  }

  const EGLint egl_config_attribs[] = {EGL_SURFACE_TYPE,
                                       EGL_WINDOW_BIT,
                                       EGL_RED_SIZE,
                                       1,
                                       EGL_GREEN_SIZE,
                                       1,
                                       EGL_BLUE_SIZE,
                                       1,
                                       EGL_ALPHA_SIZE,
                                       0,
                                       EGL_RENDERABLE_TYPE,
                                       EGL_OPENGL_ES2_BIT,
                                       EGL_SAMPLES,
                                       0,
                                       EGL_NONE};
  EGLint egl_configs_matched_count = 0;
  if (!eglChooseConfig(egl_display, egl_config_attribs, egl_configs,
                       egl_configs_count, &egl_configs_matched_count) ||
      !egl_configs_matched_count) {
    fprintf(stderr, "no EGL configs with appropriate attributes\n");

    free(egl_configs);
    return EXIT_FAILURE;
  }

  for (int i = 0; i < egl_configs_matched_count; ++i) {
    EGLint native_visual_id;
    if (eglGetConfigAttrib(egl_display, egl_configs[i], EGL_NATIVE_VISUAL_ID,
                           &native_visual_id) == EGL_TRUE)
      if (native_visual_id == rgb_format)
        egl_configs = egl_configs[i];
  }

  EGLint context_attribs[] = {EGL_CONTEXT_MAJOR_VERSION, 3, EGL_NONE};
  EGLContext egl_context = eglCreateContext(egl_display, egl_configs,
                                            EGL_NO_CONTEXT, context_attribs);
  if (egl_context == NULL) {
    fprintf(stderr, "unable to create EGL context\n");
    return EXIT_FAILURE;
  }

  EGLSurface egl_surface = eglCreateWindowSurface(
      egl_display, egl_configs, (EGLNativeWindowType)gbm_surface, NULL);
  if (egl_surface == EGL_NO_SURFACE) {
    fprintf(stderr, "unable to create EGL surface\n");
    return EXIT_FAILURE;
  }

  eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

  // -- init egl

  // GL
  int shader_program = create_shader_program();
  if (shader_program < 0)
    return EXIT_FAILURE;
  glUseProgram(shader_program);

  // Bind vertex array object
  GLuint vertexArrayObject;
  glGenVertexArrays(1, &vertexArrayObject);
  glBindVertexArray(vertexArrayObject);

  // clang-format off
  float vertices[] = {
    -0.25f, -0.25f, 0.0f,
    0.25f, -0.25f, 0.0f,
    0.0f,  0.50f, 0.0f,
  };
  // clang-format on

  GLuint buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  // -- GL

  glClearColor(0.156f, 0.172f, 0.203f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  eglSwapBuffers(egl_display, egl_surface);
  struct gbm_bo *front_framebuffer = gbm_surface_lock_front_buffer(gbm_surface);
  uint32_t front_framebuffer_id = get_framebuffer_id_from_bo(front_framebuffer);

  int err = drmModeSetCrtc(graphics_card_fd, crtc_id, front_framebuffer_id, 0,
                           0, &drm_connector->connector_id, 1, &drm_mode);
  if (err) {
    fprintf(stderr, "unable to perform mode setting: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  while (1) {
    glClearColor(0.156f, 0.172f, 0.203f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    eglSwapBuffers(egl_display, egl_surface);
    struct gbm_bo *back_framebuffer =
        gbm_surface_lock_front_buffer(gbm_surface);
    uint32_t back_framebuffer_id = get_framebuffer_id_from_bo(back_framebuffer);
    if (!back_framebuffer_id) {
      fprintf(stderr, "failed to get back framebuffer\n");
      return EXIT_FAILURE;
    }

    int err = drmModeSetCrtc(graphics_card_fd, crtc_id, back_framebuffer_id, 0,
                             0, &drm_connector->connector_id, 1, &drm_mode);
    if (err)
      fprintf(stderr, "unable to flip buffers (%d): %m\n", errno);

    gbm_surface_release_buffer(gbm_surface, front_framebuffer);
    front_framebuffer = back_framebuffer;
  }

  return 0;
}
