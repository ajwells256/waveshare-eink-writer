#include <SPI.h>
#include "screen.h"
#include "fonts.h"
#include <stdio.h>

Screen *s;

int buff[64];
int rPtr = 0;
int wPtr = 0;

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(9600);
    if (s->EpdInit() != 0) {
        Serial.println("e-Paper init failed");
        return;
    }
    
    s = new Screen(1);
    s->DefineSection(0, 15, &Font12);
    s->Print(0, "\n\n\n\n\n\n\n\n\nLoading...", ALIGN_CENTER);
    s->Draw();
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
        s->Clear();
      } else if(strcmp(build, "die") == 0) {
        s->Sleep();
      } else if(strcmp(build, "wake") == 0) {
        s->Reset();
      } else {
        for(int i = 0; i < 64; i++) {
          if(build[i] == '\0')
            break;
          else if(build[i] == '\\')
            build[i] = '\n';
        }
        for(int i = 0; i < 1; i++) {
          s->Print(i, build, ALIGN_CENTER);
        }
        s->Draw();
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
