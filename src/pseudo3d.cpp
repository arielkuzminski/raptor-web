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

static int p3d_enabled;
static char p3d_world[P3D_LINES * P3D_W];

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

            for (sx = 0; sx < P3D_W; sx++)
            {
                int col = (int)floor(cx + (sx - cx) * hs + 0.5);

                /* wrap across the 288 px map width: far rows need more than the level has */
                col %= P3D_W;
                if (col < 0)
                    col += P3D_W;

                if (line < 0 || line >= P3D_LINES)
                    dst[sx] = P3D_SKY;
                else
                    dst[sx] = p3d_world[line * P3D_W + col];
            }
        }
    }
}
