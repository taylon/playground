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

void draw(unsigned char *addr, int width, int height, int pitch) {}

static uint8_t next_color(bool *up, uint8_t current, unsigned int mod) {
  uint8_t next;

  next = current + (*up ? 1 : -1) * (rand() % mod);
  if ((*up && next < current) || (!*up && next > current)) {
    *up = !*up;
    next = current;
  }

  return next;
}

int main(int argc, char *argv[]) {
  int videoCardFd = open("/dev/dri/card1", O_RDWR);
  if (videoCardFd < 0) {
    fprintf(stderr, "unable to open /dev/dri/card1: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  drmModeRes *resources = drmModeGetResources(videoCardFd);
  if (resources == NULL) {
    fprintf(stderr, "unable to retrieve DRM resources %s\n", strerror(errno));
    drmClose(videoCardFd);
    exit(EXIT_FAILURE);
  }

  // find the first available connector with modes
  drmModeConnector *connector = NULL;
  for (int i = 0; i < resources->count_connectors; ++i) {
    connector = drmModeGetConnector(videoCardFd, resources->connectors[i]);
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
  // prints all modes
  /* for (int i = 0; i < connector->count_modes; i++) { */
  /*   drmModeModeInfo mode = connector->modes[i]; */
  /*   printf("(%dx%d@%dhz)\n", mode.hdisplay, mode.vdisplay, */
  /*           mode.vrefresh); */
  /* } */

  // find the encoder matching the connector we found previously
  drmModeEncoder *encoder = NULL;
  for (int i = 0; i < resources->count_encoders; ++i) {
    encoder = drmModeGetEncoder(videoCardFd, resources->encoders[i]);
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

  // create dumb buffer
  struct drm_mode_create_dumb framebuffer_data;
  memset(&framebuffer_data, 0, sizeof(framebuffer_data));
  framebuffer_data.width = mode.hdisplay;
  framebuffer_data.height = mode.vdisplay;
  framebuffer_data.bpp = 32;

  int err =
      drmIoctl(videoCardFd, DRM_IOCTL_MODE_CREATE_DUMB, &framebuffer_data);
  if (err < 0) {
    fprintf(stderr, "cannot create dumb buffer (%d): %m\n", errno);
    return -errno;
  }

  // create framebuffer object for the dumb-buffer
  uint32_t framebuffer_id;
  err = drmModeAddFB(videoCardFd, mode.hdisplay, mode.vdisplay, 24,
                     framebuffer_data.bpp, framebuffer_data.pitch,
                     framebuffer_data.handle, &framebuffer_id);
  if (err) {
    fprintf(stderr, "cannot create framebuffer (%d): %m\n", errno);
    err = -errno;

    struct drm_mode_destroy_dumb destroy_buffer_request;
    memset(&destroy_buffer_request, 0, sizeof(destroy_buffer_request));
    destroy_buffer_request.handle = framebuffer_data.handle;
    drmIoctl(videoCardFd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_buffer_request);
    return err;
  }

  /* prepare buffer for memory mapping */
  struct drm_mode_map_dumb map_request;
  memset(&map_request, 0, sizeof(map_request));
  map_request.handle = framebuffer_data.handle;
  err = drmIoctl(videoCardFd, DRM_IOCTL_MODE_MAP_DUMB, &map_request);
  if (err) {
    fprintf(stderr, "cannot map dumb buffer (%d): %m\n", errno);
    err = -errno;
    drmModeRmFB(videoCardFd, framebuffer_id);
  }

  // perform actual memory mapping
  uint8_t *framebuffer = mmap(0, framebuffer_data.size, PROT_READ | PROT_WRITE,
                              MAP_SHARED, videoCardFd, map_request.offset);
  if (framebuffer == MAP_FAILED) {
    fprintf(stderr, "cannot mmap dumb buffer (%d): %m\n", errno);
    err = -errno;
    drmModeRmFB(videoCardFd, framebuffer_id);
  }
  memset(framebuffer, 0, framebuffer_data.size);

  // modeset
  // we save the crtc before we change it, that way we can restore it later
  drmModeCrtcPtr crtc = drmModeGetCrtc(videoCardFd, encoder->crtc_id);
  if (crtc == NULL) {
    fprintf(stderr, "unable not retrieve information about the crtc: %s\n",
            strerror(errno));

    exit(EXIT_FAILURE);
  }

  err = drmModeSetCrtc(videoCardFd, encoder->crtc_id, framebuffer_id, 0, 0,
                       &connector->connector_id,
                       1, // element count of the connectors array above
                       &mode);
  if (err) {
    fprintf(stderr, "modesetting failed: %s\n", strerror(errno));
    drmModeRmFB(videoCardFd, framebuffer_id);
    exit(EXIT_FAILURE);
  }

  srand(time(NULL));
  uint8_t r = rand() % 0xff;
  uint8_t g = rand() % 0xff;
  uint8_t b = rand() % 0xff;
  bool r_up = true;
  bool g_up = true;
  bool b_up = true;

  for (int i = 0; i < 10; ++i) {
    r = next_color(&r_up, r, 20);
    g = next_color(&g_up, g, 10);
    b = next_color(&b_up, b, 5);

    for (int vertical = 0; vertical < mode.vdisplay; ++vertical) {
      for (int horizontal = 0; horizontal < (mode.hdisplay / 2); ++horizontal) {
        // NOTE(taylon): see the notebook for the full reasoning about this
        // whole thing.
        // calculating the offset: (pitch * x) + (y * (bpp / 8))
        int offset = (framebuffer_data.pitch * vertical) +
                     (horizontal * (framebuffer_data.bpp / 8));
        *(uint32_t *)&framebuffer[offset] = (r << 16) | (g << 8) | b;
        /* framebuffer[off] = (r << 16) | (g << 8) | b; */
      }
    }

    sleep(1);
  }

  // cleanup
  // restore CRTC configuration
  drmModeSetCrtc(videoCardFd, encoder->crtc_id, crtc->buffer_id, crtc->x,
                 crtc->y, &connector->connector_id, 1, &crtc->mode);
  drmModeFreeCrtc(crtc);

  // unmap framebuffer and then delete it
  munmap(framebuffer, framebuffer_data.size);
  drmModeRmFB(videoCardFd, framebuffer_id);

  // delete dumb buffer
  struct drm_mode_destroy_dumb dreq;
  memset(&dreq, 0, sizeof(dreq));
  dreq.handle = framebuffer_data.handle;
  drmIoctl(videoCardFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);

  exit(EXIT_SUCCESS);
}
