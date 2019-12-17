#ifndef SCREEN_H
#define SCREEN_H

// Display resolution
#define EPD_WIDTH 122
#define EPD_HEIGHT 250

#define LINEBYTES 16
#define LINEBITS (LINEBYTES * 8)

#define ALIGN_LEFT 0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT 2

#include "fonts.h"

// #define UNIT 0
// #define FEATURE_TOGGLE 0

#ifdef UNIT
#include <stdio.h>
#else
#ifdef FEATURE_TOGGLE 
#include <SPI.h>
#include "epdif.h"
#endif
#include <avr/pgmspace.h>

#endif
#include <stdlib.h>

struct Section {
    sFONT *font;
    int cap;
    int width;
    int height;
};

class Screen {
    public:
        Screen(int sections);
        ~Screen();
        unsigned char *GetLine(int x);
        int DefineSection(int section, int lines, sFONT *font);
        void AddText(int section, char *txt);
        void Print(int section, char *txt, int align=ALIGN_LEFT);
        void Print();

    private:
        const uint8_t ***secPtrs;
        struct Section **secDescs;
        int sects;
        unsigned char *Screen::GetLineFromSection(int section, int x);
};
#endif
