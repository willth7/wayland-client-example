//   Copyright 2022 Will Thomas
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0;
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

struct wl_compositor* comp;
struct wl_surface* srfc;
struct wl_buffer* bfr;
struct wl_shm* shm;
struct xdg_wm_base* sh;
struct xdg_toplevel* top;
struct wl_seat* seat;
struct wl_keyboard* kb;
uint8_t* pixl;
uint16_t w = 200;
uint16_t h = 100;
uint8_t c;
uint8_t cls;

int32_t alc_shm(uint64_t sz) {
	int8_t name[8];
	name[0] = '/';
	name[7] = 0;
	for (uint8_t i = 1; i < 6; i++) {
		name[i] = (rand() & 23) + 97;
	}

	int32_t fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH);
	shm_unlink(name);
	ftruncate(fd, sz);

	return fd;
}

void resz() {
	int32_t fd = alc_shm(w * h * 4);

	pixl = mmap(0, w * h * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	struct wl_shm_pool* pool = wl_shm_create_pool(shm, fd, w * h * 4);
	bfr = wl_shm_pool_create_buffer(pool, 0, w, h, w * 4, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);
}

void draw() {
	memset(pixl, c, w * h * 4);

	wl_surface_attach(srfc, bfr, 0, 0);
	wl_surface_damage_buffer(srfc, 0, 0, w, h);
	wl_surface_commit(srfc);
}

struct wl_callback_listener cb_list;

void frame_new(void* data, struct wl_callback* cb, uint32_t a) {
	wl_callback_destroy(cb);
	cb = wl_surface_frame(srfc);
	wl_callback_add_listener(cb, &cb_list, 0);
	
	c++;
	draw();
}

struct wl_callback_listener cb_list = {
	.done = frame_new
};

void xrfc_conf(void* data, struct xdg_surface* xrfc, uint32_t ser) {
	xdg_surface_ack_configure(xrfc, ser);
	if (!pixl) {
		resz();
	}
	
	draw();
}

struct xdg_surface_listener xrfc_list = {
	.configure = xrfc_conf
};

void top_conf(void* data, struct xdg_toplevel* top, int32_t nw, int32_t nh, struct wl_array* stat) {
	if (!nw && !nh) {
		return;
	}

	if (w != nw || h != nh) {
		munmap(pixl, w * h * 4);
		w = nw;
		h = nh;
		resz();
	}
}

void top_cls(void* data, struct xdg_toplevel* top) {
	cls = 1;
}

struct xdg_toplevel_listener top_list = {
	.configure = top_conf,
	.close = top_cls
};

void sh_ping(void* data, struct xdg_wm_base* sh, uint32_t ser) {
	xdg_wm_base_pong(sh, ser);
}

struct xdg_wm_base_listener sh_list = {
	.ping = sh_ping
};

void kb_map(void* data, struct wl_keyboard* kb, uint32_t frmt, int32_t fd, uint32_t sz) {
	
}

void kb_enter(void* data, struct wl_keyboard* kb, uint32_t ser, struct wl_surface* srfc, struct wl_array* keys) {
	
}

void kb_leave(void* data, struct wl_keyboard* kb, uint32_t ser, struct wl_surface* srfc) {
	
}

void kb_key(void* data, struct wl_keyboard* kb, uint32_t ser, uint32_t t, uint32_t key, uint32_t stat) {
	if (key == 1) {
		cls = 1;
	}
	else if (key == 30) {
		printf("a\n");
	}
	else if (key == 32) {
		printf("d\n");
	}
}

void kb_mod(void* data, struct wl_keyboard* kb, uint32_t ser, uint32_t dep, uint32_t lat, uint32_t lock, uint32_t grp) {
	
}

void kb_rep(void* data, struct wl_keyboard* kb, int32_t rate, int32_t del) {
	
}

struct wl_keyboard_listener kb_list = {
	.keymap = kb_map,
	.enter = kb_enter,
	.leave = kb_leave,
	.key = kb_key,
	.modifiers = kb_mod,
	.repeat_info = kb_rep
};

void seat_cap(void* data, struct wl_seat* seat, uint32_t cap) {
	if (cap & WL_SEAT_CAPABILITY_KEYBOARD && !kb) {
		kb = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(kb, &kb_list, 0);
	}
}

void seat_name(void* data, struct wl_seat* seat, int8_t* name) {
		
}

struct wl_seat_listener seat_list = {
	.capabilities = seat_cap,
	.name = seat_name
};

void reg_glob(void* data, struct wl_registry* reg, uint32_t name, int8_t* intf, uint32_t v) {
	if (!strcmp(intf, wl_compositor_interface.name)) {
		comp = wl_registry_bind(reg, name, &wl_compositor_interface, 4);
	}
	else if (!strcmp(intf, wl_shm_interface.name)) {
		shm = wl_registry_bind(reg, name, &wl_shm_interface, 1);
	}
	else if (!strcmp(intf, xdg_wm_base_interface.name)) {
		sh = wl_registry_bind(reg, name, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(sh, &sh_list, 0);
	}
	else if (!strcmp(intf, wl_seat_interface.name)) {
		seat = wl_registry_bind(reg, name, &wl_seat_interface, 1);
		wl_seat_add_listener(seat, &seat_list, 0);
	}
}

void reg_glob_rem(void* data, struct wl_registry* reg, uint32_t name) {
	
}

struct wl_registry_listener reg_list = {
	.global = reg_glob,
	.global_remove = reg_glob_rem
};

int8_t main() {
	struct wl_display* disp = wl_display_connect(0);
	struct wl_registry* reg = wl_display_get_registry(disp);
	wl_registry_add_listener(reg, &reg_list, 0);
	wl_display_roundtrip(disp);

	srfc = wl_compositor_create_surface(comp);
	struct wl_callback* cb = wl_surface_frame(srfc);
	wl_callback_add_listener(cb, &cb_list, 0);

	struct xdg_surface* xrfc = xdg_wm_base_get_xdg_surface(sh, srfc);
	xdg_surface_add_listener(xrfc, &xrfc_list, 0);
	top = xdg_surface_get_toplevel(xrfc);
	xdg_toplevel_add_listener(top, &top_list, 0);
	xdg_toplevel_set_title(top, "wayland client");
	wl_surface_commit(srfc);

	while (wl_display_dispatch(disp)) {
		if (cls) break;
	}
	
	if (kb) {
		wl_keyboard_destroy(kb);
	}
	wl_seat_release(seat);
	if (bfr) {
		wl_buffer_destroy(bfr);
	}
	xdg_toplevel_destroy(top);
	xdg_surface_destroy(xrfc);
	wl_surface_destroy(srfc);
	wl_display_disconnect(disp);
	return 0;
}
