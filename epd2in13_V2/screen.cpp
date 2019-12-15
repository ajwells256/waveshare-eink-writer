
#include "screen.h"

Screen::Screen(int sectors) {
    sects = sectors;
    secBases = (int *)malloc(sects * sizeof(int));
    secFonts = (sFONT **)malloc(sects * sizeof(sFONT *));
    secPtrs = (const uint8_t **)malloc(sects * sizeof(const uint8_t *));
}

Screen::~Screen() {
    for(int i = 0; i < sects; i++) {
        if(&secPtrs[i] != nullptr) {
            free((void *)secPtrs[i]);
        }
    }
    free(secBases);
    free(secPtrs);
    free(secFonts);
}

void Screen::AddSmallText(char *txt) {
    AddText(0, txt);
}

void Screen::AddText(int section, char *txt) {
    const uint8_t *secData = secPtrs[section];
    bool nullTerm = false;
    unsigned int char_offset;
#ifdef UNIT
    unsigned int factor = 12 * (7 / 8 + (7 % 8 ? 1 : 0));
#else
    unsigned int factor = smallFont->Height * (smallFont->Width / 8 + (smallFont->Width % 8 ? 1 : 0));
#endif
    for(int i = 0; i < 20; i++) {
        for(int j = 0; j < 15; j++) {
            if(nullTerm) {
                small[i][j] = nullptr;
                continue;
            }
            char c = txt[i * 15 + j];
            if (c == '\0') {
                nullTerm = true;
                small[i][j] = nullptr;
                continue;
            } else {
                char_offset = (c - ' ') * factor;
#ifdef UNIT
                small[i][j] = (const uint8_t *)c;
#else
                small[i][j] = &smallFont->table[char_offset];
#endif
            }
        }
    }
}

unsigned char * Screen::GetLine(int x) {
    unsigned char *line = (unsigned char*)calloc(16, 1);
#ifndef UNIT
    int ln = x / smallFont->Height;
    int subln = x % smallFont->Height;
#else
    int ln = x / 12;
    int subln = x % 12;
#endif
    line[0] = 0xFF;
    for (int i = 0; i < 15; i++) {
        if(ln >= 20) {
            line[i+1] = 0xFF;
        } else {
            const uint8_t *frame = small[ln][i];
#ifdef UNIT
            unsigned char cbyte = (char)frame;
#else
            unsigned char cbyte = frame == nullptr ? 0xFF : ~pgm_read_byte(&frame[subln]);
#endif
            line[i+1] = cbyte;
        }
    }
    return line;
}
#ifdef UNIT
void Screen::Print() {
    for(int i = 0; i < 20; i++) {
        for(int j = 0; j < 15; j++) {
            printf("%p ", small[i][j]);
        }
        printf("\n");
    }
}
#endif

#ifdef UNIT
void printarray_test(Screen *s)
{
    s->Print();
}

void getline_test(Screen *s)
{
    unsigned char *ln = s->GetLine(0);
    for (int h = 0; h < 15; h++)
    {
        printf("%c ", ln[h]);
    }
    printf("\n");
    free(ln);
}
int main(int argc, char* argv[]) {
    Screen s = Screen(nullptr);
    s.AddSmallText(argv[1]);
    s.Print();
}

#endif