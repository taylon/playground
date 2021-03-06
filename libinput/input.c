#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libudev.h>
#include <libinput.h>
#include <xkbcommon/xkbcommon.h>

static int open_restricted(const char *path, int flags, void *user_data) {
  int fd = open(path, flags);

  if (fd < 0) {
    fprintf(stderr, "Failed to open %s (%s)\n", path, strerror(errno));

    return -errno;
  }

  return fd;
}

static void close_restricted(int fd, void *user_data) { close(fd); }

const static struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

void handle_keyboard_key(struct libinput_event *event,
                         struct xkb_state *xkb_state) {
  struct libinput_event_keyboard *keyboard_event =
      libinput_event_get_keyboard_event(event);

  uint32_t keycode = libinput_event_keyboard_get_key(keyboard_event);

  enum libinput_key_state state =
      libinput_event_keyboard_get_key_state(keyboard_event);

  char utf8_key[128];
  // STUDY(taylon): why does the keycode need to be + 8?
  xkb_state_key_get_utf8(xkb_state, keycode + 8, utf8_key, sizeof(utf8_key));

  switch (state) {
  case LIBINPUT_KEY_STATE_RELEASED:;
    printf("Released '%s'\n", utf8_key);
    break;

  case LIBINPUT_KEY_STATE_PRESSED:
    printf("Pressed '%s'\n", utf8_key);
    break;
  }
}

int main(void) {
  struct udev *udev = udev_new();
  if (!udev) {
    puts("Unable to create udev context");
  }

  struct libinput *li_context =
      libinput_udev_create_context(&interface, NULL, udev);
  if (!li_context) {
    puts("Unable to create libinput context");
  }

  udev_unref(udev);

  libinput_udev_assign_seat(li_context, "seat0");

  struct xkb_rule_names rules = {0};
  struct xkb_context *xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!xkb_context) {
    puts("Unable to create xkb context");
  }
  struct xkb_keymap *keymap =
      xkb_map_new_from_names(xkb_context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);
  struct xkb_state *xkb_state = xkb_state_new(keymap);

  for (;;) {
    if (libinput_dispatch(li_context) != 0) {
      fprintf(stderr, "libinput_dispatch failed: %s\n", strerror(errno));
      return 1;
    }

    struct libinput_event *event;
    while ((event = libinput_get_event(li_context))) {
      struct libinput_device *libinput_dev = libinput_event_get_device(event);
      enum libinput_event_type event_type = libinput_event_get_type(event);

      switch (event_type) {
      case LIBINPUT_EVENT_DEVICE_ADDED:;
        printf("Device added: %s\n", libinput_device_get_name(libinput_dev));
        break;

      case LIBINPUT_EVENT_POINTER_MOTION:;
        puts("Pointer Motion");
        break;

      case LIBINPUT_EVENT_KEYBOARD_KEY:
        handle_keyboard_key(event, xkb_state);

        break;

      default:
        fprintf(stderr, "Unknown event: %d\n", event_type);
        break;
      }

      libinput_event_destroy(event);
    }
  }

  libinput_unref(li_context);

  return 0;
}
