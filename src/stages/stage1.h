/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef STAGE1_H
#define STAGE1_H

#include "stage.h"

extern struct stage1_spells_s {
    // this struct must contain only fields of type AttackInfo
    // order of fields affects the visual spellstage number, but not its real internal ID

    struct {
        AttackInfo perfect_freeze;
    } mid;

    struct {
        AttackInfo crystal_rain;
        AttackInfo icicle_fall;
    } boss;

    struct {
        AttackInfo crystal_blizzard;
    } extra;

    // required for iteration
    AttackInfo null;
} stage1_spells;

extern StageProcs stage1_procs;
extern StageProcs stage1_spell_procs;

#endif
