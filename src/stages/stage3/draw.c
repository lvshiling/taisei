/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "draw.h"

#include "stagedraw.h"
#include "stageutils.h"
#include "global.h"
#include "util/glm.h"

static Stage3DrawData *stage3_draw_data;

Stage3DrawData *stage3_get_draw_data(void) {
	return NOT_NULL(stage3_draw_data);
}

void stage3_drawsys_init(void) {
	stage3_draw_data = calloc(1, sizeof(*stage3_draw_data));
	stage3d_init(&stage_3d_context, 16);

	stage_3d_context.cx[2] = -10;
	stage_3d_context.crot[0] = -95;
	stage_3d_context.cv[1] = 10;

	stage3_draw_data->color.r = 1.0;
	stage3_draw_data->color.g = 0.0;
	stage3_draw_data->color.b = 0.5;
	stage3_draw_data->color.mixfactor = 1.0;
	stage3_draw_data->fog.brightness = 0.5;
}

void stage3_drawsys_shutdown(void) {
	stage3d_shutdown(&stage_3d_context);
	free(stage3_draw_data);
	stage3_draw_data = NULL;
}

static uint stage3_bg_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	Stage3DrawData *draw_data = stage3_get_draw_data();
	//vec3 p = {100 * cos(global.frames / 52.0), 100, 50 * sin(global.frames / 50.0)};
	vec3 p = {
		draw_data->tunnel.side * cos(global.frames / 52.0),
		0,
		draw_data->tunnel.updn * sin(global.frames / 50.0)
	};
	vec3 r = {0, 3000, 0};

	return linear3dpos(s3d, pos, maxrange, p, r);
}

static void stage3_bg_tunnel_draw(vec3 pos) {
	Stage3DrawData *draw_data = stage3_get_draw_data();
	int n = 6;
	float r = 300;
	int i;

	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);

	r_uniform_sampler("tex", "stage3/border");

	for(i = 0; i < n; i++) {
		r_mat_mv_push();
		r_mat_mv_rotate(M_PI*2.0/n*i + draw_data->tunnel.angle * DEG2RAD, 0, 1, 0);
		r_mat_mv_translate(0,0,-r);
		r_mat_mv_scale(2*r/tan((n-2)*M_PI/n), 3000, 1);
		r_draw_quad();
		r_mat_mv_pop();
	}

	r_mat_mv_pop();
}

static bool stage3_tunnel(Framebuffer *fb) {
	Stage3DrawData *draw_data = stage3_get_draw_data();
	r_shader("tunnel");
	r_uniform_vec3("color", draw_data->color.r, draw_data->color.g, draw_data->color.b);
	r_uniform_float("mixfactor", draw_data->color.mixfactor);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

static bool stage3_fog(Framebuffer *fb) {
	Stage3DrawData *draw_data = stage3_get_draw_data();
	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4("fog_color", draw_data->fog.brightness, draw_data->fog.brightness, draw_data->fog.brightness, 1.0);
	r_uniform_float("start", 0.2);
	r_uniform_float("end", 0.8);
	r_uniform_float("exponent", draw_data->fog.exponent/2);
	r_uniform_float("sphereness", 0);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

static bool stage3_glitch(Framebuffer *fb) {
	float strength;

	if(global.boss && global.boss->current && ATTACK_IS_SPELL(global.boss->current->type) && !strcmp(global.boss->name, "Scuttle")) {
		strength = 0.05 * fmax(0, (global.frames - global.boss->current->starttime) / (double)global.boss->current->timeout);
	} else {
		strength = 0.0;
	}

	if(strength > 0) {
		r_shader("glitch");
		r_uniform_float("strength", strength);
		r_uniform_float("frames", global.frames + 15 * rng_sreal());
	} else {
		return false;
	}

	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

void stage3_draw(void) {
	r_mat_proj_perspective(STAGE3D_DEFAULT_FOVY, STAGE3D_DEFAULT_ASPECT, 300, 5000);
	stage3d_draw(&stage_3d_context, 7000, 1, (Stage3DSegment[]) { stage3_bg_tunnel_draw, stage3_bg_pos });
}

ShaderRule stage3_bg_effects[] = {
	stage3_fog,
	stage3_tunnel,
	NULL
};

ShaderRule stage3_postprocess[] = {
	stage3_glitch,
	NULL
};
