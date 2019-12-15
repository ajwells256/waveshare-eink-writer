#include <SPI.h>
#include "epd2in13_V2.h"
#include "imagedata.h"
#include "fonts.h"
#include <stdio.h>

Epd epd;

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

    Serial.println("e-Paper clear");
    epd.Clear();

    epd.Display(IMAGE_DATA);
    Serial.println("e-Paper show pic");
    Serial.println("e-Paper clear and sleep");
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
        Screen s = Screen(2);
        s.DefineSection(0, 2, &Font12);
        s.DefineSection(1, 2, &Font8);
        s.AddText(0, build);
        s.AddText(1, build);
        epd.DisplayScreen(&s);
      }
      Serial.println("Message Received");
      rPtr++; // get past the trailing semicolon
    }
    wPtr++;
    if(wPtr == 64) {
      wPtr = 0;
    }
  }
}
