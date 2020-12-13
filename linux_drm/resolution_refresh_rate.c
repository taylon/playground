#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

int main(int argc, char *argv[]) {
  int videoCardFd = open("/dev/dri/card0", O_RDWR);
  if (videoCardFd < 0) {
    fprintf(stderr, "unable to open /dev/dri/card0: %s\n", strerror(errno));
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
      printf("Connector found: %d\n", connector->connector_id);

      // check if the monitor is connected and has at least one valid mode
      if (connector->connection == DRM_MODE_CONNECTED &&
          connector->count_modes > 0) {
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

  for (int i = 0; i < connector->count_modes; i++) {
    drmModeModeInfo mode = connector->modes[i];
    fprintf(stderr, "(%dx%d@%dhz)\n", mode.hdisplay, mode.vdisplay,
            mode.vrefresh);
  }
}
