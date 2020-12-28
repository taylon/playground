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

int create_framebuffer(int video_card_fd, FrameBuffer *framebuffer) {
  // TODO(taylon): we rely on width and height to exist on frame_buffer by the
  // time that this function is called, should we get width and height as
  // parameters? if not, should we have at least an "assert" here?
  struct drm_mode_create_dumb framebuffer_data;
  memset(&framebuffer_data, 0, sizeof(framebuffer_data));
  framebuffer_data.width = framebuffer->width;
  framebuffer_data.height = framebuffer->height;
  framebuffer_data.bpp = 32;

  int err =
      drmIoctl(video_card_fd, DRM_IOCTL_MODE_CREATE_DUMB, &framebuffer_data);
  if (err < 0) {
    fprintf(stderr, "cannot create dumb buffer (%d): %m\n", errno);
    return -errno;
  }

  framebuffer->pitch = framebuffer_data.pitch;
  framebuffer->size = framebuffer_data.size;
  framebuffer->handle = framebuffer_data.handle;
  framebuffer->bytes_per_pixel = framebuffer_data.bpp;

  // create framebuffer object for the dumb-buffer
  err = drmModeAddFB(video_card_fd, framebuffer->width, framebuffer->height, 24,
                     framebuffer->bytes_per_pixel, framebuffer->pitch,
                     framebuffer->handle, &framebuffer->id);
  if (err) {
    fprintf(stderr, "cannot create framebuffer (%d): %m\n", errno);
    err = -errno;

    struct drm_mode_destroy_dumb destroy_buffer_request;
    memset(&destroy_buffer_request, 0, sizeof(destroy_buffer_request));
    destroy_buffer_request.handle = framebuffer_data.handle;
    drmIoctl(video_card_fd, DRM_IOCTL_MODE_DESTROY_DUMB,
             &destroy_buffer_request);
    return err;
  }

  /* prepare buffer for memory mapping */
  struct drm_mode_map_dumb map_request;
  memset(&map_request, 0, sizeof(map_request));
  map_request.handle = framebuffer_data.handle;
  err = drmIoctl(video_card_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_request);
  if (err) {
    fprintf(stderr, "cannot map dumb buffer (%d): %m\n", errno);
    drmModeRmFB(video_card_fd, framebuffer->id);
    return -errno;
  }

  // perform actual memory mapping
  framebuffer->pixels = mmap(0, framebuffer_data.size, PROT_READ | PROT_WRITE,
                             MAP_SHARED, video_card_fd, map_request.offset);
  if (framebuffer->pixels == MAP_FAILED) {
    fprintf(stderr, "cannot mmap dumb buffer (%d): %m\n", errno);
    drmModeRmFB(video_card_fd, framebuffer->id);
    return -errno;
  }
  memset(framebuffer->pixels, 0, framebuffer->size);

  return 0;
}

drmModeConnector *find_connector(int video_card_fd, drmModeRes *resources) {
  // find the first available connector with modes
  drmModeConnector *connector;
  for (int i = 0; i < resources->count_connectors; ++i) {
    connector = drmModeGetConnector(video_card_fd, resources->connectors[i]);
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

drmModeEncoder *find_encoder_by_id(int video_card_fd, drmModeRes *resources,
                                   uint32_t encoder_id) {
  drmModeEncoder *encoder;
  for (int i = 0; i < resources->count_encoders; ++i) {
    encoder = drmModeGetEncoder(video_card_fd, resources->encoders[i]);
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

  uint32_t crtc_id;
} PageFlipContext;

void page_flip_handler(int video_card_fd, unsigned int frame, unsigned int sec,
                       unsigned int usec, void *data) {
  PageFlipContext *context = data;
  FrameBuffer *framebuffer =
      &context->framebuffers[context->front_framebuffer_index ^ 1];

  render(framebuffer);

  int err = drmModePageFlip(video_card_fd, context->crtc_id, framebuffer->id,
                            DRM_MODE_PAGE_FLIP_EVENT, context);
  if (err)
    fprintf(stderr, "unable to flip buffers (%d): %m\n", errno);
  else
    context->front_framebuffer_index ^= 1;
}

int main(int argc, char *argv[]) {
  int video_card_fd = open("/dev/dri/card0", O_RDWR);
  if (video_card_fd < 0) {
    fprintf(stderr, "unable to open /dev/dri/card0: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  drmModeRes *resources = drmModeGetResources(video_card_fd);
  if (resources == NULL) {
    fprintf(stderr, "unable to retrieve DRM resources %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  drmModeConnector *connector = find_connector(video_card_fd, resources);
  if (connector == NULL) {
    fprintf(stderr, "no valid connector found\n");
    return EXIT_FAILURE;
  }

  drmModeModeInfo mode = connector->modes[1];
  fprintf(stderr, "Mode: %dx%d - %dhz\n", mode.hdisplay, mode.vdisplay,
          mode.vrefresh);
  // prints all modes
  // for (int i = 0; i < connector->count_modes; i++) {
  //  drmModeModeInfo mode = connector->modes[i];
  //  printf("(%dx%d@%dhz)\n", mode.hdisplay, mode.vdisplay,
  //          mode.vrefresh);
  // }

  drmModeEncoder *encoder =
      find_encoder_by_id(video_card_fd, resources, connector->encoder_id);
  if (encoder == NULL) {
    fprintf(stderr, "No enconder matching with connector\n");
    return EXIT_FAILURE;
  }

  // alloc two buffers so we can flip between them when rendering
  FrameBuffer framebuffers[2] = {
      {.width = mode.hdisplay, .height = mode.vdisplay},
      {.width = mode.hdisplay, .height = mode.vdisplay}};
  int err = create_framebuffer(video_card_fd, &framebuffers[0]);
  int err_2 = create_framebuffer(video_card_fd, &framebuffers[1]);
  if (err || err_2) {
    fprintf(stderr, "unable to create framebuffers (%d): %m\n", errno);
    return EXIT_FAILURE;
  }

  // modeset
  // we save the crtc before we change it, that way we can restore
  // it later
  drmModeCrtcPtr original_crtc =
      drmModeGetCrtc(video_card_fd, encoder->crtc_id);
  if (original_crtc == NULL) {
    fprintf(stderr, "unable not retrieve information about the crtc: %s\n",
            strerror(errno));

    return EXIT_FAILURE;
  }

  int front_framebuffer_index = 0;
  err = drmModeSetCrtc(video_card_fd, original_crtc->crtc_id,
                       framebuffers[front_framebuffer_index].id, 0, 0,
                       &connector->connector_id,
                       1, // element count of the connectors array above
                       &mode);
  if (err) {
    fprintf(stderr, "modesetting failed: %s\n", strerror(errno));
    drmModeRmFB(video_card_fd, framebuffers[front_framebuffer_index].id);
    return EXIT_FAILURE;
  }

  PageFlipContext page_flip_context = {
      .crtc_id = original_crtc->crtc_id,
      .framebuffers = framebuffers,
      .front_framebuffer_index = front_framebuffer_index,
  };
  err = drmModePageFlip(video_card_fd, encoder->crtc_id,
                        framebuffers[front_framebuffer_index].id,
                        DRM_MODE_PAGE_FLIP_EVENT, &page_flip_context);
  if (err) {
    fprintf(stderr, "unable to flip buffers (%d): %m\n", errno);
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
    FD_SET(video_card_fd, &fds);

    struct timeval timeout = {.tv_sec = 3, .tv_usec = 0};
    int number_of_fds = select(video_card_fd + 1, &fds, NULL, NULL, &timeout);
    if (number_of_fds < 0) {
      fprintf(stderr, "select() failed  %d: %m\n", errno);
      break;
    } else if (FD_ISSET(STDIN_FILENO, &fds)) {
      fprintf(stderr, "exiting...\n");
      break;
    } else if (FD_ISSET(video_card_fd, &fds)) {
      drmHandleEvent(video_card_fd, &drm_event_context);
    }
  }

  // cleanup
  // restore CRTC configuration
  drmModeSetCrtc(video_card_fd, original_crtc->crtc_id,
                 original_crtc->buffer_id, original_crtc->x, original_crtc->y,
                 &connector->connector_id, 1, &original_crtc->mode);
  drmModeFreeCrtc(original_crtc);

  return EXIT_SUCCESS;
}
