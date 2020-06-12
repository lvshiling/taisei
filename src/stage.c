/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "util.h"
#include "stage.h"
#include "global.h"
#include "video.h"
#include "resource/bgm.h"
#include "replay.h"
#include "config.h"
#include "player.h"
#include "menu/ingamemenu.h"
#include "menu/gameovermenu.h"
#include "audio/audio.h"
#include "log.h"
#include "stagetext.h"
#include "stagedraw.h"
#include "stageobjects.h"
#include "eventloop/eventloop.h"
#include "common_tasks.h"
#include "stageinfo.h"

static void stage_start(StageInfo *stage) {
	global.timer = 0;
	global.frames = 0;
	global.gameover = 0;
	global.voltage_threshold = 0;

	player_stage_pre_init(&global.plr);

	stats_stage_reset(&global.plr.stats);

	if(stage->type == STAGE_SPELL) {
		global.is_practice_mode = true;
		global.plr.lives = 0;
		global.plr.bombs = 0;
	} else if(global.is_practice_mode) {
		global.plr.lives = PLR_STGPRACTICE_LIVES;
		global.plr.bombs = PLR_STGPRACTICE_BOMBS;
	}

	if(global.is_practice_mode) {
		global.plr.power = config_get_int(CONFIG_PRACTICE_POWER);
	}

	if(global.plr.power < 0) {
		global.plr.power = 0;
	} else if(global.plr.power > PLR_MAX_POWER) {
		global.plr.power = PLR_MAX_POWER;
	}

	reset_all_sfx();
}

static bool ingame_menu_interrupts_bgm(void) {
	return global.stage->type != STAGE_SPELL;
}

static void stage_fade_bgm(void) {
	audio_bgm_stop((FPS * FADE_TIME) / 2000.0);
}

static void stage_leave_ingame_menu(CallChainResult ccr) {
	MenuData *m = ccr.result;

	if(m->state != MS_Dead) {
		return;
	}

	if(global.gameover > 0) {
		stop_all_sfx();

		if(ingame_menu_interrupts_bgm() || global.gameover != GAMEOVER_RESTART) {
			stage_fade_bgm();
		}
	} else {
		resume_all_sfx();
		audio_bgm_resume();
	}

	CallChain *cc = ccr.ctx;
	run_call_chain(cc, NULL);
	free(cc);
}

static void stage_enter_ingame_menu(MenuData *m, CallChain next) {
	if(ingame_menu_interrupts_bgm()) {
		audio_bgm_pause();
	}

	pause_all_sfx();
	enter_menu(m, CALLCHAIN(stage_leave_ingame_menu, memdup(&next, sizeof(next))));
}

void stage_pause(void) {
	if(global.gameover == GAMEOVER_TRANSITIONING || taisei_is_skip_mode_enabled()) {
		return;
	}

	stage_enter_ingame_menu(
		(global.replaymode == REPLAY_PLAY
			? create_ingame_menu_replay
			: create_ingame_menu
		)(), NO_CALLCHAIN
	);
}

void stage_gameover(void) {
	if(global.stage->type == STAGE_SPELL && config_get_int(CONFIG_SPELLSTAGE_AUTORESTART)) {
		global.gameover = GAMEOVER_RESTART;
		return;
	}

	stage_enter_ingame_menu(create_gameover_menu(), NO_CALLCHAIN);
}

static bool stage_input_common(SDL_Event *event, void *arg) {
	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	switch(type) {
		case TE_GAME_KEY_DOWN:
			switch(code) {
				case KEY_STOP:
					stage_finish(GAMEOVER_DEFEAT);
					return true;

				case KEY_RESTART:
					stage_finish(GAMEOVER_RESTART);
					return true;
			}

			break;

		case TE_GAME_PAUSE:
			stage_pause();
			break;

		default:
			break;
	}

	return false;
}

static bool stage_input_key_filter(KeyIndex key, bool is_release) {
	if(key == KEY_HAHAIWIN) {
		IF_DEBUG(
			if(!is_release) {
				stage_finish(GAMEOVER_WIN);
			}
		);

		return false;
	}

	IF_NOT_DEBUG(
		if(
			key == KEY_IDDQD ||
			key == KEY_POWERUP ||
			key == KEY_POWERDOWN
		) {
			return false;
		}
	);

	if(stage_is_cleared()) {
		if(key == KEY_SHOT) {
			if(
				global.gameover == GAMEOVER_SCORESCREEN &&
				global.frames - global.gameover_time > GAMEOVER_SCORE_DELAY * 2
			) {
				if(!is_release) {
					stage_finish(GAMEOVER_WIN);
				}
			}
		}

		if(key == KEY_BOMB || key == KEY_SPECIAL) {
			return false;
		}
	}

	return true;
}

static bool stage_input_handler_gameplay(SDL_Event *event, void *arg) {
	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	if(stage_input_common(event, arg)) {
		return false;
	}

	switch(type) {
		case TE_GAME_KEY_DOWN:
			if(stage_input_key_filter(code, false)) {
				player_event_with_replay(&global.plr, EV_PRESS, code);
			}
			break;

		case TE_GAME_KEY_UP:
			if(stage_input_key_filter(code, true)) {
				player_event_with_replay(&global.plr, EV_RELEASE, code);
			}
			break;

		case TE_GAME_AXIS_LR:
			player_event_with_replay(&global.plr, EV_AXIS_LR, (uint16_t)code);
			break;

		case TE_GAME_AXIS_UD:
			player_event_with_replay(&global.plr, EV_AXIS_UD, (uint16_t)code);
			break;

		default: break;
	}

	return false;
}

static bool stage_input_handler_replay(SDL_Event *event, void *arg) {
	stage_input_common(event, arg);
	return false;
}

static void replay_input(void) {
	ReplayStage *s = global.replay_stage;
	int i = 0;

	events_poll((EventHandler[]){
		{ .proc = stage_input_handler_replay },
		{NULL}
	}, EFLAG_GAME);

	for(i = s->playpos; i < s->events.num_elements; ++i) {
		ReplayEvent *e = dynarray_get_ptr(&s->events, i);

		if(e->frame != global.frames) {
			break;
		}

		switch(e->type) {
			case EV_OVER:
				global.gameover = GAMEOVER_DEFEAT;
				break;

			case EV_CHECK_DESYNC:
				s->desync_check = e->value;
				break;

			case EV_FPS:
				s->fps = e->value;
				break;

			default: {
				player_event(&global.plr, e->type, (int16_t)e->value, NULL, NULL);
				break;
			}
		}
	}

	s->playpos = i;
	player_applymovement(&global.plr);
}

static void display_bgm_title(void) {
	BGM *bgm = audio_bgm_current();
	const char *title = bgm ? bgm_get_title(bgm) : NULL;

	if(title) {
		char txt[strlen(title) + 6];
		snprintf(txt, sizeof(txt), "BGM: %s", title);
		stagetext_add(txt, VIEWPORT_W-15 + I * (VIEWPORT_H-20), ALIGN_RIGHT, res_font("standard"), RGB(1, 1, 1), 30, 180, 35, 35);
	}
}

static bool stage_handle_bgm_change(SDL_Event *evt, void *a) {
	BGM *bgm = evt->user.data1;

	if(dialog_is_active(global.dialog)) {
		INVOKE_TASK_WHEN(&global.dialog->events.fadeout_began, common_call_func, display_bgm_title);
	} else {
		display_bgm_title();
	}

	return false;
}

static void stage_input(void) {
	events_poll((EventHandler[]){
		{ .proc = stage_input_handler_gameplay },
		{ .proc = stage_handle_bgm_change, .event_type = MAKE_TAISEI_EVENT(TE_AUDIO_BGM_STARTED) },
		{NULL}
	}, EFLAG_GAME);
	player_fix_input(&global.plr);
	player_applymovement(&global.plr);
}

#ifdef DEBUG
static const char *_skip_to_bookmark;
bool _skip_to_dialog;

void _stage_bookmark(const char *name) {
	log_debug("Bookmark [%s] reached at %i", name, global.frames);

	if(_skip_to_bookmark && !strcmp(_skip_to_bookmark, name)) {
		_skip_to_bookmark = NULL;
		global.plr.iddqd = false;
	}
}

DEFINE_EXTERN_TASK(stage_bookmark) {
	_stage_bookmark(ARGS.name);
}
#endif

static bool _stage_should_skip(void) {
#ifdef DEBUG
	if(_skip_to_bookmark || _skip_to_dialog) {
		return true;
	}
#endif
	return false;
}

static void stage_logic(void) {
	process_boss(&global.boss);
	process_enemies(&global.enemies);
	process_projectiles(&global.projs, true);
	process_items();
	process_lasers();
	process_projectiles(&global.particles, false);

	if(global.dialog) {
		dialog_update(global.dialog);

		if((global.plr.inputflags & INFLAG_SKIP) && dialog_is_active(global.dialog)) {
			dialog_page(global.dialog);
		}
	}

	if(_stage_should_skip()) {
		if(dialog_is_active(global.dialog)) {
			dialog_page(global.dialog);
		}

		if(global.boss) {
			ent_damage(&global.boss->ent, &(DamageInfo) { 400, DMG_PLAYER_SHOT } );
		}
	}

	update_all_sfx();

	global.frames++;

	if(!dialog_is_active(global.dialog) && (!global.boss || boss_is_fleeing(global.boss))) {
		global.timer++;
	}

	if(global.replaymode == REPLAY_PLAY && global.gameover != GAMEOVER_TRANSITIONING) {
		ReplayStage *rstg = global.replay_stage;
		ReplayEvent *last_event = dynarray_get_ptr(&rstg->events, rstg->events.num_elements - 1);

		if(global.frames == last_event->frame - FADE_TIME) {
			stage_finish(GAMEOVER_DEFEAT);
		}
	}

	stagetext_update();
}

void stage_clear_hazards_predicate(bool (*predicate)(EntityInterface *ent, void *arg), void *arg, ClearHazardsFlags flags) {
	bool force = flags & CLEAR_HAZARDS_FORCE;

	if(flags & CLEAR_HAZARDS_BULLETS) {
		for(Projectile *p = global.projs.first, *next; p; p = next) {
			next = p->next;

			if(!force && !projectile_is_clearable(p)) {
				continue;
			}

			if(!predicate || predicate(&p->ent, arg)) {
				clear_projectile(p, flags);
			}
		}
	}

	if(flags & CLEAR_HAZARDS_LASERS) {
		for(Laser *l = global.lasers.first, *next; l; l = next) {
			next = l->next;

			if(!force && !laser_is_clearable(l)) {
				continue;
			}

			if(!predicate || predicate(&l->ent, arg)) {
				clear_laser(l, flags);
			}
		}
	}
}

void stage_clear_hazards(ClearHazardsFlags flags) {
	stage_clear_hazards_predicate(NULL, NULL, flags);
}

static bool proximity_predicate(EntityInterface *ent, void *varg) {
	Circle *area = varg;

	switch(ent->type) {
		case ENT_TYPE_ID(Projectile): {
			Projectile *p = ENT_CAST(ent, Projectile);
			return cabs(p->pos - area->origin) < area->radius;
		}

		case ENT_TYPE_ID(Laser): {
			Laser *l = ENT_CAST(ent, Laser);
			return laser_intersects_circle(l, *area);
		}

		default: UNREACHABLE;
	}
}

static bool ellipse_predicate(EntityInterface *ent, void *varg) {
	Ellipse *e = varg;

	switch(ent->type) {
		case ENT_TYPE_ID(Projectile): {
			Projectile *p = ENT_CAST(ent, Projectile);
			return point_in_ellipse(p->pos, *e);
		}

		case ENT_TYPE_ID(Laser): {
			Laser *l = ENT_CAST(ent, Laser);
			return laser_intersects_ellipse(l, *e);
		}

		default: UNREACHABLE;
	}
}

void stage_clear_hazards_at(cmplx origin, double radius, ClearHazardsFlags flags) {
	Circle area = { origin, radius };
	stage_clear_hazards_predicate(proximity_predicate, &area, flags);
}

void stage_clear_hazards_in_ellipse(Ellipse e, ClearHazardsFlags flags) {
	stage_clear_hazards_predicate(ellipse_predicate, &e, flags);
}

TASK(clear_dialog, NO_ARGS) {
	assert(global.dialog != NULL);
	// dialog_deinit() should've been called by dialog_end() at this point
	global.dialog = NULL;
}

TASK(dialog_fixup_timer, NO_ARGS) {
	// HACK: remove when global.timer is gone
	global.timer++;
}

void stage_begin_dialog(Dialog *d) {
	assert(global.dialog == NULL);
	global.dialog = d;
	dialog_init(d);
	INVOKE_TASK_WHEN(&d->events.fadeout_began, dialog_fixup_timer);
	INVOKE_TASK_WHEN(&d->events.fadeout_ended, clear_dialog);
}

static void stage_free(void) {
	delete_enemies(&global.enemies);
	delete_items();
	delete_lasers();

	delete_projectiles(&global.projs);
	delete_projectiles(&global.particles);

	if(global.dialog) {
		dialog_deinit(global.dialog);
		global.dialog = NULL;
	}

	if(global.boss) {
		free_boss(global.boss);
		global.boss = NULL;
	}

	projectiles_free();
	lasers_free();
	stagetext_free();
}

static void stage_finalize(void *arg) {
	global.gameover = (intptr_t)arg;
}

void stage_finish(int gameover) {
	if(global.gameover == GAMEOVER_TRANSITIONING) {
		return;
	}

	int prev_gameover = global.gameover;
	global.gameover_time = global.frames;

	if(gameover == GAMEOVER_SCORESCREEN) {
		global.gameover = GAMEOVER_SCORESCREEN;
	} else {
		global.gameover = GAMEOVER_TRANSITIONING;
		set_transition_callback(TransFadeBlack, FADE_TIME, FADE_TIME*2, stage_finalize, (void*)(intptr_t)gameover);
		stage_fade_bgm();
	}

	if(
		global.replaymode != REPLAY_PLAY &&
		prev_gameover != GAMEOVER_SCORESCREEN &&
		(gameover == GAMEOVER_SCORESCREEN || gameover == GAMEOVER_WIN)
	) {
		StageProgress *p = stageinfo_get_progress(global.stage, global.diff, true);

		if(p) {
			++p->num_cleared;
			log_debug("Stage cleared %u times now", p->num_cleared);
		}
	}
}

static void stage_preload(void) {
	difficulty_preload();
	projectiles_preload();
	player_preload();
	items_preload();
	boss_preload();
	lasers_preload();
	enemies_preload();

	if(global.stage->type != STAGE_SPELL) {
		dialog_preload();
	}

	global.stage->procs->preload();
}

static void display_stage_title(StageInfo *info) {
	stagetext_add(info->title,    VIEWPORT_W/2 + I * (VIEWPORT_H/2-40), ALIGN_CENTER, res_font("big"), RGB(1, 1, 1), 50, 85, 35, 35);
	stagetext_add(info->subtitle, VIEWPORT_W/2 + I * (VIEWPORT_H/2),    ALIGN_CENTER, res_font("standard"), RGB(1, 1, 1), 60, 85, 35, 35);
}

void stage_start_bgm(const char *bgm) {
#if 0
	char *old_title = NULL;

	if(current_bgm.title && global.stage->type == STAGE_SPELL) {
		old_title = strdup(current_bgm.title);
	}

	start_bgm(bgm);

	if(current_bgm.title && current_bgm.started_at >= 0 && (!old_title || strcmp(current_bgm.title, old_title))) {
		if(dialog_is_active(global.dialog)) {
			INVOKE_TASK_WHEN(&global.dialog->events.fadeout_began, common_call_func, display_bgm_title);
		} else {
			display_bgm_title();
		}
	}

	free(old_title);
#else
	audio_bgm_play(res_bgm(bgm), true, 0, 0);
#endif
}

void stage_set_voltage_thresholds(uint easy, uint normal, uint hard, uint lunatic) {
	switch(global.diff) {
		case D_Easy:    global.voltage_threshold = easy;    return;
		case D_Normal:  global.voltage_threshold = normal;  return;
		case D_Hard:    global.voltage_threshold = hard;    return;
		case D_Lunatic: global.voltage_threshold = lunatic; return;
		default: UNREACHABLE;
	}
}

bool stage_is_cleared(void) {
	return global.gameover == GAMEOVER_SCORESCREEN || global.gameover == GAMEOVER_TRANSITIONING;
}

typedef struct StageFrameState {
	StageInfo *stage;
	CallChain cc;
	CoSched sched;
	int transition_delay;
	int logic_calls;
	uint16_t last_replay_fps;
	float view_shake;
} StageFrameState;

static StageFrameState *_current_stage_state;  // TODO remove this shitty hack

static void stage_update_fps(StageFrameState *fstate) {
	if(global.replaymode == REPLAY_RECORD) {
		uint16_t replay_fps = (uint16_t)rint(global.fps.logic.fps);

		if(replay_fps != fstate->last_replay_fps) {
			replay_stage_event(global.replay_stage, global.frames, EV_FPS, replay_fps);
			fstate->last_replay_fps = replay_fps;
		}
	}
}

static void stage_give_clear_bonus(const StageInfo *stage, StageClearBonus *bonus) {
	memset(bonus, 0, sizeof(*bonus));

	// FIXME: this is clunky...
	if(!global.is_practice_mode && stage->type == STAGE_STORY) {
		StageInfo *next = stageinfo_get_by_id(stage->id + 1);

		if(next == NULL || next->type != STAGE_STORY) {
			bonus->all_clear = true;
		}
	}

	if(stage->type == STAGE_STORY) {
		bonus->base = stage->id * 1000000;
	}

	if(bonus->all_clear) {
		bonus->base += global.plr.point_item_value * 100;
		bonus->graze = global.plr.graze * (global.plr.point_item_value / 10);
	}

	bonus->voltage = imax(0, (int)global.plr.voltage - (int)global.voltage_threshold) * (global.plr.point_item_value / 25);
	bonus->lives = global.plr.lives * global.plr.point_item_value * 5;

	// TODO: maybe a difficulty multiplier?

	bonus->total = bonus->base + bonus->voltage + bonus->lives;
	player_add_points(&global.plr, bonus->total, global.plr.pos);
}

INLINE bool stage_should_yield(void) {
	return (global.boss && !boss_is_fleeing(global.boss)) || dialog_is_active(global.dialog);
}

static LogicFrameAction stage_logic_frame(void *arg) {
	StageFrameState *fstate = arg;
	StageInfo *stage = fstate->stage;

	++fstate->logic_calls;

	stage_update_fps(fstate);

	if(_stage_should_skip()) {
		global.plr.iddqd = true;
	}

	fapproach_p(&fstate->view_shake, 0, 1);
	fapproach_asymptotic_p(&fstate->view_shake, 0, 0.05, 1e-2);

	((global.replaymode == REPLAY_PLAY) ? replay_input : stage_input)();

	if(global.gameover != GAMEOVER_TRANSITIONING) {
		cosched_run_tasks(&fstate->sched);

		if(!stage_should_yield()) {
			stage->procs->event();
		}

		if(global.gameover == GAMEOVER_SCORESCREEN && global.frames - global.gameover_time == GAMEOVER_SCORE_DELAY) {
			StageClearBonus b;
			stage_give_clear_bonus(stage, &b);
			stage_display_clear_screen(&b);
		}

		if(stage->type == STAGE_SPELL && !global.boss && !fstate->transition_delay) {
			fstate->transition_delay = 120;
		}

		stage->procs->update();
	}

	replay_stage_check_desync(global.replay_stage, global.frames, (rng_u64() ^ global.plr.points) & 0xFFFF, global.replaymode);
	stage_logic();

	if(fstate->transition_delay) {
		if(!--fstate->transition_delay) {
			stage_finish(GAMEOVER_WIN);
		}
	} else {
		update_transition();
	}

	if(global.replaymode == REPLAY_RECORD && global.plr.points > progress.hiscore) {
		progress.hiscore = global.plr.points;
	}

	if(global.gameover > 0) {
		log_warn("RESTART");
		return LFRAME_STOP;
	}

#ifdef DEBUG
	if(_skip_to_dialog && dialog_is_active(global.dialog)) {
		_skip_to_dialog = false;
		global.plr.iddqd = false;
	}

	taisei_set_skip_mode(_stage_should_skip());

	if(taisei_is_skip_mode_enabled() || gamekeypressed(KEY_SKIP)) {
		return LFRAME_SKIP;
	}
#endif

	if(global.frameskip || (global.replaymode == REPLAY_PLAY && gamekeypressed(KEY_SKIP))) {
		return LFRAME_SKIP;
	}

	return LFRAME_WAIT;
}

static RenderFrameAction stage_render_frame(void *arg) {
	StageFrameState *fstate = arg;
	StageInfo *stage = fstate->stage;

	if(_stage_should_skip()) {
		return RFRAME_DROP;
	}

	rng_lock(&global.rand_game);
	rng_make_active(&global.rand_visual);
	BEGIN_DRAW_CODE();
	stage_draw_scene(stage);
	END_DRAW_CODE();
	rng_unlock(&global.rand_game);
	rng_make_active(&global.rand_game);
	draw_transition();

	return RFRAME_SWAP;
}

static void stage_end_loop(void *ctx);

static void stage_stub_proc(void) { }

void stage_enter(StageInfo *stage, CallChain next) {
	assert(stage);
	assert(stage->procs);

	#define STUB_PROC(proc, stub) do {\
		if(!stage->procs->proc) { \
			stage->procs->proc = stub; \
			log_debug(#proc " proc is missing"); \
		} \
	} while(0)

	static const ShaderRule shader_rules_stub[1] = { NULL };

	STUB_PROC(preload, stage_stub_proc);
	STUB_PROC(begin, stage_stub_proc);
	STUB_PROC(end, stage_stub_proc);
	STUB_PROC(draw, stage_stub_proc);
	STUB_PROC(event, stage_stub_proc);
	STUB_PROC(update, stage_stub_proc);
	STUB_PROC(shader_rules, (ShaderRule*)shader_rules_stub);

	if(global.gameover == GAMEOVER_WIN) {
		global.gameover = 0;
	} else if(global.gameover) {
		run_call_chain(&next, NULL);
		return;
	}

	// I really want to separate all of the game state from the global struct sometime
	global.stage = stage;

	ent_init();
	stage_objpools_alloc();
	stage_draw_pre_init();
	stage_preload();
	stage_draw_init();

	rng_make_active(&global.rand_game);
	stage_start(stage);

	if(global.replaymode == REPLAY_RECORD) {
		uint64_t start_time = (uint64_t)time(0);
		uint64_t seed = makeseed();
		rng_seed(&global.rand_game, seed);

		global.replay_stage = replay_create_stage(&global.replay, stage, start_time, seed, global.diff, &global.plr);

		// make sure our player state is consistent with what goes into the replay
		player_init(&global.plr);
		replay_stage_sync_player_state(global.replay_stage, &global.plr);

		log_debug("Start time: %"PRIu64, start_time);
		log_debug("Random seed: 0x%"PRIx64, seed);

		StageProgress *p = stageinfo_get_progress(stage, global.diff, true);

		if(p) {
			log_debug("You played this stage %u times", p->num_played);
			log_debug("You cleared this stage %u times", p->num_cleared);

			++p->num_played;
			p->unlocked = true;
		}
	} else {
		ReplayStage *stg = global.replay_stage;

		assert(stg != NULL);
		assert(stageinfo_get_by_id(stg->stage) == stage);

		rng_seed(&global.rand_game, stg->rng_seed);

		log_debug("REPLAY_PLAY mode: %d events, stage: \"%s\"", stg->events.num_elements, stage->title);
		log_debug("Start time: %"PRIu64, stg->start_time);
		log_debug("Random seed: 0x%"PRIx64, stg->rng_seed);

		global.diff = stg->diff;
		player_init(&global.plr);
		replay_stage_sync_player_state(stg, &global.plr);
		stg->playpos = 0;
	}

	StageFrameState *fstate = calloc(1 , sizeof(*fstate));
	cosched_init(&fstate->sched);
	cosched_set_invoke_target(&fstate->sched);
	fstate->stage = stage;
	fstate->cc = next;

	_current_stage_state = fstate;

	#ifdef DEBUG
	_skip_to_dialog = env_get_int("TAISEI_SKIP_TO_DIALOG", 0);
	_skip_to_bookmark = env_get_string_nonempty("TAISEI_SKIP_TO_BOOKMARK", NULL);
	taisei_set_skip_mode(_stage_should_skip());
	#endif

	stage->procs->begin();
	player_stage_post_init(&global.plr);

	if(global.stage->type != STAGE_SPELL) {
		display_stage_title(stage);
	}

	eventloop_enter(fstate, stage_logic_frame, stage_render_frame, stage_end_loop, FPS);
}

void stage_end_loop(void* ctx) {
	StageFrameState *s = ctx;
	assert(s == _current_stage_state);

	if(global.replaymode == REPLAY_RECORD) {
		replay_stage_event(global.replay_stage, global.frames, EV_OVER, 0);
		global.replay_stage->plr_points_final = global.plr.points;

		if(global.gameover == GAMEOVER_WIN) {
			global.replay_stage->flags |= REPLAY_SFLAG_CLEAR;
		}
	}

	s->stage->procs->end();
	stage_draw_shutdown();
	stage_free();
	player_free(&global.plr);
	cosched_finish(&s->sched);
	rng_make_active(&global.rand_visual);
	free_all_refs();
	ent_shutdown();
	stage_objpools_free();
	stop_all_sfx();

	taisei_commit_persistent_data();
	taisei_set_skip_mode(false);

	if(taisei_quit_requested()) {
		global.gameover = GAMEOVER_ABORT;
	}

	_current_stage_state = NULL;

	CallChain cc = s->cc;
	free(s);

	run_call_chain(&cc, NULL);
}

void stage_unlock_bgm(const char *bgm) {
	if(global.replaymode != REPLAY_PLAY && !global.plr.stats.total.continues_used) {
		progress_unlock_bgm(bgm);
	}
}

void stage_shake_view(float strength) {
	assume(strength >= 0);
	_current_stage_state->view_shake += strength;
}

float stage_get_view_shake_strength(void) {
	if(_current_stage_state) {
		return _current_stage_state->view_shake;
	}

	return 0;
}
