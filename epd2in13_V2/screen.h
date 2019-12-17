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
#define FEATURE_TOGGLE 0

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

const unsigned char lut_full_update[]= {
    0x80,0x60,0x40,0x00,0x00,0x00,0x00,             //LUT0: BB:     VS 0 ~7
    0x10,0x60,0x20,0x00,0x00,0x00,0x00,             //LUT1: BW:     VS 0 ~7
    0x80,0x60,0x40,0x00,0x00,0x00,0x00,             //LUT2: WB:     VS 0 ~7
    0x10,0x60,0x20,0x00,0x00,0x00,0x00,             //LUT3: WW:     VS 0 ~7
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT4: VCOM:   VS 0 ~7

    0x03,0x03,0x00,0x00,0x02,                       // TP0 A~D RP0
    0x09,0x09,0x00,0x00,0x02,                       // TP1 A~D RP1
    0x03,0x03,0x00,0x00,0x02,                       // TP2 A~D RP2
    0x00,0x00,0x00,0x00,0x00,                       // TP3 A~D RP3
    0x00,0x00,0x00,0x00,0x00,                       // TP4 A~D RP4
    0x00,0x00,0x00,0x00,0x00,                       // TP5 A~D RP5
    0x00,0x00,0x00,0x00,0x00,                       // TP6 A~D RP6

    0x15,0x41,0xA8,0x32,0x30,0x0A,
};

struct Section {
    sFONT *font;
    int cap;
    int width;
    int height;
};

class Screen : EpdIf {
    public:
        Screen(int sections);
        ~Screen();
        unsigned char *GetLine(int x);
        int DefineSection(int section, int lines, sFONT *font);
        void AddText(int section, char *txt);
        void Print(int section, char *txt, int align=ALIGN_LEFT);
        void Print();
        // Epd
        void Reset();
        void Clear();
        void Sleep();
        void Draw();
        int EpdInit();

    private:
        const uint8_t ***secPtrs;
        struct Section **secDescs;
        int sects;
        unsigned char *GetLineFromSection(int section, int x);
        // Epd 
        void SendCommand(unsigned char command);
        void SendData(unsigned char data);
        void WaitUntilIdle();
};
#endif
