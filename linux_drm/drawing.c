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

#include <libkms.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#define M_PI 3.14159265358979323846

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

int create_framebuffer(int video_card_fd, struct kms_driver *kms_driver,
                       FrameBuffer *framebuffer) {
  // TODO(taylon): we rely on width and height to exist on frame_buffer by the
  // time that this function is called, should we get width and height as
  // parameters? if not, should we have at least an "assert" here?
  struct kms_bo *buffer_object;
  unsigned buffer_object_attributes[] = {KMS_WIDTH,
                                         framebuffer->width,
                                         KMS_HEIGHT,
                                         framebuffer->height,
                                         KMS_BO_TYPE,
                                         KMS_BO_TYPE_SCANOUT_X8R8G8B8,
                                         KMS_TERMINATE_PROP_LIST};
  if (kms_bo_create(kms_driver, buffer_object_attributes, &buffer_object)) {
    fprintf(stderr, "unable to create buffer object: %s\n", strerror(errno));
    return -errno;
  }

  if (kms_bo_get_prop(buffer_object, KMS_PITCH, &framebuffer->pitch)) {
    fprintf(stderr, "unable to get framebuffer pitch: %s\n", strerror(errno));
    kms_bo_destroy(&buffer_object);
    return -errno;
  }

  if (kms_bo_get_prop(buffer_object, KMS_HANDLE, &framebuffer->handle)) {
    fprintf(stderr, "unable to get the framebuffer handle: %s\n",
            strerror(errno));
    kms_bo_destroy(&buffer_object);
    return -errno;
  }

  // map the bo to user space buffer
  if (kms_bo_map(buffer_object, (void *)&framebuffer->pixels)) {
    fprintf(stderr, "unable to map the framebuffer: %s\n", strerror(errno));
    kms_bo_destroy(&buffer_object);
    return -errno;
  }

  kms_bo_unmap(buffer_object);

  // create framebuffer object for the dumb-buffer
  framebuffer->bytes_per_pixel = 32;
  int err = drmModeAddFB(video_card_fd, framebuffer->width, framebuffer->height,
                         24, framebuffer->bytes_per_pixel, framebuffer->pitch,
                         framebuffer->handle, &framebuffer->id);
  if (err) {
    fprintf(stderr, "cannot create framebuffer (%d): %m\n", errno);
    return -errno;
  }

  // TODO(taylon): should libkms expose the framebuffer size? That would be
  // great for my little experiment here but I have not idea if that is a real
  // use case
  framebuffer->size =
      framebuffer->pitch * ((framebuffer->height + 4 - 1) & ~(4 - 1));

  return 0;
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

  // kms_create will use the correct driver depending on our gpu
  // the driver is used later to create framebuffers
  struct kms_driver *kms_driver;
  if (kms_create(video_card_fd, &kms_driver)) {
    fprintf(stderr, "unable to create kms driver: %s\n", strerror(errno));
    drmModeFreeResources(resources);
    exit(EXIT_FAILURE);
  }

  // alloc two buffers so we can flip between them when rendering
  FrameBuffer *framebuffers = calloc(2, sizeof(FrameBuffer));
  framebuffers[0].height = display.height;
  framebuffers[1].height = display.height;
  framebuffers[0].width = display.width;
  framebuffers[1].width = display.width;
  int err = create_framebuffer(video_card_fd, kms_driver, &framebuffers[0]);
  int err_2 = create_framebuffer(video_card_fd, kms_driver, &framebuffers[1]);
  if (err || err_2) {
    fprintf(stderr, "unable to create framebuffers (%d): %m\n", errno);
    exit(EXIT_FAILURE);
  }
  int front_framebuffer_index = 0;

  // we save the crtc before we change it, that way we can restore
  // it later
  drmModeCrtcPtr crtc = drmModeGetCrtc(video_card_fd, encoder->crtc_id);
  if (crtc == NULL) {
    fprintf(stderr, "unable not retrieve information about the crtc: %s\n",
            strerror(errno));

    exit(EXIT_FAILURE);
  }

  err = drmModeSetCrtc(video_card_fd, encoder->crtc_id,
                       framebuffers[front_framebuffer_index].id, 0, 0,
                       &connector->connector_id,
                       1, // element count of the connectors array above
                       &mode);
  if (err) {
    fprintf(stderr, "modesetting failed: %s\n", strerror(errno));
    drmModeRmFB(video_card_fd, framebuffers[front_framebuffer_index].id);
    exit(EXIT_FAILURE);
  }

  srand(time(NULL));
  uint8_t r = rand() % 0xff;
  uint8_t g = rand() % 0xff;
  uint8_t b = rand() % 0xff;

  int rect_width = 400;
  int rect_height = 200;

  FrameBuffer *back_framebuffer;
  for (int vertical_position = 0; vertical_position < (display.height - 200);
       vertical_position += 20) {
    back_framebuffer = &framebuffers[front_framebuffer_index ^ 1];
    memset(back_framebuffer->pixels, 1, back_framebuffer->size);

    for (int vertical = vertical_position;
         vertical < (vertical_position + rect_height); ++vertical) {
      for (int horizontal = ((display.width / 2) - rect_width);
           horizontal < ((display.width / 2) + rect_width); ++horizontal) {
        int offset = (back_framebuffer->pitch * vertical) +
                     (horizontal * (back_framebuffer->bytes_per_pixel / 8));
        *(uint32_t *)&back_framebuffer->pixels[offset] =
            (r << 16) | (g << 8) | b;
      }
    }

    err = drmModeSetCrtc(video_card_fd, encoder->crtc_id, back_framebuffer->id,
                         0, 0, &connector->connector_id, 1, &mode);
    if (err)
      fprintf(stderr, "unable to flip buffers (%d): %m\n", errno);
    else
      front_framebuffer_index ^= 1;

    usleep(100000);
  }

  // cleanup
  // restore CRTC configuration
  drmModeSetCrtc(video_card_fd, crtc->crtc_id, crtc->buffer_id, crtc->x,
                 crtc->y, &connector->connector_id, 1, &crtc->mode);
  drmModeFreeCrtc(crtc);

  exit(EXIT_SUCCESS);
}
