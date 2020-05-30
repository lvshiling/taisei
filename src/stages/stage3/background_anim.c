/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "background_anim.h"
#include "draw.h"

#include "stageutils.h"

DEFINE_EXTERN_TASK(animate_bg) {
	Stage3DrawData *draw_data = stage3_get_draw_data();

	draw_data->tunnel.angle += draw_data->tunnel.avel;
	stage_3d_context.crot[2] = -(creal(global.plr.pos)-VIEWPORT_W/2)/80.0;

	if(dialog_is_active(global.dialog)) {
		stage3d_update(&stage_3d_context);
		return;
	}

	stage_3d_context.cv[1] -= 0.5/2;
	draw_data->color.r -= 0.2 / 160.0;
	draw_data->color.b -= 0.1 / 160.0;

	stage3d_update(&stage_3d_context);

}

