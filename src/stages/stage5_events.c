/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage5_events.h"
#include "stage5.h"
#include "global.h"
#include "common_tasks.h"

PRAGMA(message "Remove when this stage is modernized")
DIAGNOSTIC(ignored "-Wdeprecated-declarations")

TASK(boss_appear_stub, NO_ARGS) {
	log_warn("FIXME");
}

static void stage5_dialog_pre_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Stage5PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage5PreBossDialog, pm->dialog->Stage5PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear_stub);
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage5boss");
}

static void stage5_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage5PostBossDialog, pm->dialog->Stage5PostBoss);
}

static void stage5_dialog_post_midboss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage5PostMidBossDialog, pm->dialog->Stage5PostMidBoss);
}

static int stage5_greeter(Enemy *e, int t) {
	TIMER(&t)
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 2, ITEM_POWER, 2);
		return 1;
	}

	e->moving = true;
	e->dir = creal(e->args[0]) < 0;

	FROM_TO(0, 50, 1) {
		GO_TO(e, e->pos0 + e->args[0] * 50, 0.05);
	}

	if(t > 200)
		e->pos += e->args[0];

	FROM_TO(80, 180, 20) {
		for(int i = -(int)global.diff; i <= (int)global.diff; i++) {
			PROJECTILE(
				.proto = pp_bullet,
				.pos = e->pos,
				.color = RGB(0.0, 0.0, 1.0),
				.rule = asymptotic,
				.args = {
					(3.5+(global.diff == D_Lunatic))*cexp(I*carg(global.plr.pos-e->pos) + 0.06*I*i),
					5
				}
			);
		}

		play_sound("shot1");
	}

	return 1;
}

static int stage5_lightburst(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 4, ITEM_POWER, 2);
		return 1;
	}

	FROM_TO(0, 70, 1) {
		GO_TO(e, e->pos0 + e->args[0] * 70, 0.05);
	}

	if(t > 200)
		e->pos += e->args[0];

	FROM_TO_SND("shot1_loop", 20, 300, 5) {
		int c = 5+global.diff;
		for(int i = 0; i < c; i++) {
			cmplx n = cexp(I*carg(global.plr.pos) + 2.0*I*M_PI/c*i);
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos + 50*n*cexp(-0.4*I*_i*global.diff),
				.color = RGB(0.3, 0, 0.7),
				.rule = asymptotic,
				.args = { 3*n, 3 }
			);
		}

		play_sound("shot2");
	}

	return 1;
}

static int stage5_swirl(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 1);
		return 1;
	}

	if(t > creal(e->args[1]) && t < cimag(e->args[1]))
		e->args[0] *= e->args[2];

	e->pos += e->args[0];

	FROM_TO(0, 400, 26-global.diff*4) {
		for(int i = 1; i >= -1; i -= 2) {
			PROJECTILE(
				.proto = pp_bullet,
				.pos = e->pos,
				.color = RGB(0.3, 0.4, 0.5),
				.rule = asymptotic,
				.args = { i*2*e->args[0]*I/cabs(e->args[0]), 3 }
			);
		}

		play_sound("shot1");
	}

	return 1;
}

static int stage5_limiter(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 4, ITEM_POWER, 4);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO_SND("shot1_loop", 0, 1200, 3) {
		uint f = PFLAG_NOSPAWNFADE;
		double base_angle = carg(global.plr.pos - e->pos);

		for(int i = 1; i >= -1; i -= 2) {
			double a = i * 0.2 - 0.1 * (global.diff / 4) + i * 3.0 / (_i + 1);
			cmplx aim = cexp(I * (base_angle + a));

			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGB(0.5,0.1,0.2),
				.rule = asymptotic,
				.args = { 10*aim, 2 },
				.flags = f,
			);
		}
	}

	return 1;
}

static int stage5_laserfairy(Enemy *e, int t) {
	TIMER(&t)
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 5, ITEM_POWER, 5);
		return 1;
	}

	FROM_TO(0, 100, 1) {
		GO_TO(e, e->pos0 + e->args[0] * 100, 0.05);
	}

	if(t > 700)
		e->pos -= e->args[0];

	FROM_TO(100, 700, (7-global.diff)*(1+(int)creal(e->args[1]))) {
		cmplx n = cexp(I*carg(global.plr.pos-e->pos)+(0.2-0.02*global.diff)*I*_i);
		float fac = (0.5+0.2*global.diff);
		create_lasercurve2c(e->pos, 100, 300, RGBA(0.7, 0.3, 1, 0), las_accel, fac*4*n, fac*0.05*n);
		PROJECTILE(
			.proto = pp_plainball,
			.pos = e->pos,
			.color = RGBA(0.7, 0.3, 1, 0),
			.rule = accelerated,
			.args = { fac*4*n, fac*0.05*n }
		);
		play_sfx_ex("shot_special1", 0, true);
	}

	return 1;
}

static int stage5_miner(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 2);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(0, 600, 5-global.diff/2) {
		tsrand_fill(2);
		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos + 20*cexp(2.0*I*M_PI*afrand(0)),
			.color = RGB(0,0,cabs(e->args[0])),
			.rule = linear,
			.args = { cexp(2.0*I*M_PI*afrand(1)) }
		);
		play_sfx_ex("shot3", 0, false);
	}

	return 1;
}

static void lightning_particle(cmplx pos, int t) {
	if(!(t % 5)) {
		char *part = frand() > 0.5 ? "lightning0" : "lightning1";
		PARTICLE(
			.sprite = part,
			.pos = pos,
			.color = RGBA(1.0, 1.0, 1.0, 0.0),
			.timeout = 20,
			.draw_rule = Fade,
			.flags = PFLAG_REQUIREDPARTICLE,
			.angle = frand()*2*M_PI,
		);
	}
}

static int stage5_magnetto(Enemy *e, int t) {
	TIMER(&t);

	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 5, ITEM_POWER, 5);
		return 1;
	}

	if(t < 0) {
		return 1;
	}

	cmplx offset = (frand()-0.5)*10 + (frand()-0.5)*10.0*I;
	lightning_particle(e->pos + 3*offset, t);

	FROM_TO(0, 70, 1) {
		GO_TO(e, e->args[0], 0.1);
	}

	AT(140) {
		play_sound("redirect");
		play_sfx_delayed("redirect", 0, false, 180);
	}

	FROM_TO(140, 320, 1) {
		e->pos += 3 * cexp(I*(carg(e->args[1] - e->pos) + M_PI/2));
		GO_TO(e, e->args[1], pow((t - 140) / 300.0, 3));
	}

	FROM_TO_SND("shot1_loop", 140, 280, 1 + 2 * (1 + D_Lunatic - global.diff)) {
		// complex dir = cexp(I*carg(global.plr.pos - e->pos));

		for(int i = 0; i < 2 - (global.diff == D_Easy); ++i) {
			cmplx dir = cexp(I*(M_PI*i + M_PI/8*sin(2*(t-140)/70.0 * M_PI) + carg(e->args[1] - e->pos)));

			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.color = RGBA(0.1 + 0.5 * pow((t - 140) / 140.0, 2), 0.0, 0.8, 0.0),
				.rule = accelerated,
				.args = {
					(-2 + (global.diff == D_Hard)) * dir,
					0.02 * dir * (!i || global.diff != D_Lunatic),
				},
			);
		}
	}

	AT(380) {
		return ACTION_DESTROY;
	}

	return 1;
}

static int stage5_explosion(Enemy *e, int t) {
	TIMER(&t)
	AT(EVENT_KILLED) {
		spawn_items(e->pos,
			ITEM_POINTS, 5,
			ITEM_POWER, 5,
			ITEM_LIFE, creal(e->args[1])
		);

		PARTICLE(
			.sprite = "blast_huge_rays",
			.color = color_add(RGBA(0, 0.2 + 0.5 * frand(), 0.5 + 0.5 * frand(), 0.0), RGBA(1, 1, 1, 0)),
			.pos = e->pos,
			.timeout = 60 + 10 * frand(),
			.draw_rule = ScaleFade,
			.args = { 0, 0, (0 + 3*I) * (1 + 0.2 * frand()) },
			.angle = frand() * 2 * M_PI,
			.layer = LAYER_PARTICLE_HIGH | 0x42,
			.flags = PFLAG_REQUIREDPARTICLE,
		);

		PARTICLE(
			.sprite = "blast_huge_halo",
			.pos = e->pos,
			.color = RGBA(0.3 * frand(), 0.3 * frand(), 1.0, 0),
			.timeout = 200 + 24 * frand(),
			.draw_rule = ScaleFade,
			.args = { 0, 0, (0.25 + 2.5*I) * (1 + 0.2 * frand()) },
			.layer = LAYER_PARTICLE_HIGH | 0x41,
			.angle = frand() * 2 * M_PI,
			.flags = PFLAG_REQUIREDPARTICLE,
		);

		play_sound("boom");
		return 1;
	}

	FROM_TO(0, 80, 1) {
		GO_TO(e, e->pos0 + e->args[0] * 80, 0.05);
	}

	FROM_TO(90, 300, 7-global.diff) {
		PROJECTILE(
			.proto = pp_soul,
			.pos = e->pos,
			.color = RGBA(0, 0, 1, 0),
			.rule = asymptotic,
			.args = { 4*cexp(0.5*I*_i), 3 }
		);
		play_sound("shot_special1");
	}

	FROM_TO(200, 720, 6-global.diff) {
		for(int i = 1; i >= -1; i -= 2) {
			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGB(1,0,0),
				.rule = asymptotic,
				.args = { i*2*cexp(-0.3*I*_i+frand()*I), 3 }
			);
		}

		play_sound("shot3");
	}

	FROM_TO(500-30*(global.diff-D_Easy), 800, 100-10*global.diff) {
		create_laserline(e->pos, 10*cexp(I*carg(global.plr.pos-e->pos)+0.04*I*(1-2*frand())), 60, 120, RGBA(1, 0.3, 1, 0));
		play_sfx_delayed("laser1", 0, true, 45);
	}

	return 1;
}

static void iku_slave_visual(Enemy *e, int t, bool render) {
	if(render) {
		return;
	}

	cmplx offset = (frand()-0.5)*10 + (frand()-0.5)*10.0*I;

	if(e->args[2] && !(t % 5)) {
		lightning_particle(e->pos + 3*offset, t);
	}

	if(!(t % 3)) {
		float alpha = 1;

		if(!e->args[2]) {
			alpha *= 0.03;
		}

		Color *clr = RGBA_MUL_ALPHA(0.1*alpha, 0.1*alpha, 0.6*alpha, 0.5*alpha);
		clr->a = 0;

		PARTICLE(
			.sprite = "lightningball",
			.pos = 0,
			.color = clr,
			.draw_rule = Fade,
			.rule = enemy_flare,
			.timeout = 50,
			.args = { offset*0.1, add_ref(e) },
			.flags = PFLAG_REQUIREDPARTICLE,
		);
	}
}

static void iku_mid_intro(Boss *b, int t) {
	TIMER(&t);

	b->pos += -1-7.0*I+10*t*(cimag(b->pos)<-200);

	FROM_TO(90, 110, 10) {
		create_enemy3c(b->pos, ENEMY_IMMUNE, iku_slave_visual, stage5_explosion, -2-0.5*_i+I*_i, _i == 1,1);
	}

	AT(960)
		enemy_kill_all(&global.enemies);
}

static void midboss_dummy(Boss *b, int t) { }

static Boss *create_iku_mid(void) {
	Boss *b = create_boss("Bombs?", "iku_mid", VIEWPORT_W+800.0*I);
	b->glowcolor = *RGB(0.2, 0.4, 0.5);
	b->shadowcolor = *RGBA_MUL_ALPHA(0.65, 0.2, 0.75, 0.5);

	Attack *a = boss_add_attack(b, AT_SurvivalSpell, "Discharge Bombs", 16, 10, iku_mid_intro, NULL);
	boss_set_attack_bonus(a, 5);

	// suppress the boss death effects (this triggers the "boss fleeing" case)
	boss_add_attack(b, AT_Move, "", 0, 0, midboss_dummy, NULL);

	boss_start_attack(b, b->attacks);
	b->attacks->starttime = global.frames;  // HACK: thwart attack delay

	return b;
}

static int stage5_lightburst2(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 4, ITEM_POWER, 4);
		return 1;
	}

	FROM_TO(0, 70, 1) {
		GO_TO(e, e->pos0 + e->args[0] * 70, 0.05);
	}

	if(t > 200)
		e->pos += e->args[0];

	FROM_TO_SND("shot1_loop", 20, 170, 5) {
		int i;
		int c = 4+global.diff-(global.diff==D_Easy);
		for(i = 0; i < c; i++) {
			tsrand_fill(2);
			cmplx n = cexp(I*carg(global.plr.pos-e->pos) + 2.0*I*M_PI/c*i);
			PROJECTILE(
				.proto = pp_bigball,
				.pos = e->pos + 50*n*cexp(-1.0*I*_i*global.diff),
				.color = RGB(0.3, 0, 0.7+0.3*(_i&1)),
				.rule = asymptotic,
				.args = {
					2.5*n+0.25*global.diff*afrand(0)*cexp(2.0*I*M_PI*afrand(1)),
					3
				}
			);
		}

		play_sound("shot2");
	}

	return 1;
}

static int stage5_superbullet(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 4, ITEM_POWER, 3);
		return 1;
	}

	FROM_TO(0, 70, 1) {
		GO_TO(e, e->pos0 + e->args[0] * 70, 0.05);
	}

	FROM_TO(60, 200, 1) {
		cmplx n = cexp(I*M_PI*sin(_i/(8.0+global.diff)+frand()*0.1)+I*carg(global.plr.pos-e->pos));
		PROJECTILE(
			.proto = pp_bullet,
			.pos = e->pos + 50*n,
			.color = RGB(0.6, 0, 0),
			.rule = asymptotic,
			.args = { 2*n, 10 }
		);
		play_sound("shot1");
	}

	FROM_TO(260, 400, 1)
		e->pos -= e->args[0];
	return 1;
}

static void iku_intro(Boss *b, int t) {
	GO_TO(b, VIEWPORT_W/2+240.0*I, 0.015);

	if(t == 160)
		stage5_dialog_pre_boss();
}

static void cloud_common(void) {
	tsrand_fill(4);
	float v = (afrand(2)+afrand(3))*0.5+1.0;

	PROJECTILE(
		// FIXME: add prototype, or shove it into the basic ones somehow,
		// or just replace this with some thing else
		.sprite_ptr = get_sprite("part/lightningball"),
		.size = 48 * (1+I),
		.collision_size = 21.6 * (1+I),

		.pos = VIEWPORT_W*afrand(0)-15.0*I,
		.color = RGBA_MUL_ALPHA(0.2, 0.0, 0.4, 0.6),
		.rule = accelerated,
		.args = {
			1-2*afrand(1)+v*I,
			-0.01*I
		},
		.shader = "sprite_default",
	);
}

static void iku_bolts(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	FROM_TO(0, 400, 2) {
		cloud_common();
	}

	FROM_TO(60, 400, 50) {
		int i, c = 10+global.diff;

		for(i = 0; i < c; i++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = b->pos,
				.color = RGBA(0.4, 1.0, 1.0, 0),
				.rule = asymptotic,
				.args = {
					(i+2)*0.4*cexp(I*carg(global.plr.pos-b->pos))+0.2*(global.diff-1)*frand(),
					3
				},
			);
		}

		play_sound("shot2");
		play_sound("redirect");
	}

	FROM_TO(0, 70, 1)
		GO_TO(b, 100+300.0*I, 0.02);

	FROM_TO(100, 200, 1)
		GO_TO(b, VIEWPORT_W/2+100.0*I, 0.02);

	FROM_TO(230, 300, 1)
		GO_TO(b, VIEWPORT_W-100+300.0*I, 0.02);

	FROM_TO(330, 400, 1)
		GO_TO(b, VIEWPORT_W/2+100.0*I, 0.02);

}

void iku_atmospheric(Boss *b, int time) {
	if(time < 0) {
		GO_TO(b, VIEWPORT_W/2+200.0*I, 0.06);
		return;
	}

	int t = time % 500;
	TIMER(&t);

	GO_TO(b,VIEWPORT_W/2+tanh(sin(time/100))*(200-100*(global.diff==D_Easy))+I*VIEWPORT_H/3+I*(cos(t/200)-1)*50,0.03);

	FROM_TO(0, 500, 23-2*global.diff) {
		tsrand_fill(4);
		cmplx p1 = VIEWPORT_W*afrand(0) + VIEWPORT_H/2*I*afrand(1);
		cmplx p2 = p1 + (120+20*global.diff)*cexp(0.5*I-afrand(2)*I)*(1-2*(afrand(3) > 0.5));

		int i;
		int c = 6+global.diff;

		for(i = -c*0.5; i <= c*0.5; i++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = p1+(p2-p1)/c*i,
				.color = RGBA(1-1/(1+fabs(0.1*i)), 0.5-0.1*abs(i), 1, 0),
				.rule = accelerated,
				.args = {
					0, (0.004+0.001*global.diff)*cexp(I*carg(p2-p1)+I*M_PI/2+0.2*I*i)
				},
			);
		}

		play_sound("shot_special1");
		play_sound("redirect");
	}

	FROM_TO(0, 500, 7-global.diff) {
		if(global.diff >= D_Hard) {
			PROJECTILE(
				.proto = pp_thickrice,
				.pos = VIEWPORT_W*frand(),
				.color = RGB(0,0.3,0.7),
				.rule = accelerated,
				.args = { 0, 0.01*I }
			);
		}
		else {
			PROJECTILE(
				.proto = pp_rice,
				.pos = VIEWPORT_W*frand(),
				.color = RGB(0,0.3,0.7),
				.rule = linear,
				.args = { 2*I }
			);
		}
	}
}

static cmplx bolts2_laser(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("lasers/iku_lightning");
		return 0;
	}

	double diff = creal(l->args[2]);
	return creal(l->args[0])+I*cimag(l->pos) + sign(cimag(l->args[0]-l->pos))*0.06*I*t*t + (20+4*diff)*sin(t*0.025*diff+creal(l->args[0]))*l->args[1];
}

static void iku_bolts2(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	// FIXME: ANOTHER one of these... get rid of this hack when attacks have proper state
	static bool flip_laser;

	if(time == EVENT_BIRTH) {
		flip_laser = true;
	}

	FROM_TO(0, 400, 2) {
		cloud_common();
	}

	FROM_TO(0, 400, 60) {
		flip_laser = !flip_laser;
		aniplayer_queue(&b->ani, flip_laser ? "dashdown_left" : "dashdown_right", 1);
		aniplayer_queue(&b->ani, "main", 0);
		create_lasercurve3c(creal(global.plr.pos), 100, 200, RGBA(0.3, 1, 1, 0), bolts2_laser, global.plr.pos, flip_laser*2-1, global.diff);
		play_sfx_ex("laser1", 0, false);
	}

	FROM_TO_SND("shot1_loop", 0, 400, 5-global.diff)
		if(frand() < 0.9) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = b->pos,
				.color = RGB(0.2,0,0.8),
				.rule = linear,
				.args = { cexp(0.1*I*_i) }
			);
		}

	FROM_TO(0, 70, 1)
		GO_TO(b, 100+200.0*I, 0.02);

	FROM_TO(230, 300, 1)
		GO_TO(b, VIEWPORT_W-100+200.0*I, 0.02);

}

static int lightning_slave(Enemy *e, int t) {
	if(t < 0)
		return 1;
	if(t > 200)
		return ACTION_DESTROY;

	TIMER(&t);

	e->pos += e->args[0];

	FROM_TO(0,200,20)
		e->args[0] *= cexp(I * (0.25 + 0.25 * frand() * M_PI));

	FROM_TO(0, 200, 3)
		if(cabs(e->pos-global.plr.pos) > 60) {
			Color *clr = RGBA(1-1/(1+0.01*_i), 0.5-0.01*_i, 1, 0);

			Projectile *p = PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos,
				.color = clr,
				.rule = asymptotic,
				.args = {
					0.75*e->args[0]/cabs(e->args[0])*I,
					10
				},
			);

			if(projectile_in_viewport(p)) {
				for(int i = 0; i < 3; ++i) {
					tsrand_fill(2);
					lightning_particle(p->pos + 5 * afrand(0) * cexp(I*M_PI*2*afrand(1)), 0);
				}

				play_sfx_ex("shot3", 0, false);
				// play_sound_ex("redirect", 0, true);
			}
		}

	return 1;
}

static int zigzag_bullet(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	int l = 50;
	p->pos = p->pos0+(abs(((2*t)%l)-l/2)*I+t)*2*p->args[0];

	if(t%2 == 0) {
		PARTICLE(
			.sprite = "lightningball",
			.pos = p->pos,
			.color = RGBA(0.1, 0.1, 0.6, 0.0),
			.timeout = 15,
			.draw_rule = Fade,
		);
	}

	return ACTION_NONE;
}

void iku_lightning(Boss *b, int time) {
	int t = time % 141;

	if(time == EVENT_DEATH) {
		enemy_kill_all(&global.enemies);
		return;
	}

	if(time < 0) {
		GO_TO(b, BOSS_DEFAULT_GO_POS, 0.03);
		return;
	}

	TIMER(&t);

	GO_TO(b,VIEWPORT_W/2+tanh(sin(time/100))*200+I*VIEWPORT_H/3+I*(cos(t/200)-1)*50,0.03);

	AT(0) {
		play_sound("charge_generic");
	}

	FROM_TO(0, 60, 1) {
		cmplx n = cexp(2.0*I*M_PI*frand());
		float l = 150*frand()+50;
		float s = 4+_i*0.01;
		float alpha = 0.5;

		PARTICLE(
			.sprite = "lightningball",
			.pos = b->pos+l*n,
			.color = RGBA(0.1*alpha, 0.1*alpha, 0.6*alpha, 0),
			.draw_rule = Fade,
			.rule = linear,
			.timeout = l/s,
			.args = { -s*n },
		);
	}

	if(global.diff >= D_Hard && time > 0 && !(time%100)) {
		int c = 7 + 2 * (global.diff == D_Lunatic);
		for(int i = 0; i<c; i++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = b->pos,
				.color = RGBA(0.5, 0.1, 1.0, 0.0),
				.rule = zigzag_bullet,
				.args = { cexp(2*M_PI*I/c*i+I*carg(global.plr.pos-b->pos)) },
			);
		}

		play_sound("redirect");
		play_sound("shot_special1");
	}

	AT(100) {
		aniplayer_hard_switch(&b->ani, ((time/141)&1) ? "dashdown_left" : "dashdown_right",1);
		aniplayer_queue(&b->ani, "main", 0);
		int c = 40;
		int l = 200;
		int s = 10;

		for(int i=0; i < c; i++) {
			cmplx n = cexp(2.0*I*M_PI*frand());
			PARTICLE(
				.sprite = "smoke",
				.pos = b->pos,
				.color = RGBA(0.4, 0.4, 1.0, 0.0),
				.draw_rule = Fade,
				.rule = linear,
				.timeout = l/s,
				.args = { s*n },
			);
		}

		for(int i = 0; i < global.diff+1; i++){
			create_enemy1c(b->pos, ENEMY_IMMUNE, NULL, lightning_slave, 10*cexp(I*carg(global.plr.pos - b->pos)+2.0*I*M_PI/(global.diff+1)*i));
		}

		play_sound("shot_special1");
	}
}

static void iku_bolts3(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	FROM_TO(0, 400, 2) {
		cloud_common();
	}

	FROM_TO(60, 400, 60) {
		aniplayer_queue(&b->ani, (_i&1) ? "dashdown_left" : "dashdown_right",1);
		aniplayer_queue(&b->ani, "main", 0);
		int i, c = 10+global.diff;
		cmplx n = cexp(I*carg(global.plr.pos-b->pos)+0.1*I-0.2*I*frand());
		for(i = 0; i < c; i++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = b->pos,
				.color = RGBA(0.4, 1.0, 1.0, 0.0),
				.rule = asymptotic,
				.args = {
					(i+2)*0.4*n+0.2*(global.diff-1)*frand(),
					3
				},
			);
		}

		play_sound("shot2");
		play_sound("redirect");
	}

	FROM_TO_SND("shot1_loop", 0, 400, 5-global.diff)
		if(frand() < 0.9) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = b->pos,
				.color = RGB(0.2,0,0.8),
				.rule = linear,
				.args = { cexp(0.1*I*_i) }
			);
		}

	FROM_TO(0, 70, 1)
		GO_TO(b, 100+200.0*I, 0.02);

	FROM_TO(230, 300, 1)
		GO_TO(b, VIEWPORT_W-100+200.0*I, 0.02);

}

static cmplx induction_bullet_traj(Projectile *p, float t) {
	return p->pos0 + p->args[0]*t*cexp(p->args[1]*t);
}

static int induction_bullet(Projectile *p, int time) {
	if(time < 0) {
		return ACTION_ACK;
	}

	float t = time*sqrt(global.diff);

	if(global.diff > D_Normal && !p->args[2]) {
		t = time*0.6;
		t = 230-t;
		if(t < 0)
			return ACTION_DESTROY;
	}

	p->pos = induction_bullet_traj(p,t);

	if(time == 0) {
		// don't lerp; the spawn position is very different on hard/lunatic and would cause false hits
		p->prevpos = p->pos;
	}

	p->angle = carg(p->args[0]*cexp(p->args[1]*t)*(1+p->args[1]*t));
	return 1;
}

static cmplx cathode_laser(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("lasers/iku_cathode");
		return 0;
	}

	l->args[1] = I*cimag(l->args[1]);

	return l->pos + l->args[0]*t*cexp(l->args[1]*t);
}

void iku_cathode(Boss *b, int t) {
	GO_TO(b, VIEWPORT_W/2+200.0*I, 0.02);

	TIMER(&t)

	FROM_TO(50, 18000, 70-global.diff*10) {
		aniplayer_hard_switch(&b->ani, (_i&1) ? "dashdown_left" : "dashdown_right",1);
		aniplayer_queue(&b->ani,"main",0);

		int i;
		int c = 7+global.diff/2;

		double speedmod = 1-0.3*(global.diff == D_Lunatic);
		for(i = 0; i < c; i++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = b->pos,
				.color = RGBA(0.2, 0.4, 1.0, 0.0),
				.rule = induction_bullet,
				.args = {
					speedmod*2*cexp(2.0*I*M_PI*frand()),
					speedmod*0.01*I*(1-2*(_i&1)),
					1
				},
			);
			if(i < c*3/4)
				create_lasercurve2c(b->pos, 60, 200, RGBA(0.4, 1, 1, 0), cathode_laser, 2*cexp(2.0*I*M_PI*M_PI*frand()), 0.015*I*(1-2*(_i&1)));
		}

		// XXX: better ideas?
		play_sound("shot_special1");
		play_sound("redirect");
		play_sound("shot3");
		play_sound("shot2");
	}
}

void iku_induction(Boss *b, int t) {
	// thwarf safespots
	cmplx ofs = global.diff > D_Normal ? 10*I : 0;

	GO_TO(b, VIEWPORT_W/2+200.0*I + ofs, 0.03);

	if(t < 0) {
		return;
	}

	TIMER(&t);
	AT(0) {
		aniplayer_queue(&b->ani, "dashdown_wait", 0);
	}

	FROM_TO_SND("shot1_loop", 0, 18000, 8) {
		play_sound("redirect");

		int i,j;
		int c = 6;
		int c2 = 6-(global.diff/4);
		for(i = 0; i < c; i++) {
			for(j = 0; j < 2; j++) {
				Color *clr = RGBA(1-1/(1+0.1*(_i%c2)), 0.5-0.1*(_i%c2), 1.0, 0.0);
				float shift = 0.6*(_i/c2);
				float a = -0.0002*(global.diff-D_Easy);
				if(global.diff == D_Hard)
					a += 0.0005;
				PROJECTILE(
					.proto = pp_ball,
					.pos = b->pos,
					.color = clr,
					.rule = induction_bullet,
					.args = {
						2*cexp(2.0*I*M_PI/c*i+I*M_PI/2+I*shift),
						(0.01+0.001*global.diff)*I*(1-2*j)+a
					},
					.max_viewport_dist = 400*(global.diff>=D_Hard),
				);
			}
		}

	}
}

void iku_spell_bg(Boss *b, int t);

static Enemy* iku_extra_find_next_slave(cmplx from, double playerbias) {
	Enemy *nearest = NULL, *e;
	double dist, mindist = INFINITY;

	cmplx org = from + playerbias * cexp(I*(carg(global.plr.pos - from)));

	for(e = global.enemies.first; e; e = e->next) {
		if(e->args[2]) {
			continue;
		}

		dist = cabs(e->pos - org);

		if(dist < mindist) {
			nearest = e;
			mindist = dist;
		}
	}

	return nearest;
}

static void iku_extra_slave_visual(Enemy *e, int t, bool render) {
	iku_slave_visual(e, t, render);

	if(render) {
		return;
	}

	if(e->args[2] && !(t % 5)) {
		cmplx offset = (frand()-0.5)*30 + (frand()-0.5)*20.0*I;
		PARTICLE(
			.sprite = "smoothdot",
			.pos = offset,
			.color = e->args[1] ? RGBA(1.0, 0.5, 0.0, 0.0) : RGBA(0.0, 0.5, 0.5, 0.0),
			.draw_rule = Shrink,
			.rule = enemy_flare,
			.timeout = 50,
			.args = {
				(-50.0*I-offset)/50.0,
				add_ref(e)
			},
		);
	}
}

static int iku_extra_trigger_bullet(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[1]);
		return ACTION_ACK;
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	Enemy *target = REF(p->args[1]);

	if(!target) {
		return ACTION_DESTROY;
	}

	if(creal(p->args[2]) < 0) {
		linear(p, t);
		if(cabs(p->pos - target->pos) < 5) {
			p->pos = target->pos;
			target->args[1] = 1;
			p->args[2] = 55 - 5 * global.diff;
			target->args[3] = global.frames + p->args[2];
			play_sound("shot_special1");
		}
	} else {
		p->args[2] = approach(creal(p->args[2]), 0, 1);
		play_sfx_loop("charge_generic");
	}

	if(creal(p->args[2]) == 0) {
		int cnt = 6 + 2 * global.diff;
		for(int i = 0; i < cnt; ++i) {
			cmplx dir = cexp(I*(t + i*2*M_PI/cnt));

			PROJECTILE(
				.proto = pp_bigball,
				.pos = p->pos,
				.color = RGBA(1.0, 0.5, 0.0, 0.0),
				.rule = asymptotic,
				.args = { 1.1*dir, 5 },
			);

			PROJECTILE(
				.proto = pp_bigball,
				.pos = p->pos,
				.color = RGBA(0.0, 0.5, 1.0, 0.0),
				.rule = asymptotic,
				.args = { dir, 10 },
			);
		}
		stage_shake_view(40);
		aniplayer_hard_switch(&global.boss->ani,"main_mirror",0);
		play_sound("boom");
		return ACTION_DESTROY;
	}

	p->angle = global.frames + t;

	tsrand_fill(5);

	PARTICLE(
		.sprite = afrand(0) > 0.5 ? "lightning0" : "lightning1",
		.pos = p->pos + 3 * (anfrand(1)+I*anfrand(2)),
		.angle = afrand(3) * 2 * M_PI,
		.color = RGBA(1.0, 0.7 + 0.2 * anfrand(4), 0.4, 0.0),
		.timeout = 20,
		.draw_rule = GrowFade,
		.args = { 0, 2.4 },
	);

	return ACTION_NONE;
}

static void iku_extra_fire_trigger_bullet(void) {
	Enemy *e = iku_extra_find_next_slave(global.boss->pos, 250);

	aniplayer_hard_switch(&global.boss->ani,"dashdown_left",1);
	aniplayer_queue(&global.boss->ani,"main",0);
	if(!e) {
		return;
	}

	Boss *b = global.boss;

	PROJECTILE(
		.proto = pp_soul,
		.pos = b->pos,
		.color = RGBA(0.2, 0.2, 1.0, 0.0),
		.rule = iku_extra_trigger_bullet,
		.args = {
			3*cexp(I*carg(e->pos - b->pos)),
			add_ref(e),
			-1
		},
		.flags = PFLAG_NOCLEAR,
	);

	play_sound("shot_special1");
	play_sound("enemydeath");
	play_sound("shot2");
}

static int iku_extra_slave(Enemy *e, int t) {
	GO_TO(e, e->args[0], 0.05);

	if(e->args[1]) {
		if(creal(e->args[1]) < 2) {
			e->args[1] += 1;
			return 0;
		}

		if(global.frames == creal(e->args[3])) {
			cmplx o2 = e->args[2];
			e->args[2] = 0;
			Enemy *new = iku_extra_find_next_slave(e->pos, 75);
			e->args[2] = o2;

			if(new && e != new) {
				e->args[1] = 0;
				e->args[2] = new->args[2] = 600;
				new->args[1] = 1;
				new->args[3] = global.frames + 55 - 5 * global.diff;

				Laser *l = create_laserline_ab(e->pos, new->pos, 10, 30, e->args[2], RGBA(0.3, 1, 1, 0));
				l->ent.draw_layer = LAYER_LASER_LOW;
				l->unclearable = true;

				if(global.diff > D_Easy) {
					int cnt = floor(global.diff * 2.5), i;
					double r = frand() * 2 * M_PI;

					for(i = 0; i < cnt; ++i) {
						PROJECTILE(
							.proto = pp_rice,
							.pos = e->pos,
							.color = RGBA(1, 1, 0, 0),
							.rule = asymptotic,
							.args = { 2*cexp(I*(r+i*2*M_PI/cnt)), 2 },
						);
					}

					play_sound("shot2");
				}

				play_sound("redirect");
			} else {
				Enemy *o;
				Laser *l;
				int cnt = 6 + 2 * global.diff, i;

				e->args[2] = 1;

				for(o = global.enemies.first; o; o = o->next) {
					if(!o->args[2])
						continue;

					for(i = 0; i < cnt; ++i) {
						PROJECTILE(
							.proto = pp_ball,
							.pos = o->pos,
							.color = RGBA(0, 1, 1, 0),
							.rule = asymptotic,
							.args = { 1.5*cexp(I*(t + i*2*M_PI/cnt)), 8},
						);
					}

					o->args[1] = 0;
					o->args[2] = 0;

					stage_shake_view(5);
				}

				for(l = global.lasers.first; l; l = l->next) {
					l->deathtime = global.frames - l->birthtime + 20;
				}
				play_sound("boom");
				iku_extra_fire_trigger_bullet();
			}
		}
	}

	if(e->args[2]) {
		e->args[2] -= 1;
	}

	return 0;
}

void iku_extra(Boss *b, int t) {
	TIMER(&t);

	AT(EVENT_DEATH) {
		enemy_kill_all(&global.enemies);
	}

	if(t < 0) {
		return;
	}

	GO_TO(b, VIEWPORT_W/2+50.0*I, 0.02);

	AT(0) {
		int i, j;
		int cnt = 5;
		double step = VIEWPORT_W / (double)cnt;

		for(i = 0; i < cnt; ++i) {
			for(j = 0; j < cnt; ++j) {
				cmplx epos = step * (0.5 + i) + (step * j + 125) * I;
				create_enemy4c(b->pos, ENEMY_IMMUNE, iku_extra_slave_visual, iku_extra_slave, epos, 0, 0, 1);
			}
		}
	}

	AT(60) {
		iku_extra_fire_trigger_bullet();
	}
}

Boss* stage5_spawn_iku(cmplx pos) {
	Boss *b = create_boss("Nagae Iku", "iku", pos);
	boss_set_portrait(b, "iku", NULL, "normal");
	b->glowcolor = *RGBA_MUL_ALPHA(0.2, 0.4, 0.5, 0.5);
	b->shadowcolor = *RGBA_MUL_ALPHA(0.65, 0.2, 0.75, 0.5);
	return b;
}

static Boss* create_iku(void) {
	Boss *b = stage5_spawn_iku(VIEWPORT_W/2-200.0*I);

	boss_add_attack(b, AT_Move, "Introduction", 4, 0, iku_intro, NULL);
	boss_add_attack(b, AT_Normal, "Bolts1", 40, 24000, iku_bolts, NULL);
	boss_add_attack_from_info(b, &stage5_spells.boss.atmospheric_discharge, false);
	boss_add_attack(b, AT_Normal, "Bolts2", 45, 27000, iku_bolts2, NULL);
	boss_add_attack_from_info(b, &stage5_spells.boss.artificial_lightning, false);
	boss_add_attack(b, AT_Normal, "Bolts3", 50, 30000, iku_bolts3, NULL);

	if(global.diff < D_Hard) {
		boss_add_attack_from_info(b, &stage5_spells.boss.induction_field, false);
	} else {
		boss_add_attack_from_info(b, &stage5_spells.boss.inductive_resonance, false);
	}
	boss_add_attack_from_info(b, &stage5_spells.boss.natural_cathode, false);

	boss_add_attack_from_info(b, &stage5_spells.extra.overload, false);

	boss_start_attack(b, b->attacks);
	return b;
}

void stage5_events(void) {
	TIMER(&global.timer);

	AT(0) {
		stage_start_bgm("stage5");
		stage5_skip(env_get("STAGE5_TEST", 0));
		stage_set_voltage_thresholds(255, 480, 860, 1250);
	}

	FROM_TO(60, 150, 15) {
		create_enemy1c(VIEWPORT_W*(_i&1)+70.0*I+50*_i*I, 400, Fairy, stage5_greeter, 3-6*(_i&1));
	}

	FROM_TO(270, 320, 40)
		create_enemy1c(VIEWPORT_W/4+VIEWPORT_W/2*_i, 2000, BigFairy, stage5_lightburst, 2.0*I);

	FROM_TO(400, 600, 10) {
		tsrand_fill(2);
		create_enemy3c(200.0*I*afrand(0), 500, Swirl, stage5_swirl, 4+I, 70+20*afrand(1)+200.0*I, cexp(-0.05*I));
	}

	FROM_TO(700, 800, 10) {
		tsrand_fill(3);
		create_enemy3c(VIEWPORT_W+200.0*I*afrand(0), 500, Swirl, stage5_swirl, -4+afrand(1)*I, 70+20*afrand(2)+200.0*I, cexp(0.05*I));
	}

	FROM_TO(870+50*(global.diff==D_Easy), 1000, 50)
		create_enemy1c(VIEWPORT_W/4+VIEWPORT_W/2*(_i&1), 2000, BigFairy, stage5_limiter, I);

	AT(1000)
		create_enemy1c(VIEWPORT_W/2, 9000, BigFairy, stage5_laserfairy, 2.0*I);

	FROM_TO(1400+100*(D_Lunatic - global.diff), 2560, 60-5*global.diff) {
		tsrand_fill(2);
		create_enemy1c(VIEWPORT_W+200.0*I*afrand(0), 500, Swirl, stage5_miner, -3+2.0*I*afrand(1));
	}

	FROM_TO(1500, 2400, 80)
		create_enemy1c(VIEWPORT_W*(_i&1)+100.0*I, 300, Fairy, stage5_greeter, 3-6*(_i&1));

	AT(2500) {
		create_enemy1c(VIEWPORT_W/2, 2000, BigFairy, stage5_lightburst, 2.0*I);
	}

	FROM_TO(2200, 2600, 60-8*global.diff)
		create_enemy1c(VIEWPORT_W/(6+global.diff)*_i, 200, Swirl, stage5_miner, 3.0*I);

	AT(2700) {
		if(global.diff > D_Easy) {
			create_enemy1c(VIEWPORT_W-20+120*I, 2000, BigFairy, stage5_lightburst, -2.0);
		}
	}

	AT(2900)
		global.boss = create_iku_mid();

	AT(2920) {
		stage5_dialog_post_midboss();

		// XXX: this shitty hack is needed to force the dialog to not reopen immediately when it's closed with the shot button
		global.timer++;
	}

	FROM_TO(3000, 3200, 100) {
		create_enemy1c(VIEWPORT_W/2 + VIEWPORT_W/6*(1-2*(_i&1)), 2000, BigFairy, stage5_lightburst2, -1+2*(_i&1) + 2.0*I);
	}

	FROM_TO(3300, 4000, 90-10*global.diff)
		create_enemy1c(200.0*I+VIEWPORT_W*(_i&1), 1500, Fairy, stage5_superbullet, 3-6*(_i&1));

	AT(3400) {
		create_enemy2c(VIEWPORT_W/4, 6000, BigFairy, stage5_laserfairy, 2.0*I, 1);
		create_enemy2c(VIEWPORT_W/4*3, 6000, BigFairy, stage5_laserfairy, 2.0*I, 1);
	}

	FROM_TO(4200, 5000, 20-3*global.diff) {
		float f = frand();
		create_enemy3c(
			VIEWPORT_W/2+300*sin(global.frames)*cos(2*global.frames),
			400,
			Swirl,
			stage5_swirl,
			2*cexp(I*M_PI*f)+I,
			60 + 100.0*I,
			cexp(0.01*I*(1-2*(f<0.5)))
		);
	}

	FROM_TO(4320, 4400, 60) {
		double ofs = 32;
		create_enemy1c(ofs + (VIEWPORT_W-2*ofs)*(_i&1) + VIEWPORT_H*I, 200, Swirl, stage5_miner, -I);
	}

	FROM_TO(4260, 5000, 60) {
		create_enemy1c(VIEWPORT_W*(_i&1)+120*I, 400, Fairy, stage5_greeter, 6 * (1-2*(_i&1)) + I);
	}

	AT(5000) {
		create_enemy1c(VIEWPORT_W/2, 2000, BigFairy, stage5_lightburst, 2.0*I);
	}

	FROM_TO(5030, 5060, 30) {
		create_enemy1c(30.0*I+VIEWPORT_W*(_i&1), 1500, Fairy, stage5_superbullet, 2-4*(_i&1) + I);
	}

	AT(5180) {
		create_enemy1c(VIEWPORT_W/2+100, 2000, BigFairy, stage5_lightburst2, 2.0*I - 0.25);
	}

	FROM_TO(5060, 5300, 60-10*global.diff) {
		tsrand_fill(2);
		create_enemy1c(VIEWPORT_W+200.0*I*afrand(0), 500, Swirl, stage5_miner, -3+2.0*I*afrand(1));
	}

	AT(5360) {
		create_enemy1c(30.0*I+VIEWPORT_W, 1500, Fairy, stage5_superbullet, -2 + I);

	}

	AT(5390) {
		create_enemy1c(30.0*I, 1500, Fairy, stage5_superbullet, 2 + I);
	}

	AT(5500) {
		create_enemy1c(VIEWPORT_W+20 + VIEWPORT_H*0.6*I, 2000, BigFairy, stage5_lightburst, -2*I - 2);
		create_enemy1c(          -20 + VIEWPORT_H*0.6*I, 2000, BigFairy, stage5_lightburst, -2*I + 2);
	}

	AT(5600) {
		enemy_kill_all(&global.enemies);
	}

	{
		int cnt = 5;
		int step = 10;
		double ofs = 42*2;

		FROM_TO(5620, 5620 + step*cnt-1, step) {
			cmplx src1 = -ofs/4               + (-ofs/4 +      _i    * (VIEWPORT_H-2*ofs)/(cnt-1))*I;
			cmplx src2 = (VIEWPORT_W + ofs/4) + (-ofs/4 + (cnt-_i-1) * (VIEWPORT_H-2*ofs)/(cnt-1))*I;
			cmplx dst1 = ofs                  + ( ofs   +      _i    * (VIEWPORT_H-2*ofs)/(cnt-1))*I;
			cmplx dst2 = (VIEWPORT_W - ofs)   + ( ofs   + (cnt-_i-1) * (VIEWPORT_H-2*ofs)/(cnt-1))*I;

			create_enemy2c(src1, 2000, Swirl, stage5_magnetto, dst1, dst2);
			create_enemy2c(src2, 2000, Swirl, stage5_magnetto, dst2, dst1);
		}
	}

	FROM_TO(6100, 6350, 60-12*global.diff) {
		tsrand_fill(2);
		create_enemy1c(VIEWPORT_W+200.0*I*afrand(0), 500, Swirl, stage5_miner, -3+2.0*I*afrand(1));
	}

	FROM_TO(6300, 6350, 50) {
		create_enemy1c(VIEWPORT_W/4+VIEWPORT_W/2*!(_i&1), 2000, BigFairy, stage5_limiter, 2*I);
	}

	AT(6960) {
		stage_unlock_bgm("stage5");
		global.boss = create_iku();
	}

	AT(6980) {
		stage_unlock_bgm("stage5boss");
		stage5_dialog_post_boss();
	}

	AT(6985) {
		stage_finish(GAMEOVER_SCORESCREEN);
	}
}
