/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"
#include "../wriggle.h"

#include "global.h"

DEFINE_EXTERN_TASK(stage3_boss_nonspell_3) {
    Boss *boss = INIT_BOSS_ATTACK();
    BEGIN_BOSS_ATTACK();
}
