#ifndef SCREEN_H
#define SCREEN_H

#define null 0;
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
        void AddSmallText(char *txt);
        void Print();

    private:
        const uint8_t **secPtrs;
        sFONT **secFonts;
        int *secBases;
        int sects;
        const uint8_t *small[20][15];
        sFONT *smallFont = &Font12;
};

#endif
