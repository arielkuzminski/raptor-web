/***************************************************************************
   INTERP - smooth frames between game ticks

   The game logic runs at 70 / 3 = 23.3 ticks per second and draws one
   frame per tick. To show more frames without touching the logic, the
   drawing calls of a tick are recorded instead of executed ( every
   display function still runs exactly once, with all its side effects ).
   Between ticks the recorded list is replayed at the monitor's refresh
   rate, with positions interpolated between the previous and the current
   tick.

   Scrolling moves the ground 1 pixel per tick, which cannot be shown in
   between at 320x200. The ground layer is therefore drawn at whole pixels
   and the whole playfield is shifted by the fraction when it is scaled to
   the screen ( i_video.cpp ). The HUD goes into a separate overlay layer
   so it does not move with it.
 ***************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unordered_map>
#include <vector>
#include "SDL.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "common.h"
#include "gfxapi.h"
#include "flame.h"
#include "i_video.h"
#include "rap.h"
#include "tile.h"
#include "interp.h"
#include "pseudo3d.h"

void TILE_Put(char *inpic, int x, int y);
void SHADOW_Draw(char *pic, int x, int y);

#define MAX_JUMP 48              // farther than this between ticks = new object, no interpolation

typedef struct
{
    uint8_t op;
    uint8_t kind;
    uint8_t overlay;
    uint8_t ground;              // ground-attached object ( projected onto the pseudo-3D floor )
    intptr_t id;
    int sub;
    char *pic;
    int v[5];
    int prev;                    // index of the same object in the previous tick, or -1
}IOP;

int interp_recording;

static int enabled;
static int max_fps;              // 0 = every screen refresh
static double last_frame;
static std::vector<IOP> ops[2];
static int cur;                  // ops[cur] = this tick, ops[cur ^ 1] = previous tick
static std::unordered_map<uint64_t, int> counters;
static int key_kind;
static intptr_t key_id;
static int interp_ground;
static int overlay;
static int scroll;               // ground moved by this many pixels since the previous tick
static char overlay8[64000];
static char overlay_mask[64000];

static uint64_t
Key(
    int kind,
    intptr_t id,
    int sub
)
{
    uint64_t k = (uint64_t)(uintptr_t)id * 0x9E3779B97F4A7C15ULL;
    return k ^ ((uint64_t)kind << 56) ^ ((uint64_t)sub << 40);
}

/***************************************************************************
   INTERP_Init() - Turns smooth frames on / off ( [Video] interp )
 ***************************************************************************/
void
INTERP_Init(
    int on,
    int fps_limit
)
{
    enabled = on;
    max_fps = fps_limit > 0 ? fps_limit : 0;
    INTERP_Reset();
}

int
INTERP_Enabled(
    void
)
{
    return enabled;
}

/***************************************************************************
   INTERP_Reset() - Forget the previous tick ( new wave, after a window )
 ***************************************************************************/
void
INTERP_Reset(
    void
)
{
    ops[0].clear();
    ops[1].clear();
    interp_recording = 0;
}

/***************************************************************************
   INTERP_BeginTick() - Start recording the drawing of one game tick
 ***************************************************************************/
void
INTERP_BeginTick(
    void
)
{
    if (!enabled)
        return;

    cur ^= 1;
    ops[cur].clear();
    counters.clear();
    key_kind = IK_NONE;
    key_id = 0;
    overlay = 0;
    interp_ground = 0;
    interp_recording = 1;
}

/***************************************************************************
   INTERP_Key() - Following operations belong to this object
 ***************************************************************************/
void
INTERP_Key(
    int kind,
    intptr_t id
)
{
    key_kind = kind;
    key_id = id;
}

/***************************************************************************
   INTERP_Ground() - Following sprites are attached to the ground ( pseudo-3D
                     projects them onto the floor ) when on = 1
 ***************************************************************************/
void
INTERP_Ground(
    int on
)
{
    interp_ground = on;
}

/***************************************************************************
   INTERP_Overlay() - Following operations are HUD ( not scrolled )
 ***************************************************************************/
void
INTERP_Overlay(
    int on
)
{
    overlay = on;
    key_kind = IK_NONE;
    key_id = 0;
}

/***************************************************************************
   INTERP_Op() - Records one drawing operation
 ***************************************************************************/
void
INTERP_Op(
    int op,
    char *pic,
    int a,
    int b,
    int c,
    int d,
    int e
)
{
    IOP rec;
    uint64_t base = Key(key_kind, key_id, 0);

    rec.op = op;
    rec.kind = key_kind;
    rec.overlay = overlay;
    rec.ground = (uint8_t)interp_ground;
    rec.id = key_id;
    rec.sub = counters[base]++;
    rec.pic = pic;
    rec.v[0] = a;
    rec.v[1] = b;
    rec.v[2] = c;
    rec.v[3] = d;
    rec.v[4] = e;
    rec.prev = -1;

    ops[cur].push_back(rec);
}

/***************************************************************************
   INTERP_EndTick() - Stop recording, match objects with the previous tick
 ***************************************************************************/
void
INTERP_EndTick(
    void
)
{
    std::unordered_map<uint64_t, int> index;
    std::vector<IOP> &now = ops[cur];
    std::vector<IOP> &old = ops[cur ^ 1];
    int tile_moves[3] = {0, 0, 0};

    if (!enabled)
        return;

    interp_recording = 0;

#ifdef __EMSCRIPTEN__
    EM_ASM({ Module.raptorTicks = (Module.raptorTicks || 0) + 1; });
#endif

    for (int i = 0; i < (int)old.size(); i++)
        index[Key(old[i].kind, old[i].id, old[i].sub) ^ old[i].op] = i;

    for (int i = 0; i < (int)now.size(); i++)
    {
        IOP &rec = now[i];

        if (rec.kind == IK_NONE)
            continue;

        std::unordered_map<uint64_t, int>::iterator it =
            index.find(Key(rec.kind, rec.id, rec.sub) ^ rec.op);

        if (it == index.end())
            continue;

        IOP &was = old[it->second];

        if (abs(was.v[0] - rec.v[0]) > MAX_JUMP || abs(was.v[1] - rec.v[1]) > MAX_JUMP)
            continue;

        rec.prev = it->second;

        if (rec.kind == IK_TILE && rec.v[1] - was.v[1] >= 0 && rec.v[1] - was.v[1] <= 2)
            tile_moves[rec.v[1] - was.v[1]]++;
    }

    // the ground moved by what most tiles moved
    scroll = 0;
    if (tile_moves[1] > tile_moves[scroll])
        scroll = 1;
    if (tile_moves[2] > tile_moves[scroll])
        scroll = 2;
}

/*------------------------------------------------------------------------
   Replay() - Executes one recorded operation at position x, y
  ------------------------------------------------------------------------*/
static void
Replay(
    const IOP &rec,
    int x,
    int y,
    int x2,
    int y2
)
{
    switch (rec.op)
    {
    case IOP_SPRITE:
        /* flat: the ship ( at the camera ), HUD-like pop-ups ( money ) and pickups */
        if (!P3D_Enabled() || rec.kind == IK_PLAYER || rec.kind == IK_BONUS
            || (rec.kind == IK_ANIM && !rec.ground))
            GFX_PutSprite(rec.pic, x, y);
        else if (rec.ground)
            P3D_DrawGround(rec.pic, x, y);
        else
            P3D_DrawAir(rec.pic, x, y);
        break;
    case IOP_SHADESHAPE:
        GFX_ShadeShape(rec.v[2], rec.pic, x, y);
        break;
    case IOP_TILE:
        TILE_Put(rec.pic, x, y);
        break;
    case IOP_SHADOW:
        SHADOW_Draw(rec.pic, x, y);
        break;
    case IOP_LINE:
        GFX_Line(x, y, x2, y2, rec.v[4]);
        break;
    case IOP_FLAMEUP:
        FLAME_Up(x, y, rec.v[2], rec.v[3]);
        break;
    case IOP_FLAMEDOWN:
        FLAME_Down(x, y, rec.v[2], rec.v[3]);
        break;
    case IOP_COLORBOX:
        GFX_ColorBox(x, y, rec.v[2], rec.v[3], rec.v[4]);
        break;
    case IOP_VLINE:
        GFX_VLine(x, y, rec.v[2], rec.v[3]);
        break;
    }
}

static int
Lerp(
    int from,
    int to,
    double alpha,
    double shift
)
{
    return (int)floor(from + (to - from) * alpha - shift + 0.5);
}

/***************************************************************************
   INTERP_Render() - Draws the last tick at alpha ( 0 = previous tick,
                     1 = current tick ) and shows it
 ***************************************************************************/
void
INTERP_Render(
    double alpha,
    int shake
)
{
    std::vector<IOP> &now = ops[cur];
    std::vector<IOP> &old = ops[cur ^ 1];
    char *screen = displaybuffer;
    double shift;
    int pass, i;

    if (now.empty())
        return;

    // frame limit: skip this screen refresh if the last frame is too recent
    // ( 1 ms slack so 60 fps on a 60 Hz screen never skips )
    if (max_fps)
    {
        double t = I_NowMs();

        if (t - last_frame < 1000.0 / max_fps - 1.0)
            return;

        last_frame = t;
    }

#ifdef __EMSCRIPTEN__
    EM_ASM({ Module.raptorFrames = (Module.raptorFrames || 0) + 1; });
#endif

    if (alpha < 0)
        alpha = 0;
    if (alpha > 1)
        alpha = 1;

    // the playfield is shifted down by this fraction of a pixel on screen,
    // so everything is drawn that much higher
    shift = scroll * alpha;
    if (shift >= 1)
        shift = 1;

    // pseudo-3D: the ground is cast from the map first; the sprites are then drawn over it
    if (P3D_Enabled())
        P3D_Floor(screen, shift);

    for (pass = 0; pass < 3; pass++)
    {
        // pass 0: playfield, pass 1 + 2: HUD over 0 and over 255 ( drawn pixels = mask )
        if (pass == 1)
        {
            memset(overlay8, 0, sizeof(overlay8));
            displaybuffer = overlay8;
        }
        else if (pass == 2)
        {
            memset(overlay_mask, 255, sizeof(overlay_mask));
            displaybuffer = overlay_mask;
        }

        for (i = 0; i < (int)now.size(); i++)
        {
            const IOP &rec = now[i];
            int x = rec.v[0], y = rec.v[1], x2 = rec.v[2], y2 = rec.v[3];

            if ((pass == 0) == (rec.overlay != 0))
                continue;

            // in pseudo-3D the map tiles are replaced by the floor
            if (pass == 0 && rec.op == IOP_TILE && P3D_Enabled())
                continue;

            if (pass == 0)
            {
                if (rec.prev >= 0)
                {
                    const IOP &was = old[rec.prev];

                    x = Lerp(was.v[0], rec.v[0], alpha, 0);
                    y = Lerp(was.v[1], rec.v[1], alpha, shift);

                    if (rec.op == IOP_LINE)
                    {
                        x2 = Lerp(was.v[2], rec.v[2], alpha, 0);
                        y2 = Lerp(was.v[3], rec.v[3], alpha, shift);
                    }
                }
                else
                {
                    // new this tick: hold it where the ground will carry it
                    y = Lerp(rec.v[1], rec.v[1], alpha, shift);
                    if (rec.op == IOP_LINE)
                        y2 = Lerp(rec.v[3], rec.v[3], alpha, shift);
                }
            }

            Replay(rec, x, y, x2, y2);
        }
    }

    displaybuffer = screen;

    for (i = 0; i < 64000; i++)
        overlay_mask[i] = overlay_mask[i] == overlay8[i];

    I_SetPlayfield(MAP_LEFT, 288, shift);
    I_SetOverlay(overlay8, overlay_mask);
    I_SetVSyncPaced(1);

    if (shake)
        TILE_ShakeScreen();
    else
        TILE_DisplayScreen();

    I_SetVSyncPaced(0);
    I_SetOverlay(NULL, NULL);
    I_SetPlayfield(MAP_LEFT, 288, 0);
}
