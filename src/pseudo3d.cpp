/***************************************************************************
   PSEUDO3D - Mode 7 style ground plane ( alpha )
   See pseudo3d.h. Camera looks straight ahead; one row of the playfield
   corresponds to a depth z = 1 / ( y - horizon ), the bottom row is the
   nearest. Horizontal scale grows with depth, so distant rows are wider in
   source and look compressed on screen.
 ***************************************************************************/
#include <string.h>
#include <math.h>
#include "pseudo3d.h"

#define P3D_STRIDE   320         /* row length of the 320x200 screen */
#define P3D_X0       16          /* first playfield column (MAP_LEFT) */
#define P3D_W        288         /* playfield width */
#define P3D_H        200         /* playfield height */
#define P3D_HORIZON  30          /* rows 0..HORIZON are sky */
#define P3D_SKY      0           /* palette index of the sky */
#define P3D_MAX_HSCALE 4.0       /* limit horizontal stretch near the horizon */

static int p3d_enabled;
static char p3d_src[P3D_W * P3D_H];

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
P3D_Warp(
    char *screen
)
{
    const double cx = (P3D_W - 1) / 2.0;
    int x, y;

    for (y = 0; y < P3D_H; y++)
        memcpy(p3d_src + y * P3D_W, screen + y * P3D_STRIDE + P3D_X0, P3D_W);

    for (y = 0; y < P3D_H; y++)
    {
        char *dst = screen + y * P3D_STRIDE + P3D_X0;
        double t, sy, hscale;
        int row;

        if (y <= P3D_HORIZON)
        {
            memset(dst, P3D_SKY, P3D_W);
            continue;
        }

        /* t = 0 at the horizon, 1 at the bottom row; source row = 2H*t/(1+t):
           the bottom keeps its size, the rows towards the horizon shrink */
        t = (double)(y - P3D_HORIZON) / (P3D_H - 1 - P3D_HORIZON);
        sy = (P3D_H - 1) * 2.0 * t / (1.0 + t);
        hscale = 1.0 / t;
        if (hscale > P3D_MAX_HSCALE)
            hscale = P3D_MAX_HSCALE;

        if (sy < 0)
            sy = 0;
        if (sy > P3D_H - 1)
            sy = P3D_H - 1;
        row = (int)(sy + 0.5);

        for (x = 0; x < P3D_W; x++)
        {
            int sx = (int)floor(cx + (x - cx) * hscale + 0.5);

            if (sx < 0)
                sx = 0;
            if (sx > P3D_W - 1)
                sx = P3D_W - 1;

            dst[x] = p3d_src[row * P3D_W + sx];
        }
    }
}
