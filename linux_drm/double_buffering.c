#define _GNU_SOURCE
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#define M_PI 3.14159265358979323846

static uint8_t next_color(bool *up, uint8_t current, unsigned int mod) {
  uint8_t next;

  next = current + (*up ? 1 : -1) * (rand() % mod);
  if ((*up && next < current) || (!*up && next > current)) {
    *up = !*up;
    next = current;
  }

  return next;
}

typedef struct Display {
  uint32_t width;
  uint32_t height;
} Display;

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

void destroy_framebuffer(int video_card_fd, FrameBuffer *framebuffer) {
  munmap(framebuffer, framebuffer->size);
  drmModeRmFB(video_card_fd, framebuffer->id);

  struct drm_mode_destroy_dumb dreq;
  memset(&dreq, 0, sizeof(dreq));
  dreq.handle = framebuffer->handle;

  drmIoctl(video_card_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
}

int main(int argc, char *argv[]) {
  int video_card_fd = open("/dev/dri/card1", O_RDWR);
  if (video_card_fd < 0) {
    fprintf(stderr, "unable to open /dev/dri/card1: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  drmModeRes *resources = drmModeGetResources(video_card_fd);
  if (resources == NULL) {
    fprintf(stderr, "unable to retrieve DRM resources %s\n", strerror(errno));
    drmClose(video_card_fd);
    exit(EXIT_FAILURE);
  }

  // find the first available connector with modes
  drmModeConnector *connector = NULL;
  for (int i = 0; i < resources->count_connectors; ++i) {
    connector = drmModeGetConnector(video_card_fd, resources->connectors[i]);
    if (connector != NULL) {
      // check if the monitor is connected and has at least one valid mode
      if (connector->connection == DRM_MODE_CONNECTED &&
          connector->count_modes > 0) {
        printf("Connector found: %d\n", connector->connector_id);
        break;
      }

      connector = NULL;
      drmModeFreeConnector(connector);
    }
  }

  if (connector == NULL) {
    fprintf(stderr, "no valid connector found\n");
    drmModeFreeResources(resources);
    exit(EXIT_FAILURE);
  }

  drmModeModeInfo mode = connector->modes[1];
  fprintf(stderr, "Mode: %dx%d - %dhz\n", mode.hdisplay, mode.vdisplay,
          mode.vrefresh);
  Display display = {.height = mode.vdisplay, .width = mode.hdisplay};
  // prints all modes
  /* for (int i = 0; i < connector->count_modes; i++) { */
  /*   drmModeModeInfo mode = connector->modes[i]; */
  /*   printf("(%dx%d@%dhz)\n", mode.hdisplay, mode.vdisplay, */
  /*           mode.vrefresh); */
  /* } */

  // find the encoder matching the connector we found previously
  drmModeEncoder *encoder = NULL;
  for (int i = 0; i < resources->count_encoders; ++i) {
    encoder = drmModeGetEncoder(video_card_fd, resources->encoders[i]);
    if (encoder != NULL) {
      fprintf(stderr, "Encoder found: %d\n", encoder->encoder_id);

      if (encoder->encoder_id == connector->encoder_id)
        break;

      drmModeFreeEncoder(encoder);
    }
  }

  if (encoder == NULL) {
    fprintf(stderr, "No enconder matching with connector\n");
    drmModeFreeResources(resources);
    exit(EXIT_FAILURE);
  }

  // alloc two buffers so we can flip between them when rendering
  FrameBuffer *framebuffers = calloc(2, sizeof(FrameBuffer));
  framebuffers[0].height = display.height;
  framebuffers[1].height = display.height;
  framebuffers[0].width = display.width;
  framebuffers[1].width = display.width;
  int err = create_framebuffer(video_card_fd, &framebuffers[0]);
  int err_2 = create_framebuffer(video_card_fd, &framebuffers[1]);
  if (err || err_2) {
    fprintf(stderr, "unable to create framebuffers (%d): %m\n", errno);
    exit(EXIT_FAILURE);
  }
  int framebuffer_index = 0;

  // modeset
  // we save the crtc before we change it, that way we can restore
  // it later
  drmModeCrtcPtr crtc = drmModeGetCrtc(video_card_fd, encoder->crtc_id);
  if (crtc == NULL) {
    fprintf(stderr, "unable not retrieve information about the crtc: %s\n",
            strerror(errno));

    exit(EXIT_FAILURE);
  }

  err = drmModeSetCrtc(video_card_fd, encoder->crtc_id,
                       framebuffers[framebuffer_index].id, 0, 0,
                       &connector->connector_id,
                       1, // element count of the connectors array above
                       &mode);
  if (err) {
    fprintf(stderr, "modesetting failed: %s\n", strerror(errno));
    drmModeRmFB(video_card_fd, framebuffers[framebuffer_index].id);
    exit(EXIT_FAILURE);
  }

  srand(time(NULL));
  uint8_t r = rand() % 0xff;
  uint8_t g = rand() % 0xff;
  uint8_t b = rand() % 0xff;
  bool r_up = true;
  bool g_up = true;
  bool b_up = true;

  int rect_width = 400;
  int rect_height = 200;

  FrameBuffer *framebuffer;
  for (int vertical_position = 0; vertical_position < (display.height - 200);
       vertical_position += 20) {
    framebuffer = &framebuffers[framebuffer_index ^ 1];

    memset(framebuffer->pixels, 1, framebuffer->size);

    r = next_color(&r_up, r, 20);
    g = next_color(&g_up, g, 10);
    b = next_color(&b_up, b, 5);

    for (int vertical = vertical_position;
         vertical < (vertical_position + rect_height); ++vertical) {
      for (int horizontal = ((display.height / 2) - rect_width);
           horizontal < ((display.width / 2) + rect_width); ++horizontal) {
        int offset = (framebuffer->pitch * vertical) +
                     (horizontal * (framebuffer->bytes_per_pixel / 8));
        *(uint32_t *)&framebuffer->pixels[offset] = (r << 16) | (g << 8) | b;
      }
    }

    err = drmModeSetCrtc(video_card_fd, encoder->crtc_id, framebuffer->id, 0, 0,
                         &connector->connector_id, 1, &mode);
    if (err)
      fprintf(stderr, "unable to flip buffers (%d): %m\n", errno);
    else
      framebuffer_index ^= 1;

    usleep(100000);
  }

  // cleanup
  // restore CRTC configuration
  drmModeSetCrtc(video_card_fd, crtc->crtc_id, crtc->buffer_id, crtc->x,
                 crtc->y, &connector->connector_id, 1, &crtc->mode);
  drmModeFreeCrtc(crtc);

  destroy_framebuffer(video_card_fd, &framebuffers[0]);
  destroy_framebuffer(video_card_fd, &framebuffers[1]);

  exit(EXIT_SUCCESS);
}
