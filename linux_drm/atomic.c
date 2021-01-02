#define _DEFAULT_SOURCE
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <errno.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

typedef struct FrameBuffer {
  uint32_t id;

  uint32_t width;
  uint32_t height;

  uint32_t pitch;
  uint32_t size;
  uint32_t bytes_per_pixel;

  uint32_t handle;
  uint8_t *pixels;
} FrameBuffer;

int create_framebuffer(int graphics_card_fd, FrameBuffer *framebuffer) {
  // TODO(taylon): we rely on width and height to exist on frame_buffer by the
  // time that this function is called, should we get width and height as
  // parameters? if not, should we have at least an "assert" here?
  struct drm_mode_create_dumb framebuffer_data;
  memset(&framebuffer_data, 0, sizeof(framebuffer_data));
  framebuffer_data.width = framebuffer->width;
  framebuffer_data.height = framebuffer->height;
  framebuffer_data.bpp = 32;
  int err =
      drmIoctl(graphics_card_fd, DRM_IOCTL_MODE_CREATE_DUMB, &framebuffer_data);
  if (err < 0) {
    fprintf(stderr, "cannot create dumb buffer (%d): %m\n", errno);
    return -errno;
  }
  framebuffer->pitch = framebuffer_data.pitch;
  framebuffer->size = framebuffer_data.size;
  framebuffer->handle = framebuffer_data.handle;
  framebuffer->bytes_per_pixel = framebuffer_data.bpp;

  // create framebuffer object for the dumb-buffer
  uint32_t handles[4] = {0};
  uint32_t pitches[4] = {0};
  uint32_t offsets[4] = {0};
  handles[0] = framebuffer->handle;
  pitches[0] = framebuffer->pitch;
  err = drmModeAddFB2(graphics_card_fd, framebuffer->width, framebuffer->height,
                      DRM_FORMAT_XRGB8888, handles, pitches, offsets,
                      &framebuffer->id, 0);
  if (err) {
    fprintf(stderr, "cannot create framebuffer (%d): %m\n", errno);
    err = -errno;

    struct drm_mode_destroy_dumb destroy_buffer_request;
    memset(&destroy_buffer_request, 0, sizeof(destroy_buffer_request));
    destroy_buffer_request.handle = framebuffer_data.handle;
    drmIoctl(graphics_card_fd, DRM_IOCTL_MODE_DESTROY_DUMB,
             &destroy_buffer_request);
    return err;
  }

  /* prepare buffer for memory mapping */
  struct drm_mode_map_dumb map_request;
  memset(&map_request, 0, sizeof(map_request));
  map_request.handle = framebuffer_data.handle;
  err = drmIoctl(graphics_card_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_request);
  if (err) {
    fprintf(stderr, "cannot map dumb buffer (%d): %m\n", errno);
    drmModeRmFB(graphics_card_fd, framebuffer->id);
    return -errno;
  }

  // perform actual memory mapping
  framebuffer->pixels = mmap(0, framebuffer_data.size, PROT_READ | PROT_WRITE,
                             MAP_SHARED, graphics_card_fd, map_request.offset);
  if (framebuffer->pixels == MAP_FAILED) {
    fprintf(stderr, "cannot mmap dumb buffer (%d): %m\n", errno);
    drmModeRmFB(graphics_card_fd, framebuffer->id);
    return -errno;
  }

  memset(framebuffer->pixels, 0, framebuffer->size);

  return 0;
}

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
        fprintf(stderr, "Encoder found: %d\n", encoder->encoder_id);
        return encoder;
      }

      drmModeFreeEncoder(encoder);
    }
  }

  return NULL;
}

int check_and_set_device_capabilities(int graphics_card_fd) {
  // We want to receive all the types of planes
  int err =
      drmSetClientCap(graphics_card_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
  if (err) {
    fprintf(stderr, "unable to set universal planes: %d\n", err);
    return err;
  }

  err = drmSetClientCap(graphics_card_fd, DRM_CLIENT_CAP_ATOMIC, 1);
  if (err) {
    fprintf(stderr, "unable to set atomic api: %d\n", err);
    return err;
  }

  uint64_t cap;
  if (drmGetCap(graphics_card_fd, DRM_CAP_DUMB_BUFFER, &cap) < 0 || !cap) {
    fprintf(stderr, "device does not support dumb buffers\n");
    return -EOPNOTSUPP;
  }

  if (drmGetCap(graphics_card_fd, DRM_CAP_CRTC_IN_VBLANK_EVENT, &cap) < 0 ||
      !cap) {
    fprintf(stderr, "device does not support the atomic api\n");
    return -EOPNOTSUPP;
  }

  return 0;
}

int add_atomic_properties_to_connector(int graphics_card_fd,
                                       drmModeAtomicReq *request,
                                       uint32_t crtc_id,
                                       uint32_t connector_id) {
  drmModeObjectProperties *props = drmModeObjectGetProperties(
      graphics_card_fd, connector_id, DRM_MODE_OBJECT_CONNECTOR);

  for (int i = 0; i < props->count_props; i++) {
    drmModePropertyRes *prop =
        drmModeGetProperty(graphics_card_fd, props->props[i]);

    if (strcmp(prop->name, "CRTC_ID") == 0) {
      drmModeAtomicAddProperty(request, connector_id, prop->prop_id, crtc_id);

      drmModeFreeProperty(prop);
      break;
    }

    drmModeFreeProperty(prop);
  }

  drmModeFreeObjectProperties(props);
  return 0;
}

int add_atomic_properties_to_crtc(int graphics_card_fd,
                                  drmModeAtomicReq *request, uint32_t crtc_id,
                                  drmModeModeInfo *modeInfo) {
  drmModeObjectProperties *props = drmModeObjectGetProperties(
      graphics_card_fd, crtc_id, DRM_MODE_OBJECT_CRTC);

  for (int i = 0; i < props->count_props; i++) {
    drmModePropertyRes *prop =
        drmModeGetProperty(graphics_card_fd, props->props[i]);

    if (strcmp(prop->name, "MODE_ID") == 0) {
      uint32_t mode_blob_id;
      int err = drmModeCreatePropertyBlob(
          graphics_card_fd, modeInfo, sizeof(drmModeModeInfo), &mode_blob_id);
      if (err != 0) {
        fprintf(stderr, "unable to create blob property\n");
        drmModeFreeProperty(prop);
        continue;
      }

      drmModeAtomicAddProperty(request, crtc_id, prop->prop_id, mode_blob_id);
    } else if (strcmp(prop->name, "ACTIVE") == 0) {
      drmModeAtomicAddProperty(request, crtc_id, prop->prop_id, 1);
    }

    drmModeFreeProperty(prop);
  }

  drmModeFreeObjectProperties(props);
  return 0;
}

int add_atomic_properties_to_plane(int graphics_card_fd,
                                   drmModeAtomicReq *request,
                                   FrameBuffer *framebuffer, uint32_t plane_id,
                                   uint32_t crtc_id) {
  drmModeObjectProperties *props = drmModeObjectGetProperties(
      graphics_card_fd, plane_id, DRM_MODE_OBJECT_PLANE);

  for (int i = 0; i < props->count_props; i++) {
    drmModePropertyRes *prop =
        drmModeGetProperty(graphics_card_fd, props->props[i]);
    uint32_t prop_id = prop->prop_id;

    // https://01.org/linuxgraphics/gfx-docs/drm/gpu/drm-kms.html#kms-properties
    uint64_t value = -1;
    // TODO(taylon): maybe write a macro for comparing strings...
    if (strcmp(prop->name, "FB_ID") == 0) {
      value = framebuffer->id;
    } else if (strcmp(prop->name, "CRTC_ID") == 0) {
      value = crtc_id;
    } else if (strcmp(prop->name, "SRC_X") == 0) {
      value = 0;
    } else if (strcmp(prop->name, "SRC_Y") == 0) {
      value = 0;
    } else if (strcmp(prop->name, "SRC_W") == 0) {
      value = framebuffer->width << 16;
    } else if (strcmp(prop->name, "SRC_H") == 0) {
      value = framebuffer->height << 16;
    } else if (strcmp(prop->name, "CRTC_X") == 0) {
      value = 0;
    } else if (strcmp(prop->name, "CRTC_Y") == 0) {
      value = 0;
    } else if (strcmp(prop->name, "CRTC_W") == 0) {
      value = framebuffer->width;
    } else if (strcmp(prop->name, "CRTC_H") == 0) {
      value = framebuffer->height;
    }

    if (value != -1) {
      drmModeAtomicAddProperty(request, plane_id, prop_id, value);
    }

    drmModeFreeProperty(prop);
  }

  drmModeFreeObjectProperties(props);
  return 0;
}

drmModeAtomicReq *create_atomic_request(int graphics_card_fd,
                                        FrameBuffer *framebuffer,
                                        drmModeModeInfo *modeInfo,
                                        uint32_t crtc_id, uint32_t plane_id,
                                        uint32_t connector_id) {
  drmModeAtomicReq *request = drmModeAtomicAlloc();

  add_atomic_properties_to_connector(graphics_card_fd, request, crtc_id,
                                     connector_id);

  add_atomic_properties_to_crtc(graphics_card_fd, request, crtc_id, modeInfo);

  add_atomic_properties_to_plane(graphics_card_fd, request, framebuffer,
                                 plane_id, crtc_id);

  return request;
}

bool is_primary_plane(int graphics_card_fd, uint32_t plane_id) {
  drmModeObjectProperties *props = drmModeObjectGetProperties(
      graphics_card_fd, plane_id, DRM_MODE_OBJECT_PLANE);

  bool is_primary = false;
  for (int i = 0; (i < props->count_props) && !is_primary; i++) {
    drmModePropertyRes *prop =
        drmModeGetProperty(graphics_card_fd, props->props[i]);

    if (strcmp(prop->name, "type") == 0 &&
        props->prop_values[i] == DRM_PLANE_TYPE_PRIMARY) {
      is_primary = true;
    }

    drmModeFreeProperty(prop);
  }

  drmModeFreeObjectProperties(props);
  return is_primary;
}

// return -1 means that there was an error or the primary plane was not found
int get_primary_plane_id(int graphics_card_fd, uint32_t *primary_plane_id) {
  bool found_primary_plane = false;
  drmModePlaneRes *planes = drmModeGetPlaneResources(graphics_card_fd);
  if (!planes) {
    fprintf(stderr, "unable to get plane resources: %s\n", strerror(errno));
    return -1;
  }

  for (int i = 0; (i < planes->count_planes) && !found_primary_plane; i++) {
    uint32_t plane_id = planes->planes[i];

    // NOTE(taylon): The primary planes should only work on one single crtc
    // (seems to be the case for cursor planes as well). So it means that the
    // primary plane does not necessarily work with our crtc, so in a real
    // application we would need to check that
    //
    // From what I understood, possible_crtcs is a bitmask where
    // each bit represents an index from resources->crtcs. So we need to figure
    // out which bit is set (if bit 1 is set then resources->crtcs[1] is the
    // crtc we are looking for) I think we could use either the
    // __builtin_ctz(plane->possible_crtcs) intrinsics or the ffs function
    // from strings.h in order to figure that out
    //
    // drmModePlane *plane = drmModeGetPlane(graphics_card_fd, plane_id);
    // if (plane->possible_crtcs & (1 << crtc_index)) { drmModeFreePlane(plane);
    if (is_primary_plane(graphics_card_fd, plane_id)) {
      *primary_plane_id = plane_id;
      found_primary_plane = true;
    }
  }

  drmModeFreePlaneResources(planes);

  if (!found_primary_plane) {
    return -1;
  }

  return 0;
}

void render(FrameBuffer *framebuffer) {
  int rect_width = 400;
  int rect_height = 200;

  for (int vertical = (framebuffer->height / 2) - (rect_height / 2);
       vertical < ((framebuffer->height / 2) + (rect_height / 2)); ++vertical) {
    for (int horizontal = ((framebuffer->width / 2) - rect_width);
         horizontal < ((framebuffer->width / 2) + rect_width); ++horizontal) {
      int offset = (framebuffer->pitch * vertical) +
                   (horizontal * (framebuffer->bytes_per_pixel / 8));
      *(uint32_t *)&framebuffer->pixels[offset] =
          (255 << 16) | (255 << 8) | 255;
    }
  }
}

typedef struct PageFlipContext {
  int front_framebuffer_index;
  FrameBuffer *framebuffers;

  // NOTE(taylon): we can probably use page_flip_handler2, which would receive
  // the crtc responsible for the event and with that we would not need to pass
  // this crtc arround like this, also if we decide to have a data structure
  // (which we probably would) holding all the data related to a
  // particular compination of crtc/plane/fd/framebuffers/etc, we could use that
  // crtc as a key, which could quickly access all the existing combinations for
  // rendering in the event handler
  uint32_t crtc_id;

  drmModeModeInfo *mode;
  uint32_t primary_plane_id;
  uint32_t connector_id;
} PageFlipContext;

void page_flip_handler(int graphics_card_fd, unsigned int frame,
                       unsigned int sec, unsigned int usec, void *data) {
  PageFlipContext *context = data;
  FrameBuffer *framebuffer =
      &context->framebuffers[context->front_framebuffer_index ^ 1];

  render(framebuffer);

  // NOTE(taylon): this code is very inneficient because inside
  // create_atomic_request we always have to call drmModeObjectGetProperties and
  // drmModeObjectGetProperty (which makes a bunch of ioctl calls) on every
  // flip. So in a real application we would probably want to obtain those
  // properties only once and then always reuse it
  drmModeAtomicReq *atomic_request = create_atomic_request(
      graphics_card_fd, framebuffer, context->mode, context->crtc_id,
      context->primary_plane_id, context->connector_id);

  int err = drmModeAtomicCommit(
      graphics_card_fd, atomic_request,
      DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK, context);
  drmModeAtomicFree(atomic_request);

  if (err)
    fprintf(stderr, "unable to perform atomic commit (%d): %m\n", errno);
  else
    context->front_framebuffer_index ^= 1;
}

int main(int argc, char *argv[]) {
  int graphics_card_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
  if (graphics_card_fd < 0) {
    fprintf(stderr, "unable to open /dev/dri/card0: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  int err = check_and_set_device_capabilities(graphics_card_fd);
  if (err) {
    return EXIT_FAILURE;
  }

  drmModeRes *resources = drmModeGetResources(graphics_card_fd);
  if (resources == NULL) {
    fprintf(stderr, "unable to retrieve DRM resources %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  drmModeConnector *connector = find_connector(graphics_card_fd, resources);
  if (connector == NULL) {
    fprintf(stderr, "no valid connector found\n");
    return EXIT_FAILURE;
  }

  drmModeEncoder *encoder =
      find_encoder_by_id(graphics_card_fd, resources, connector->encoder_id);
  if (encoder == NULL) {
    fprintf(stderr, "No enconder matching with connector\n");
    return EXIT_FAILURE;
  }

  // we save the crtc before we change it, that way we can restore
  // it later
  drmModeCrtc *crtc = drmModeGetCrtc(graphics_card_fd, encoder->crtc_id);
  if (crtc == NULL) {
    fprintf(stderr, "unable not retrieve information about the crtc: %s\n",
            strerror(errno));
    return EXIT_FAILURE;
  }

  drmModeModeInfo mode = connector->modes[0];
  fprintf(stderr, "Mode: %dx%d - %dhz\n", mode.hdisplay, mode.vdisplay,
          mode.vrefresh);
  // prints all modes
  // for (int i = 0; i < connector->count_modes; i++) {
  //  drmModeModeInfo mode = connector->modes[i];
  //  printf("(%dx%d@%dhz)\n", mode.hdisplay, mode.vdisplay,
  //          mode.vrefresh);
  // }

  // alloc two buffers so we can flip between them when rendering
  FrameBuffer framebuffers[2] = {
      {.width = mode.hdisplay, .height = mode.vdisplay},
      {.width = mode.hdisplay, .height = mode.vdisplay}};
  err = create_framebuffer(graphics_card_fd, &framebuffers[0]);
  int err_2 = create_framebuffer(graphics_card_fd, &framebuffers[1]);
  if (err || err_2) {
    fprintf(stderr, "unable to create framebuffers (%d): %m\n", errno);
    return EXIT_FAILURE;
  }

  uint32_t primary_plane_id;
  err = get_primary_plane_id(graphics_card_fd, &primary_plane_id);
  if (err) {
    fprintf(stderr, "couldn't find a primary plane\n");
    return EXIT_FAILURE;
  }

  drmModeAtomicReq *atomic_request = create_atomic_request(
      graphics_card_fd, &framebuffers[0], &mode, encoder->crtc_id,
      primary_plane_id, connector->connector_id);

  int front_framebuffer_index = 0;
  PageFlipContext page_flip_context = {
      .framebuffers = framebuffers,
      .front_framebuffer_index = front_framebuffer_index,
      .crtc_id = encoder->crtc_id,
      .mode = &mode,
      .connector_id = connector->connector_id,
      .primary_plane_id = primary_plane_id,
  };

  err = drmModeAtomicCommit(graphics_card_fd, atomic_request,
                            DRM_MODE_ATOMIC_TEST_ONLY |
                                DRM_MODE_ATOMIC_ALLOW_MODESET,
                            &page_flip_context);
  if (err < 0) {
    fprintf(stderr, "test-only atomic commit failed, %d\n", errno);
    return EXIT_FAILURE;
  }

  err = drmModeAtomicCommit(graphics_card_fd, atomic_request,
                            DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK,
                            &page_flip_context);
  drmModeAtomicFree(atomic_request);
  if (err) {
    fprintf(
        stderr,
        "unable to perform atomic commit for initial modesetting (%d): %m\n",
        errno);

    return EXIT_FAILURE;
  }

  drmEventContext drm_event_context;
  memset(&drm_event_context, 0, sizeof(drm_event_context));
  drm_event_context.version = DRM_EVENT_CONTEXT_VERSION;
  drm_event_context.page_flip_handler = page_flip_handler;

  fd_set fds;
  FD_ZERO(&fds);
  while (1) {
    FD_SET(STDIN_FILENO, &fds);
    FD_SET(graphics_card_fd, &fds);

    struct timeval timeout = {.tv_sec = 3, .tv_usec = 0};
    int number_of_fds =
        select(graphics_card_fd + 1, &fds, NULL, NULL, &timeout);
    if (number_of_fds < 0) {
      fprintf(stderr, "select() failed  %d: %m\n", errno);
      break;
    } else if (FD_ISSET(STDIN_FILENO, &fds)) {
      fprintf(stderr, "exiting...\n");
      break;
    } else if (FD_ISSET(graphics_card_fd, &fds)) {
      drmHandleEvent(graphics_card_fd, &drm_event_context);
    }
  }

  // restore CRTC configuration
  drmModeSetCrtc(graphics_card_fd, crtc->crtc_id, crtc->buffer_id, crtc->x,
                 crtc->y, &connector->connector_id, 1, &crtc->mode);

  return EXIT_SUCCESS;
}
