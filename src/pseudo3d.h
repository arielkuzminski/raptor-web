/***************************************************************************
   PSEUDO3D - ground plane ( alpha )

   The floor is cast from the level's tiles ( see pseudo3d.cpp ) into the
   playfield of the 320x200 screen. The HUD is not affected: it is drawn in
   its own layer by interp.cpp.
 ***************************************************************************/
#ifndef PSEUDO3D_H
#define PSEUDO3D_H

void P3D_Init(int enabled);
int P3D_Enabled(void);

/* screen = the 320x200 8-bit buffer; the playfield is columns 16..303.
   shift = sub-pixel scroll of this frame ( 0..1 ), as in interp.cpp */
void P3D_Floor(char *screen, double shift);

#endif /* PSEUDO3D_H */
