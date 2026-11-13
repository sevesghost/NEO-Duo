/* This file is part of Gngeo.
 * Copyright (C) the Gngeo team.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <gngeoTypes.h>

extern GN_Surface *buffer;

extern GN_Rect visible_area;

int screen_init();
int screen_reinit(void);
int screen_resize(int w, int h);
void screen_update(void *emuTaskPtr, void *neoSystemPtr, void *emuVideoPtr);
void screen_close();
