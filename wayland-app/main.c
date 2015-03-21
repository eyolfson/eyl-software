// Copyright 2015 Jonathan Eyolfson
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.

#include <stdio.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <linux/memfd.h>
#include <sys/mman.h>

#include "xdg_shell.h"

static struct wl_compositor *wl_compositor = NULL;
static struct wl_shm *wl_shm = NULL;
static struct xdg_shell *xdg_shell = NULL;

static void global(void *data,
                   struct wl_registry *wl_registry,
                   uint32_t name,
                   const char *interface,
                   uint32_t version)
{
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        wl_compositor = wl_registry_bind(wl_registry,
                                         name,
                                         &wl_compositor_interface,
                                         version);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0) {
        wl_shm = wl_registry_bind(wl_registry,
                                  name,
                                  &wl_shm_interface,
                                  version);
    }
    else if (strcmp(interface, xdg_shell_interface.name) == 0) {
        xdg_shell = wl_registry_bind(wl_registry,
                                     name,
                                     &xdg_shell_interface,
                                     version);
    }
}

static void global_remove(void *data,
                          struct wl_registry *wl_registry,
                          uint32_t name) {}

static void ping(void *data, struct xdg_shell *xdg_shell, uint32_t serial)
{
    xdg_shell_pong(xdg_shell, serial);
}

static void xs_configure(void *data,
                      struct xdg_surface *xdg_surface,
                      int32_t width,
                      int32_t height,
                      struct wl_array *states,
                      uint32_t serial)
{
    printf("w = %d, h = %d\n", width, height);
    xdg_surface_ack_configure(xdg_surface, serial);
}

static void xs_close(void *data, struct xdg_surface *xdg_surface)
{
}

static struct wl_registry_listener registry_listener = {global, global_remove};
static struct xdg_shell_listener xdg_shell_listener = {ping};
static struct xdg_surface_listener xdg_surface_listener = {xs_configure, xs_close};

int main(int argc, char **argv)
{
    struct wl_display *wl_display = wl_display_connect(NULL);
    if (wl_display == NULL) {
        printf("wl_display failed\n");
        return 1;
    }
    struct wl_registry *wl_registry = wl_display_get_registry(wl_display);
    if (wl_registry == NULL) {
        printf("wl_registry failed\n");
    }

    wl_registry_add_listener(wl_registry, &registry_listener, NULL);
    wl_display_dispatch(wl_display);
    if (wl_compositor == NULL) {
        printf("wl_compositor failed\n");
    }
    if (wl_shm == NULL) {
        printf("wl_shm failed\n");
    }
    if (xdg_shell == NULL) {
        printf("xdg_shell failed\n");
    }
    xdg_shell_use_unstable_version(xdg_shell, XDG_SHELL_VERSION_CURRENT);

    xdg_shell_add_listener(xdg_shell, &xdg_shell_listener, NULL);



    int fd = syscall(SYS_memfd_create, "wayland-app", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd == -1) {
        goto fd_fail;
    }
    const int32_t WIDTH = 300;
    const int32_t STRIDE = WIDTH * sizeof(int32_t);
    const int32_t HEIGHT = 200;
    const int32_t CAPACITY = STRIDE * HEIGHT;
    ftruncate(fd, CAPACITY);
    uint32_t *pixels = mmap(NULL, CAPACITY, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    for (int32_t w = 0; w < WIDTH; ++w) {
        for (int32_t h = 0; h < HEIGHT; ++h) {
            pixels[h*HEIGHT+w] = 0x80FF0000;
        }
    }

    struct wl_surface *wl_surface = wl_compositor_create_surface(wl_compositor);
    if (wl_surface == NULL) {
        printf("wl_surface failed\n");
    }
    struct xdg_surface *xdg_surface = xdg_shell_get_xdg_surface(xdg_shell, wl_surface);
    if (xdg_surface == NULL) {
        printf("xdg_surface failed\n");
    }
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
    xdg_surface_set_title(xdg_surface, "Hello");
    xdg_surface_set_window_geometry(xdg_surface, 10, 10, WIDTH, HEIGHT);
    struct wl_shm_pool *wl_shm_pool = wl_shm_create_pool(wl_shm, fd, CAPACITY);
    struct wl_buffer *wl_buffer = wl_shm_pool_create_buffer(wl_shm_pool, 0,
                                                            WIDTH, HEIGHT, STRIDE,
                                                            WL_SHM_FORMAT_ARGB8888);

    wl_surface_attach(wl_surface, wl_buffer, 0, 0);
    wl_surface_commit(wl_surface);

    while (1) {
        wl_display_dispatch(wl_display);
    }

    wl_buffer_destroy(wl_buffer);
    wl_shm_pool_destroy(wl_shm_pool);
    xdg_surface_destroy(xdg_surface);
    wl_surface_destroy(wl_surface);
    munmap(pixels, CAPACITY);
    close(fd);

 fd_fail:
    xdg_shell_destroy(xdg_shell);
    wl_shm_destroy(wl_shm);
    wl_compositor_destroy(wl_compositor);
    wl_registry_destroy(wl_registry);
    wl_display_disconnect(wl_display);
    return 0;
}