/***************************************************************************
   LANG - translations

   A language file (lang/<code>.lng, UTF-8) replaces the game's *_TXT items,
   window field labels and a few strings from the code:

     @@ STRINGS
     HANGAR_SWD.MISSION = LOT NA MISJĘ      <- window field (WINDOW.FIELD)
     Abort Mission ? = Przerwać misję ?     <- string used in the code
     @@ STORY1_TXT
     TEXT_COLOR 84                          <- whole item, same syntax as
     ...                                       the original *_TXT

   The original fonts only have ASCII glyphs. Accented letters are encoded
   to bytes 128..255 and their glyphs are composed at runtime from the
   base letter of the same font plus a diacritic mark.
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <map>
#include <string>
#include "SDL.h"
#include "common.h"
#include "entypes.h"
#include "lang.h"

#define ASIZE(a) (sizeof(a) / sizeof((a)[0]))

enum
{
    MK_NONE, MK_ACUTE, MK_GRAVE, MK_CIRC, MK_DIAER, MK_TILDE, MK_RING,
    MK_CARON, MK_DOT, MK_CEDILLA, MK_OGONEK, MK_STROKE, MK_CARON_R, MK_ROT180
};

typedef struct
{
    uint32_t cp;                 // unicode code point
    unsigned char base;          // ASCII letter the glyph is built from
    int mark;                    // diacritic added to it
}LANG_CHAR;

// Byte 128 + index in this table is the in-game code of each character.
static const LANG_CHAR charmap[] = {
    {0xC0,'A',MK_GRAVE},  {0xC1,'A',MK_ACUTE},  {0xC2,'A',MK_CIRC},   {0xC3,'A',MK_TILDE},
    {0xC4,'A',MK_DIAER},  {0xC5,'A',MK_RING},   {0xC7,'C',MK_CEDILLA},{0xC8,'E',MK_GRAVE},
    {0xC9,'E',MK_ACUTE},  {0xCA,'E',MK_CIRC},   {0xCB,'E',MK_DIAER},  {0xCC,'I',MK_GRAVE},
    {0xCD,'I',MK_ACUTE},  {0xCE,'I',MK_CIRC},   {0xCF,'I',MK_DIAER},  {0xD1,'N',MK_TILDE},
    {0xD2,'O',MK_GRAVE},  {0xD3,'O',MK_ACUTE},  {0xD4,'O',MK_CIRC},   {0xD5,'O',MK_TILDE},
    {0xD6,'O',MK_DIAER},  {0xD9,'U',MK_GRAVE},  {0xDA,'U',MK_ACUTE},  {0xDB,'U',MK_CIRC},
    {0xDC,'U',MK_DIAER},  {0xDD,'Y',MK_ACUTE},  {0x178,'Y',MK_DIAER},
    {0xE0,'a',MK_GRAVE},  {0xE1,'a',MK_ACUTE},  {0xE2,'a',MK_CIRC},   {0xE3,'a',MK_TILDE},
    {0xE4,'a',MK_DIAER},  {0xE5,'a',MK_RING},   {0xE7,'c',MK_CEDILLA},{0xE8,'e',MK_GRAVE},
    {0xE9,'e',MK_ACUTE},  {0xEA,'e',MK_CIRC},   {0xEB,'e',MK_DIAER},  {0xEC,'i',MK_GRAVE},
    {0xED,'i',MK_ACUTE},  {0xEE,'i',MK_CIRC},   {0xEF,'i',MK_DIAER},  {0xF1,'n',MK_TILDE},
    {0xF2,'o',MK_GRAVE},  {0xF3,'o',MK_ACUTE},  {0xF4,'o',MK_CIRC},   {0xF5,'o',MK_TILDE},
    {0xF6,'o',MK_DIAER},  {0xF9,'u',MK_GRAVE},  {0xFA,'u',MK_ACUTE},  {0xFB,'u',MK_CIRC},
    {0xFC,'u',MK_DIAER},  {0xFD,'y',MK_ACUTE},  {0xFF,'y',MK_DIAER},
    {0x104,'A',MK_OGONEK},{0x105,'a',MK_OGONEK},{0x106,'C',MK_ACUTE}, {0x107,'c',MK_ACUTE},
    {0x10C,'C',MK_CARON}, {0x10D,'c',MK_CARON}, {0x10E,'D',MK_CARON}, {0x10F,'d',MK_CARON_R},
    {0x118,'E',MK_OGONEK},{0x119,'e',MK_OGONEK},{0x11A,'E',MK_CARON}, {0x11B,'e',MK_CARON},
    {0x141,'L',MK_STROKE},{0x142,'l',MK_STROKE},{0x143,'N',MK_ACUTE}, {0x144,'n',MK_ACUTE},
    {0x147,'N',MK_CARON}, {0x148,'n',MK_CARON}, {0x158,'R',MK_CARON}, {0x159,'r',MK_CARON},
    {0x15A,'S',MK_ACUTE}, {0x15B,'s',MK_ACUTE}, {0x160,'S',MK_CARON}, {0x161,'s',MK_CARON},
    {0x164,'T',MK_CARON}, {0x165,'t',MK_CARON_R},{0x16E,'U',MK_RING}, {0x16F,'u',MK_RING},
    {0x179,'Z',MK_ACUTE}, {0x17A,'z',MK_ACUTE}, {0x17B,'Z',MK_DOT},   {0x17C,'z',MK_DOT},
    {0x17D,'Z',MK_CARON}, {0x17E,'z',MK_CARON},
    {0xA1,'!',MK_ROT180}, {0xBF,'?',MK_ROT180},
};

#define NUM_CHARS ((int)(sizeof(charmap) / sizeof(charmap[0])))

// Characters written with plain ASCII instead of a new glyph
static const struct { uint32_t cp; const char *text; } substitutes[] = {
    {0xDF,"ss"}, {0x1E9E,"SS"}, {0xE6,"ae"}, {0xC6,"AE"}, {0x153,"oe"}, {0x152,"OE"},
    {0x133,"ij"}, {0x132,"IJ"}, {0x2018,"'"}, {0x2019,"'"}, {0x201A,"'"},
    {0x201C,"\""}, {0x201D,"\""}, {0x201E,"\""}, {0xAB,"\""}, {0xBB,"\""},
    {0x2013,"-"}, {0x2014,"-"}, {0x2026,"..."}, {0xA0," "},
};

static std::string lang_code;
static std::map<std::string, std::string> lang_items;
static std::map<std::string, std::string> lang_strings;

/*------------------------------------------------------------------------
   Encode() - UTF-8 text to in-game bytes
  ------------------------------------------------------------------------*/
static std::string
Encode(
    const std::string &in
)
{
    std::string out;
    size_t i = 0;

    while (i < in.size())
    {
        unsigned char c = in[i];
        uint32_t cp;
        int extra;

        if (c < 0x80)      { cp = c; extra = 0; }
        else if (c < 0xE0) { cp = c & 0x1F; extra = 1; }
        else if (c < 0xF0) { cp = c & 0x0F; extra = 2; }
        else               { cp = c & 0x07; extra = 3; }

        i++;
        for (; extra > 0 && i < in.size(); extra--, i++)
            cp = (cp << 6) | (in[i] & 0x3F);

        if (cp < 0x80)
        {
            out += (char)cp;
            continue;
        }

        int found = 0;

        for (int n = 0; n < NUM_CHARS && !found; n++)
        {
            if (charmap[n].cp == cp)
            {
                out += (char)(128 + n);
                found = 1;
            }
        }

        for (size_t n = 0; n < ASIZE(substitutes) && !found; n++)
        {
            if (substitutes[n].cp == cp)
            {
                out += substitutes[n].text;
                found = 1;
            }
        }

        if (!found)
            out += '?';
    }

    return out;
}

static std::string
Trim(
    const std::string &s
)
{
    size_t a = s.find_first_not_of(" \t\r");
    size_t b = s.find_last_not_of(" \t\r");

    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}

/***************************************************************************
   LANG_Init() - Loads lang/<code>.lng ( "en" or empty = original texts )
 ***************************************************************************/
void
LANG_Init(
    const char *code
)
{
    char path[64];
    FILE *handle;
    char line[512];
    std::string section;
    std::string *item = NULL;

    lang_items.clear();
    lang_strings.clear();
    lang_code = code ? code : "";

    if (lang_code.empty() || lang_code == "en" || lang_code.size() > 8)
        return;

#ifdef __EMSCRIPTEN__
    snprintf(path, sizeof(path), "/lang/%s.lng", lang_code.c_str());
#else
    snprintf(path, sizeof(path), "lang/%s.lng", lang_code.c_str());
#endif

    if ((handle = fopen(path, "rb")) == NULL)
    {
        printf("LANG: %s not found, using English\n", path);
        lang_code.clear();
        return;
    }

    while (fgets(line, sizeof(line), handle))
    {
        std::string s = line;

        while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
            s.pop_back();

        if (s.compare(0, 2, "@@") == 0)
        {
            section = Trim(s.substr(2));
            item = section == "STRINGS" ? NULL : &lang_items[section];
            continue;
        }

        if (item)
        {
            *item += Encode(s) + "\r\n";
        }
        else if (section == "STRINGS")
        {
            size_t eq = s.find(" = ");

            if (s.empty() || s[0] == '#' || eq == std::string::npos)
                continue;

            lang_strings[Trim(s.substr(0, eq))] = Encode(Trim(s.substr(eq + 3)));
        }
    }

    fclose(handle);

    printf("LANG: %s - %d texts, %d strings\n", lang_code.c_str(),
        (int)lang_items.size(), (int)lang_strings.size());
}

/***************************************************************************
   LANG_Code() - Current language code ( "" = original )
 ***************************************************************************/
const char *
LANG_Code(
    void
)
{
    return lang_code.c_str();
}

/***************************************************************************
   LANG_Item() - Translated GLB item ( *_TXT ) or NULL
 ***************************************************************************/
const char *
LANG_Item(
    const char *glbname,
    int *size
)
{
    std::map<std::string, std::string>::iterator it = lang_items.find(glbname);

    if (it == lang_items.end())
        return NULL;

    *size = (int)it->second.size();
    return it->second.c_str();
}

/***************************************************************************
   LANG_Str() - Translated string from the code, or the English original
 ***************************************************************************/
const char *
LANG_Str(
    const char *english
)
{
    static std::string centered[4];
    static int next;

    if (!english || lang_strings.empty())
        return english;

    std::map<std::string, std::string>::iterator it = lang_strings.find(english);

    if (it != lang_strings.end())
        return it->second.c_str();

    // "    NEXT ITEM": original is centered with spaces - center the translation too
    std::string orig = english;
    std::string key = Trim(orig);
    size_t lead = orig.find_first_not_of(' ');

    if (lead == std::string::npos || (it = lang_strings.find(key)) == lang_strings.end())
        return english;

    int pad = ((int)(lead * 2 + key.size()) - (int)it->second.size()) / 2;
    std::string &out = centered[next];
    next = (next + 1) % 4;
    out = std::string(pad > 0 ? pad : 0, ' ') + it->second;

    return out.c_str();
}

/*------------------------------------------------------------------------
   Window ( *_SWD ) labels

   A field keeps its text inside the window item, right after the fields,
   with no room to grow. Translated labels are appended at the end of the
   item and the field's text offset / length are pointed at them.
   Keys: "HANGAR_SWD.MISSION", or "OPTS_SWD.#12" for fields without a name.
  ------------------------------------------------------------------------*/
#define SWIN_FLDOFS   76         // offsets in the SWD file layout ( swdapi.h )
#define SWIN_NUMFLDS  96
#define SFLD_SIZE     148
#define SFLD_OPT      0
#define SFLD_NAME     32
#define SFLD_MAXCHARS 92
#define SFLD_TXTOFF   140

#define SWD_MAX_FIELDS 64        // room reserved per window ...
#define SWD_FIELD_CAP  48        // ... so the code can set longer translated texts
#define FLD_INPUT      3

static std::string
SwdKey(
    const char *glbname,
    const char *fldname,
    int index
)
{
    char num[16];

    if (fldname[0])
        return std::string(glbname) + "." + std::string(fldname, strnlen(fldname, 16));

    snprintf(num, sizeof(num), ".#%d", index);
    return std::string(glbname) + num;
}

/***************************************************************************
   LANG_SwdExtra() - Bytes needed for translated labels of a window
 ***************************************************************************/
int
LANG_SwdExtra(
    const char *glbname
)
{
    std::string prefix = std::string(glbname) + ".";
    int extra = SWD_MAX_FIELDS * SWD_FIELD_CAP;

    if (lang_strings.empty())
        return 0;

    for (std::map<std::string, std::string>::iterator it = lang_strings.lower_bound(prefix);
         it != lang_strings.end() && it->first.compare(0, prefix.size(), prefix) == 0; ++it)
    {
        extra += (int)it->second.size() + 1;
    }

    return extra;
}

/***************************************************************************
   LANG_PatchSwd() - Points window fields at translated labels
 ***************************************************************************/
void
LANG_PatchSwd(
    char *swd,                   // INOUT: window item, filesize + LANG_SwdExtra()
    int filesize,                // INPUT: size of the original item
    const char *glbname          // INPUT: GLB item name ( HANGAR_SWD )
)
{
    int fldofs, numflds, end = filesize;
    int capacity = filesize + LANG_SwdExtra(glbname);

    if (capacity == filesize)
        return;

    memcpy(&fldofs, swd + SWIN_FLDOFS, 4);
    memcpy(&numflds, swd + SWIN_NUMFLDS, 4);
    fldofs = LE_LONG(fldofs);
    numflds = LE_LONG(numflds);

    for (int loop = 0; loop < numflds; loop++)
    {
        char *fld = swd + fldofs + loop * SFLD_SIZE;
        int opt, maxchars, txtoff;

        if (fld + SFLD_SIZE > swd + filesize)
            break;

        memcpy(&opt, fld + SFLD_OPT, 4);
        memcpy(&maxchars, fld + SFLD_MAXCHARS, 4);
        memcpy(&txtoff, fld + SFLD_TXTOFF, 4);
        opt = LE_LONG(opt);
        maxchars = LE_LONG(maxchars);
        txtoff = LE_LONG(txtoff);

        // input fields keep their limit ( pilot name length )
        if (maxchars <= 0 || opt == FLD_INPUT)
            continue;

        std::map<std::string, std::string>::iterator it =
            lang_strings.find(SwdKey(glbname, fld + SFLD_NAME, loop));
        std::string text = it != lang_strings.end() ? it->second
            : std::string(fld + txtoff, strnlen(fld + txtoff, maxchars));
        int cap = (int)text.size() + 1;

        if (cap < SWD_FIELD_CAP)
            cap = SWD_FIELD_CAP;

        if (end + cap > capacity)
            break;

        memset(swd + end, 0, cap);
        memcpy(swd + end, text.c_str(), text.size());

        txtoff = LE_LONG(end - (int)(fld - swd));
        memcpy(fld + SFLD_TXTOFF, &txtoff, 4);
        cap = LE_LONG(cap);
        memcpy(fld + SFLD_MAXCHARS, &cap, 4);
        end += LE_LONG(cap);
    }
}

/*========================================================================
   Glyphs
 ========================================================================*/

typedef struct
{
    FONT *font;
    int sig;
    LANG_GLYPH glyph[NUM_CHARS];
    char built[NUM_CHARS];
}GLYPH_CACHE;

#define MAX_CACHE 8
#define PAD 8

static GLYPH_CACHE cache[MAX_CACHE];
static int cache_next;

static int
FontSig(
    FONT *font
)
{
    return LE_LONG(font->height) * 131 + font->width['A'] * 17 + LE_SHORT(font->charofs['B']);
}

static const char *const marks[][4] = {
    /* MK_NONE    */ {NULL},
    /* MK_ACUTE   */ {".#", "#.", NULL},
    /* MK_GRAVE   */ {"#.", ".#", NULL},
    /* MK_CIRC    */ {".#.", "#.#", NULL},
    /* MK_DIAER   */ {"#.#", NULL},
    /* MK_TILDE   */ {".#.#", "#.#.", NULL},
    /* MK_RING    */ {".#.", "#.#", ".#.", NULL},
    /* MK_CARON   */ {"#.#", ".#.", NULL},
    /* MK_DOT     */ {"#", NULL},
    /* MK_CEDILLA */ {".#", "#.", NULL},
    /* MK_OGONEK  */ {"#.", ".#", NULL},
    /* MK_STROKE  */ {NULL},
    /* MK_CARON_R */ {"#", "#", NULL},
    /* MK_ROT180  */ {NULL},
};

static void
Stamp(
    char *grid,
    int gw,
    int gh,
    int mark,
    int left,
    int top,
    int scale,
    char color
)
{
    for (int row = 0; marks[mark][row]; row++)
    {
        for (int col = 0; marks[mark][row][col]; col++)
        {
            if (marks[mark][row][col] != '#')
                continue;

            for (int a = 0; a < scale; a++)
            {
                for (int b = 0; b < scale; b++)
                {
                    int x = left + col * scale + a;
                    int y = top + row * scale + b;

                    if (x >= 0 && x < gw && y >= 0 && y < gh)
                        grid[y * gw + x] = color;
                }
            }
        }
    }
}

static int
MarkRows(
    int mark
)
{
    int n = 0;

    while (marks[mark][n])
        n++;

    return n;
}

static int
MarkCols(
    int mark
)
{
    return marks[mark][0] ? (int)strlen(marks[mark][0]) : 0;
}

/*------------------------------------------------------------------------
   BuildGlyph() - Composes base letter + diacritic for one font
  ------------------------------------------------------------------------*/
static int
BuildGlyph(
    FONT *font,
    const LANG_CHAR *lc,
    LANG_GLYPH *out
)
{
    if (LE_SHORT(font->charofs[lc->base]) == (short)-1)
        return 0;

    int w = font->width[lc->base];
    int h = LE_LONG(font->height);
    const char *src = (char*)font + sizeof(FONT) + LE_SHORT(font->charofs[lc->base]);
    int gw = w + PAD * 2;
    int gh = h + PAD * 2;
    char *grid = (char*)calloc(gw * gh, 1);
    int scale = h >= 12 ? 2 : 1;
    int counts[256] = {0};
    char color = 1;
    int x, y;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            char v = src[y * w + x];

            if (lc->mark == MK_ROT180)
                grid[(PAD + h - 1 - y) * gw + PAD + w - 1 - x] = v;
            else
                grid[(PAD + y) * gw + PAD + x] = v;

            if (v)
                counts[(unsigned char)v]++;
        }
    }

    for (int v = 1, best = 0; v < 256; v++)
    {
        if (counts[v] > best)
        {
            best = counts[v];
            color = (char)v;
        }
    }

    // 'i' loses its dot before getting an accent
    if (lc->base == 'i')
    {
        int first = -1, gap = -1;

        for (y = PAD; y < PAD + h && gap < 0; y++)
        {
            int any = 0;

            for (x = PAD; x < PAD + w; x++)
                any |= grid[y * gw + x];

            if (any && first < 0)
                first = y;
            else if (!any && first >= 0)
                gap = y;
        }

        if (gap > 0)
            for (y = first; y < gap; y++)
                memset(grid + y * gw + PAD, 0, w);
    }

    // Bounding box of the letter
    int minx = gw, maxx = -1, miny = gh, maxy = -1;

    for (y = 0; y < gh; y++)
    {
        for (x = 0; x < gw; x++)
        {
            if (grid[y * gw + x])
            {
                if (x < minx) minx = x;
                if (x > maxx) maxx = x;
                if (y < miny) miny = y;
                if (y > maxy) maxy = y;
            }
        }
    }

    if (maxx < 0)
    {
        free(grid);
        return 0;
    }

    int mw = MarkCols(lc->mark) * scale;
    int mh = MarkRows(lc->mark) * scale;
    int center = (minx + maxx + 1) / 2;
    int gap = h <= 6 ? 0 : scale;

    switch (lc->mark)
    {
    case MK_ACUTE:
    case MK_GRAVE:
    case MK_CIRC:
    case MK_DIAER:
    case MK_TILDE:
    case MK_RING:
    case MK_CARON:
    case MK_DOT:
        Stamp(grid, gw, gh, lc->mark, center - mw / 2, miny - gap - mh, scale, color);
        break;

    case MK_CEDILLA:
        Stamp(grid, gw, gh, lc->mark, center - mw / 2, maxy + 1, scale, color);
        break;

    case MK_OGONEK:
        Stamp(grid, gw, gh, lc->mark, maxx - scale + 1, maxy + 1, scale, color);
        break;

    case MK_CARON_R:
        Stamp(grid, gw, gh, lc->mark, maxx + 1 + scale, miny, scale, color);
        break;

    case MK_STROKE:
    {
        // diagonal line across the vertical stem ( column with most pixels )
        int stem = minx, best = 0;

        for (x = minx; x <= maxx; x++)
        {
            int n = 0;

            for (y = miny; y <= maxy; y++)
                n += grid[y * gw + x] != 0;

            if (n > best)
            {
                best = n;
                stem = x;
            }
        }

        // the fonts are bold: find the whole width of the stem
        int stem_l = stem, stem_r = stem;

        for (int side = -1; side <= 1; side += 2)
        {
            for (x = stem + side; x >= minx && x <= maxx; x += side)
            {
                int n = 0;

                for (y = miny; y <= maxy; y++)
                    n += grid[y * gw + x] != 0;

                if (n * 10 < best * 7)
                    break;

                if (side < 0) stem_l = x; else stem_r = x;
            }
        }

        // slash through the stem: one step left-down, two steps right-up
        int mid = (miny + maxy) / 2 + (lc->base == 'L' ? 1 : 0);
        int px[3] = { stem_l - scale, stem_r + 1, stem_r + 1 + scale };
        int py[3] = { mid + scale, mid - scale + 1, mid - scale * 2 + 1 };

        for (int t = 0; t < 3; t++)
            for (int a = 0; a < scale; a++)
                for (int b = 0; b < scale; b++)
                    grid[(py[t] + b) * gw + px[t] + a] = color;
        break;
    }
    }

    // Crop: keep the original advance, grow up/down/right where marks stick out
    int top = PAD, bottom = PAD + h, right = PAD + w;

    for (y = 0; y < gh; y++)
    {
        for (x = PAD; x < gw; x++)
        {
            if (grid[y * gw + x])
            {
                if (y < top) top = y;
                if (y + 1 > bottom) bottom = y + 1;
                if (x + 1 > right) right = x + 1;
            }
        }
    }

    out->width = right - PAD;
    out->height = bottom - top;
    out->yoff = top - PAD;
    out->data = (char*)malloc(out->width * out->height);

    for (y = 0; y < out->height; y++)
        memcpy(out->data + y * out->width, grid + (top + y) * gw + PAD, out->width);

    free(grid);
    return 1;
}

/***************************************************************************
   LANG_Glyph() - Glyph for an in-game byte >= 128, or NULL
 ***************************************************************************/
const LANG_GLYPH *
LANG_Glyph(
    FONT *font,
    unsigned char ch
)
{
    int idx = ch - 128;
    int sig;
    GLYPH_CACHE *gc = NULL;

    if (idx < 0 || idx >= NUM_CHARS || !font)
        return NULL;

    sig = FontSig(font);

    for (int n = 0; n < MAX_CACHE; n++)
    {
        if (cache[n].font == font && cache[n].sig == sig)
        {
            gc = &cache[n];
            break;
        }
    }

    if (!gc)
    {
        gc = &cache[cache_next];
        cache_next = (cache_next + 1) % MAX_CACHE;

        for (int n = 0; n < NUM_CHARS; n++)
            free(gc->glyph[n].data);

        memset(gc, 0, sizeof(*gc));
        gc->font = font;
        gc->sig = sig;
    }

    if (!gc->built[idx])
    {
        gc->built[idx] = BuildGlyph(font, &charmap[idx], &gc->glyph[idx]) ? 1 : 2;
    }

    return gc->built[idx] == 1 ? &gc->glyph[idx] : NULL;
}
