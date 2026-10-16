/***************************************************************************
   PSEUDO3D - ground plane from the map ( alpha, E1 )

   The floor is cast from the level's tiles, not from the finished screen:
   every playfield pixel below the horizon looks up a point of the map
   ( the ground behind and ahead of the ship ) through a camera with
   depth D = Dref^2 / ( y - horizon ). Near the ship the map is shown at
   the same scale as the flat game, far away it is compressed.
   Sprites are not drawn yet ( E2 ), so this step shows only the ground.
 ***************************************************************************/
#include <string.h>
#include <math.h>
#include "SDL.h"
#include "entypes.h"
#include "gfxapi.h"
#include "pseudo3d.h"
#include "tile.h"

#define P3D_STRIDE    320        /* row length of the 320x200 screen */
#define P3D_X0        16         /* first playfield column (MAP_LEFT) */
#define P3D_W         288        /* playfield width = 9 tiles of 32 px */
#define P3D_H         200        /* screen rows */
#define P3D_HORIZON   40         /* screen rows 0..HORIZON are sky */
#define P3D_SHIP_Y    170        /* screen row where the map is 1:1 with the flat game */
#define P3D_SKY       0          /* palette index of the sky */
#define P3D_WIN_ROWS  64         /* map rows kept in the world window */
#define P3D_WIN_BACK  40         /* window starts this many rows before the ship */
#define P3D_LINES     (P3D_WIN_ROWS * 32)

/* fog = repeated use of the game's DARK shade table: level 0 = none, 3 = darkest */
#define P3D_FOG_LEVELS 4
/* how dark the fog gets: levels added at the horizon and at the outer edge */
#define P3D_FOG_DEPTH_MAX 2.0
#define P3D_FOG_SIDE_MAX  0.8

extern char *dtable;

static int p3d_enabled;
static char p3d_world[P3D_LINES * P3D_W];
static unsigned char p3d_fog[P3D_FOG_LEVELS][256];
static int p3d_fog_ready;

/* 4x4 Bayer matrix, values 0..15 */
static const int P3D_BAYER[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 }
};

static void
P3D_MakeFog(
    void
)
{
    int k, i;

    for (i = 0; i < 256; i++)
        p3d_fog[0][i] = (unsigned char)i;

    for (k = 1; k < P3D_FOG_LEVELS; k++)
        for (i = 0; i < 256; i++)
            p3d_fog[k][i] = (unsigned char)dtable[p3d_fog[k - 1][i]];

    p3d_fog_ready = 1;
}

void
P3D_Init(
    int enabled
)
{
    p3d_enabled = enabled != 0;
}

int
P3D_Enabled(
    void
)
{
    return p3d_enabled;
}

void
P3D_Floor(
    char *screen,
    double shift
)
{
    int yoff = 0;
    int wrow0, sx, sy, cur_base;
    double dref = P3D_SHIP_Y - P3D_HORIZON;
    double mref, cx = (P3D_W - 1) / 2.0;

    if (!p3d_fog_ready)
        P3D_MakeFog();

    TILE_GetScroll(&cur_base, &yoff);

    wrow0 = cur_base - P3D_WIN_BACK;
    TILE_WorldRows(wrow0, P3D_WIN_ROWS, p3d_world);

    /* map pixel shown at the ship row, matching the flat playfield scroll */
    mref = (P3D_SHIP_Y - yoff) + cur_base * 32.0 + shift;

    for (sy = 0; sy < P3D_H; sy++)
    {
        char *dst = screen + sy * P3D_STRIDE + P3D_X0;

        if (sy <= P3D_HORIZON)
        {
            memset(dst, P3D_SKY, P3D_W);
            continue;
        }

        {
            double d = dref * dref / (sy - P3D_HORIZON);
            double msrc = mref + dref - d;
            double hs = dref / (sy - P3D_HORIZON);
            int line = (int)floor(msrc) - wrow0 * 32;

            /* depth fog: 0 at the ship row and below, full at the horizon */
            double far = (double)(P3D_SHIP_Y - sy) / (P3D_SHIP_Y - P3D_HORIZON);
            double depth_f = far <= 0 ? 0 : (far > 1 ? 1 : far);

            for (sx = 0; sx < P3D_W; sx++)
            {
                int col = (int)floor(cx + (sx - cx) * hs + 0.5);
                /* side fog: fades in over the outer half of the width */
                double edge = fabs(sx - cx) / cx;
                double side_f = edge > 0.5 ? (edge - 0.5) * 2.0 : 0.0;
                double level = depth_f * P3D_FOG_DEPTH_MAX + side_f * P3D_FOG_SIDE_MAX;
                int base, lvl, v;

                /* ordered dither (4x4 Bayer) between two levels: a soft fade, no bands */
                base = (int)floor(level);
                if (level - base > P3D_BAYER[sy & 3][sx & 3] / 16.0)
                    base++;
                lvl = base;

                /* wrap across the 288 px map width: far rows need more than the level has */
                col %= P3D_W;
                if (col < 0)
                    col += P3D_W;

                if (lvl >= P3D_FOG_LEVELS)
                    lvl = P3D_FOG_LEVELS - 1;

                if (line < 0 || line >= P3D_LINES)
                    v = P3D_SKY;
                else
                    v = p3d_world[line * P3D_W + col];

                dst[sx] = (char)p3d_fog[lvl][(unsigned char)v];
            }
        }
    }
}

/***************************************************************************
   P3D_DrawGround() - A sprite standing on the ground: its bottom edge is
   projected onto the floor ( same mapping as P3D_Floor, inverted ) and the
   sprite is scaled by the same factor, so it grows as it comes closer.
   Drawn into displaybuffer ( the playfield ) during the flat pass.
 ***************************************************************************/
#define P3D_SPR_OX  32           /* scratch position: clear of the screen edges */
#define P3D_SPR_OY  8
#define P3D_SCALE_MAX 3.0
#define P3D_AIR_LIFT  30.0       /* how high air objects hover above the ground, at scale 1 */

static void P3D_DrawProjected(const char *pic, int x, int y, double lift);

static char p3d_spr[320 * 200];

static void
P3D_DrawProjected(
    const char *pic,
    int x,
    int y,
    double lift            /* height above the ground, in flat screen pixels */
)
{
    const GFX_PIC *h = (const GFX_PIC*)pic;
    int w = LE_LONG(h->width);
    int hgt = LE_LONG(h->height);
    double dref = P3D_SHIP_Y - P3D_HORIZON;
    double cx = (P3D_W - 1) / 2.0;
    double yb = y + hgt;                  /* bottom edge, flat screen */
    double fx = x + w / 2.0;              /* centre, flat screen */
    double depth, sy, s, dw, dh, left, top;
    int r, c, dy, dx, sr, sc, px, py;
    char *saved;

    if (w <= 0 || hgt <= 0 || w > 320 - P3D_SPR_OX || hgt > 200 - P3D_SPR_OY)
    {
        GFX_PutSprite((char*)pic, x, y);
        return;
    }

    /* depth of the bottom edge: the ship row is depth dref */
    depth = dref - (yb - P3D_SHIP_Y);
    if (depth < 1)
        depth = 1;

    sy = P3D_HORIZON + dref * dref / depth;
    s = (sy - P3D_HORIZON) / dref;
    if (s > P3D_SCALE_MAX)
        s = P3D_SCALE_MAX;

    /* draw the sprite flat into the scratch buffer ( transparent = 0 ) */
    for (r = 0; r < hgt; r++)
        memset(p3d_spr + (P3D_SPR_OY + r) * 320 + P3D_SPR_OX, 0, w);

    saved = displaybuffer;
    displaybuffer = p3d_spr;
    GFX_PutSprite((char*)pic, P3D_SPR_OX, P3D_SPR_OY);
    displaybuffer = saved;

    /* copy it onto the floor: horizontal centre scaled from the ship column,
       bottom edge on the projected row */
    dw = w * s;
    dh = hgt * s;
    left = 16 + cx + (fx - 16 - cx) * s - dw / 2;
    top = sy - dh - lift * s;

    for (dy = 0; dy < (int)dh; dy++)
    {
        py = (int)floor(top + dy + 0.5);
        if (py < 0 || py >= P3D_H)
            continue;

        sr = (int)((dy + 0.5) / s);
        if (sr >= hgt)
            continue;

        for (dx = 0; dx < (int)dw; dx++)
        {
            unsigned char v;

            px = (int)floor(left + dx + 0.5);
            if (px < P3D_X0 || px >= P3D_X0 + P3D_W)
                continue;

            sc = (int)((dx + 0.5) / s);
            if (sc >= w)
                continue;

            v = (unsigned char)p3d_spr[(P3D_SPR_OY + sr) * 320 + P3D_SPR_OX + sc];
            if (v)
                displaybuffer[py * 320 + px] = (char)v;
        }
    }
}

void
P3D_DrawGround(
    const char *pic,
    int x,
    int y
)
{
    P3D_DrawProjected(pic, x, y, 0.0);
}

/* flying object: projected like a ground object, lifted above it, so it
   grows as it comes closer instead of sliding down the flat screen */
void
P3D_DrawAir(
    const char *pic,
    int x,
    int y
)
{
    P3D_DrawProjected(pic, x, y, P3D_AIR_LIFT);
}
