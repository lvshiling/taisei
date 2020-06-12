/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_audio_audio_h
#define IGUARD_audio_audio_h

#include "taisei.h"

#include "resource/sfx.h"
#include "resource/bgm.h"
#include "resource/bgm_metadata.h"

#define LOOPTIMEOUTFRAMES 10
#define DEFAULT_SFX_VOLUME 100

#define LOOPFADEOUT 50

typedef struct SFXImpl SFXImpl;

typedef enum {
	LS_OFF,
	LS_LOOPING,
	LS_FADEOUT,
} LoopState;

typedef struct SFX {
	SFXImpl *impl;
	LoopState islooping;
	int lastplayframe;
} SFX;

typedef enum BGMStatus {
	BGM_STOPPED,
	BGM_PLAYING,
	BGM_PAUSED,
} BGMStatus;

typedef uint64_t SFXPlayID;

void audio_init(void);
void audio_shutdown(void);
bool audio_output_works(void);

bool audio_bgm_play(BGM *bgm, bool loop, double position, double fadein);
bool audio_bgm_play_or_restart(BGM *bgm, bool loop, double position, double fadein);
bool audio_bgm_stop(double fadeout);
bool audio_bgm_pause(void);
bool audio_bgm_resume(void);
double audio_bgm_seek(double pos);
double audio_bgm_seek_realtime(double rtpos);
double audio_bgm_tell(void);
BGMStatus audio_bgm_status(void);
bool audio_bgm_looping(void);
BGM *audio_bgm_current(void);

SFXPlayID play_sfx(const char *name) attr_nonnull(1);
SFXPlayID play_sfx_ex(const char *name, int cooldown, bool replace) attr_nonnull(1);
void play_sfx_delayed(const char *name, int cooldown, bool replace, int delay) attr_nonnull(1);
void play_sfx_loop(const char *name) attr_nonnull(1);
void play_sfx_ui(const char *name) attr_nonnull(1);
void stop_sound(SFXPlayID sid);
void replace_sfx(SFXPlayID sid, const char *name) attr_nonnull(2);
void reset_all_sfx(void);
void pause_all_sfx(void);
void resume_all_sfx(void);
void stop_all_sfx(void);
void update_all_sfx(void); // checks if loops need to be stopped

int get_default_sfx_volume(const char *sfx);

DEFINE_DEPRECATED_RESOURCE_GETTER(SFX, get_sound, res_sfx)
DEFINE_DEPRECATED_RESOURCE_GETTER(BGM, get_music, res_bgm)

attr_deprecated("Use play_sfx() instead")
INLINE SFXPlayID play_sound(const char *name) {
	return play_sfx(name);
}

attr_deprecated("Use play_sfx_ex() instead") attr_nonnull(1)
INLINE SFXPlayID play_sound_ex(const char *name, int cooldown, bool replace) {
	return play_sfx_ex(name, cooldown, replace);
}

double audioutil_loopaware_position(double rt_pos, double duration, double loop_start);

#endif // IGUARD_audio_audio_h
