#pragma once

#include <stdint.h>

// What a recorded drawing operation belongs to ( used to match it between game ticks )
enum
{
    IK_NONE, IK_TILE, IK_ENEMY, IK_SHOT, IK_ESHOT, IK_BONUS, IK_ANIM,
    IK_SHADOW, IK_GSHADOW, IK_PLAYER
};

// Recorded drawing operations
enum
{
    IOP_SPRITE, IOP_SHADESHAPE, IOP_TILE, IOP_SHADOW, IOP_LINE,
    IOP_FLAMEUP, IOP_FLAMEDOWN, IOP_COLORBOX, IOP_VLINE
};

extern int interp_recording;

void INTERP_Init(int enabled, int max_fps);
int INTERP_Enabled(void);
void INTERP_Reset(void);
void INTERP_BeginTick(void);
void INTERP_Key(int kind, intptr_t id);
void INTERP_Overlay(int on);
void INTERP_Op(int op, char *pic, int a, int b, int c, int d, int e);
void INTERP_EndTick(void);
void INTERP_Render(double alpha, int shake);
