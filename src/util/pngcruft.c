/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "pngcruft.h"
#include "log.h"

static void png_rwops_write_data(png_structp png_ptr, png_bytep data, png_size_t length) {
	SDL_RWops *out = png_get_io_ptr(png_ptr);
	SDL_RWwrite(out, data, length, 1);
}

static void png_rwops_flush_data(png_structp png_ptr) {
	// no flush operation in SDL_RWops
}

static void png_rwops_read_data(png_structp png_ptr, png_bytep data, png_size_t length) {
	SDL_RWops *out = png_get_io_ptr(png_ptr);
	SDL_RWread(out, data, length, 1);
}

void png_init_rwops_read(png_structp png, SDL_RWops *rwops) {
	png_set_read_fn(png, rwops, png_rwops_read_data);
}

void png_init_rwops_write(png_structp png, SDL_RWops *rwops) {
	png_set_write_fn(png, rwops, png_rwops_write_data, png_rwops_flush_data);
}

noreturn static void png_error_handler(png_structp png_ptr, png_const_charp error_msg) {
	log_warn("PNG error: %s", error_msg);
	png_longjmp(png_ptr, 1);
}

static void png_warning_handler(png_structp png_ptr, png_const_charp warning_msg) {
	log_warn("PNG warning: %s", warning_msg);
}

void png_setup_error_handlers(png_structp png) {
	png_set_error_fn(png, NULL, png_error_handler, png_warning_handler);
}
