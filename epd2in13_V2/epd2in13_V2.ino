#include <SPI.h>
#include "epd2in13_V2.h"
#include "imagedata.h"
#include "fonts.h"
#include <stdio.h>

Epd epd;
Screen *s;

int buff[64];
int rPtr = 0;
int wPtr = 0;

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(9600);
    if (epd.Init(FULL) != 0) {
        Serial.println("e-Paper init failed");
        return;
    }

    epd.Display(IMAGE_DATA);

    s = new Screen(5);
    s->DefineSection(0, 2, &Font8);
    s->DefineSection(1, 2, &Font12);
    s->DefineSection(2, 2, &Font16);
    s->DefineSection(3, 2, &Font20);
    s->DefineSection(4, 2, &Font24);
}

void loop()
{
  while(Serial.available() > 0) {
    buff[wPtr] = Serial.read();
    if(buff[wPtr] == (int)';' || buff[wPtr] == (int)'\n') {
      char build[64];
      int i;
      for(i = 0; i < 64; i++) {
        build[i] = '\0';
      }
      i = 0;
      while(wPtr != rPtr) {
        build[i] = (char)buff[rPtr];
        rPtr++;
        if(rPtr == 64) {
          rPtr = 0;
        }
        i++;
      }
      if(strcmp(build, "clear") == 0) {
        epd.Clear();
      } else if(strcmp(build, "draw") == 0) {
        epd.Display(IMAGE_DATA);
      } else if(strcmp(build, "die") == 0) {
        epd.Sleep();
      } else if(strcmp(build, "wake") == 0) {
        epd.Reset();
      } else {
        for(int i = 0; i < 5; i++) {
          s->AddText(i, build);
        }
        epd.DisplayScreen(s);
      }
      Serial.println("Message Received ");
      rPtr++; // get past the trailing semicolon
    }
    wPtr++;
    if(wPtr == 64) {
      wPtr = 0;
    }
  }
}
