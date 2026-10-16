/***************************************************************************
   PSEUDO3D - Mode 7 style ground plane ( alpha )

   The finished playfield ( 288x200, 8-bit ) is warped row by row into a
   tilted plane: rows near the bottom keep their size, rows towards the
   horizon are compressed. Logic is untouched; only what is drawn changes.
   The HUD is not warped ( it is drawn in its own layer by interp.cpp ).
 ***************************************************************************/
#ifndef PSEUDO3D_H
#define PSEUDO3D_H

void P3D_Init(int enabled);
int P3D_Enabled(void);

/* screen = the 320x200 8-bit buffer with the playfield in columns 16..303 */
void P3D_Warp(char *screen);

#endif /* PSEUDO3D_H */
