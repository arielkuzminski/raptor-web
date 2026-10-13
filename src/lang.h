#pragma once

#include "gfxapi.h"

typedef struct
{
    int width;                   // glyph width in pixels (advance)
    int height;                  // glyph height in pixels
    int yoff;                    // offset from text line top (negative = above)
    char *data;                  // width * height shade values, 0 = transparent
}LANG_GLYPH;

void LANG_Init(const char *code);
const char *LANG_Code(void);
const char *LANG_Item(const char *glbname, int *size);
const char *LANG_Str(const char *english);
int LANG_SwdExtra(const char *glbname);
void LANG_PatchSwd(char *swd, int filesize, const char *glbname);
const LANG_GLYPH *LANG_Glyph(FONT *font, unsigned char ch);
