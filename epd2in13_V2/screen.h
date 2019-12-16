#ifndef SCREEN_H
#define SCREEN_H

// Display resolution
#define EPD_WIDTH 122
#define EPD_HEIGHT 250

#define LINEBYTES 16
#define LINEBITS (LINEBYTES * 8)

#include "fonts.h"

// #define UNIT 0
#ifdef UNIT
#include <stdio.h>
#else
#include <avr/pgmspace.h>
#endif
#include <stdlib.h>


class Screen {
    public:
        Screen(int sections);
        ~Screen();
        unsigned char *GetLine(int x);
        int DefineSection(int section, int lines, sFONT *font);
        void AddText(int section, char *txt);
        void Print();

    private:
        const uint8_t ***secPtrs;
        sFONT **secFonts;
        int *secCap;
        int *secWidth;
        int *secHeight;
        int sects;
        unsigned char *Screen::GetLineFromSection(int section, int x);
};
#endif
