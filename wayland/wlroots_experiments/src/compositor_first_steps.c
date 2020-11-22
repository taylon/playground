#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <wayland-server.h>

#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_output.h>
#include <wlr/render/wlr_renderer.h>

typedef struct Server {
  struct wl_display *wl_display;
  struct wl_event_loop *wl_event_loop;

  struct wl_listener new_output;
  struct wl_list outputs;

  struct wlr_backend *backend;
  struct wlr_compositor *compositor;
} Server;

typedef struct Output {
  struct wlr_output *wlr_output;
  Server *server;
  struct timespec last_frame;

  struct wl_listener destroy;
  struct wl_listener frame;

  struct wl_list link;
} Output;

static void output_destroy_notify(struct wl_listener *listener, void *data) {
  Output *output = wl_container_of(listener, output, destroy);

  wl_list_remove(&output->link);
  wl_list_remove(&output->destroy.link);
  wl_list_remove(&output->frame.link);
  free(output);
}

static void output_frame_notify(struct wl_listener *listener, void *data) {
  Output *output = wl_container_of(listener, output, frame);
  Server *server = output->server;
  struct wlr_output *wlr_output = data;

  struct wlr_renderer *renderer = wlr_backend_get_renderer(wlr_output->backend);

  if (!wlr_output_attach_render(output->wlr_output, NULL)) {
    return;
  }

  int width, height;
  wlr_output_effective_resolution(output->wlr_output, &width, &height);

  wlr_renderer_begin(renderer, width, height);

  struct wl_resource *_surface;
  struct wlr_surface test;
  wl_resource_for_each(_surface, &server->compositor->surfaces) {
    struct wlr_surface *surface = wlr_surface_from_resource(_surface);
    if (!wlr_surface_has_buffer(surface)) {
      continue;
    }

    struct wlr_box render_box = {.x = 20,
                                 .y = 20,
                                 .width = surface->current.width,
                                 .height = surface->current.height};

    float matrix[16];
    wlr_matrix_project_box(matrix, &render_box, surface->current.transform, 0,
                           wlr_output->transform_matrix);
    /* wlr_render_with_matrix(renderer, surface->texture, &matrix, 1.0f); */
    wlr_render_texture_with_matrix(renderer, surface->texture, matrix, 1.0f);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_surface_send_frame_done(surface, &now);
  }

  float color[4] = {0, 0, 0, 1.0};
  wlr_renderer_clear(renderer, color);

  wlr_renderer_end(renderer);
  wlr_output_commit(output->wlr_output);
}

static void new_output_notify(struct wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, new_output);
  struct wlr_output *wlr_output = data;

  if (!wl_list_empty(&wlr_output->modes)) {
    struct wlr_output_mode *mode =
        wl_container_of(wlr_output->modes.prev, mode, link);

    wlr_output_set_mode(wlr_output, mode);
  }

  Output *output = calloc(1, sizeof(Output));
  clock_gettime(CLOCK_MONOTONIC, &output->last_frame);
  output->server = server;
  output->wlr_output = wlr_output;

  wl_list_insert(&server->outputs, &output->link);

  output->destroy.notify = output_destroy_notify;
  wl_signal_add(&wlr_output->events.destroy, &output->destroy);

  output->frame.notify = output_frame_notify;
  wl_signal_add(&wlr_output->events.frame, &output->frame);

  wlr_output_create_global(wlr_output);
}

int main(int argc, char **argv) {
  Server server;

  server.wl_display = wl_display_create();
  assert(server.wl_display);

  server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
  assert(server.wl_event_loop);

  server.backend = wlr_backend_autocreate(server.wl_display, NULL);
  assert(server.backend);

  wl_list_init(&server.outputs);
  server.new_output.notify = new_output_notify;
  wl_signal_add(&server.backend->events.new_output, &server.new_output);

  const char *socket = wl_display_add_socket_auto(server.wl_display);
  assert(socket);

  if (!wlr_backend_start(server.backend)) {
    fprintf(stderr, "failed to start the backend\n");
    wl_display_destroy(server.wl_display);

    return 1;
  }

  printf("Display: '%s'\n", socket);
  setenv("WAYLAND_DISPLAY", socket, true);

  wl_display_init_shm(server.wl_display);
  wlr_idle_create(server.wl_display);

  server.compositor = wlr_compositor_create(
      server.wl_display, wlr_backend_get_renderer(server.backend));

  wlr_xdg_shell_create(server.wl_display);

  wl_display_run(server.wl_display);
  wl_display_destroy(server.wl_display);

  return 0;
}
