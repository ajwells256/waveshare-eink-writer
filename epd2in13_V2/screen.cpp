
#include "screen.h"

Screen::Screen(int sectors) {
    sects = sectors;
    secCap = (int *)malloc(sects * sizeof(int));
    secWidth = (int *)malloc(sects * sizeof(int));
    secHeight = (int *)malloc(sects * sizeof(int));
    secFonts = (sFONT **)malloc(sects * sizeof(sFONT *));
    secPtrs = (const uint8_t ***)malloc(sects * sizeof(const uint8_t *));
}

Screen::~Screen() {
    for(int i = 0; i < sects; i++) {
        if(&secPtrs[i] != nullptr) {
            free((void *)secPtrs[i]);
        }
    }
    free(secCap);
    free(secWidth);
    free(secHeight);
    free(secPtrs);
    free(secFonts);
}

/* 
Configures a section of the screen, giving it a fixed number of lines
for the provided font size.
## sections must be defined in order ## 
*/
int Screen::DefineSection(int section, int lines, sFONT *font) {
    if(section < sects && section >= 0) {
        secFonts[section] = font;
        secHeight[section] = lines;
        secCap[section] = section == 0 ? 
            font->Height * lines : 
            font->Height * lines + secCap[section - 1];
        secWidth[section] = EPD_WIDTH / font->Width;
        int charC = (secWidth[section])*lines;
        secPtrs[section] = (const uint8_t **)malloc(charC * sizeof(void *));
        return 0;
    }
    return 1;
}

void Screen::AddText(int section, char *txt) {
    const uint8_t **secData = secPtrs[section];
    int w = secWidth[section];
    int h = secHeight[section];
    sFONT *font = secFonts[section];
    bool nullTerm = false;
    unsigned int char_offset;
#ifdef UNIT
    unsigned int factor = 12 * (7 / 8 + (7 % 8 ? 1 : 0));
#else
    unsigned int factor = font->Height * (font->Width / 8 + (font->Width % 8 ? 1 : 0));
#endif
    for(int i = 0; i < h; i++) {
        for(int j = 0; j < w; j++) {
            int index = i * w + j;
            if(nullTerm) {
                secData[index] = nullptr;
                continue;
            }
            char c = txt[i * w + j];
            if (c == '\0') {
                nullTerm = true;
                secData[index] = nullptr;
                continue;
            } else {
                char_offset = (c - ' ') * factor;
#ifdef UNIT
                secData[index] = (const uint8_t *)c;
#else
                secData[index] = &font->table[char_offset];
#endif
            }
        }
    }
}

/*
Get a line from the indicated section; x is the line starting at base 0
*/
unsigned char *Screen::GetLineFromSection(int section, int x) {
    // screen is exactly 15.25 bytes wide but screen expects to receive 16 bytes
    unsigned char *line = (unsigned char *)calloc(16, 1);
    sFONT *font = secFonts[section];
    int ln = x / font->Height;
    int subln = x % font->Height;

    const uint8_t **data = secPtrs[section];
    line[0] = 0xFF;
    for (int i = 0; i < 15; i++)
    {
        if (ln >= secHeight[section])
        {
            line[i + 1] = 0xFF;
        } else {
            const uint8_t *frame = data[ln * secWidth[section] + i];
#ifdef UNIT
            unsigned char cbyte = (char)frame;
#else
            unsigned char cbyte = frame == nullptr ? 0xFF : ~pgm_read_byte(&frame[subln]);
#endif
            line[i + 1] = cbyte;
        }
    }
    return line;
}

unsigned char * Screen::GetLine(int x) {
    for(int s = 0; s < sects; s++) {
        if(x < secCap[s]) {
            int oft = s == 0 ? x : x - secCap[s-1];
            return GetLineFromSection(s, oft);
        }
    }
    unsigned char *blank = (unsigned char *)malloc(16);
    for(int i = 0; i < 16; i++) {
        blank[i] = 0xFF;
    }
    return blank;
}
#ifdef UNIT
void Screen::Print() {
    const uint8_t **data;
    for(int s = 0; s < sects; s++) {
        printf("W %d H %d C %d\n", secWidth[s], secHeight[s], secCap[s]);
        data = secPtrs[s];
        int w = secWidth[s];
        int h = secHeight[s];
        for(int i = 0; i < h; i++) {
            for(int j = 0; j < w; j++) {
                printf("%p ", data[i * w + j]);
            }
            printf("\n");
        }
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
    sFONT Font12 = {
        nullptr,
        7,  /* Width */
        12, /* Height */
    };
    Screen s = Screen(1);
    s.DefineSection(0, 20, &Font12);
    s.AddText(0, argv[1]);
    s.Print();
    printf("%d\n", EPD_WIDTH / 7);
}

#endif
