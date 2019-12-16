
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

/* write the provided input into the destination (assume that the input is aligned left) 
 input should also have one full byte of extra space */
inline void writebuf(unsigned char *input, unsigned char *dst, uint8_t startBit, uint8_t lengthBits) {
    unsigned char byte = input[0];
    // write non-aligned
    uint8_t mask, rem, oft, index = startBit / 8;
    oft = startBit % 8;
    rem = 8-oft;
    mask = ((1 << rem) - 1); // 0^oft||1^(rem)
    if(lengthBits+oft < 8) {
        rem = lengthBits;
        mask = mask << 8-lengthBits-oft;
    }
    byte = (byte >> oft) & mask;
    dst[index] |= byte;
    index++;
    // write aligned
    uint8_t alignedBytes = (lengthBits - rem)/8;
    int i;
    for(i = 0; i < alignedBytes; i++) {
        byte = (input[i] << rem) | ((input[i+1] >> oft) & mask);
        dst[index + i] = byte;
    }
    // write non-aligned
    uint8_t lastSize = lengthBits - (8 * alignedBytes) - rem;
    mask = ~((1 << (8 - lastSize)) - 1);
    if(lastSize != 0) {
        byte = ((input[i] << rem) | (input[i+1] >> oft)) & mask;
        dst[index + alignedBytes] |= byte;
    }
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
    char used = 0;
    unsigned char rPtr = 0;
    for (int i = 0; i < 15; i++)
    {
        if (ln >= secHeight[section])
        {
            line[i + 1] = 0xFF;
        } else {
            const uint8_t *frame = data[ln * secWidth[section] + rPtr];
#ifdef UNIT
            unsigned char cbyte = (char)frame;
#else
            unsigned char cbyte = frame == nullptr ? 0xFF : ~pgm_read_byte(&frame[subln]);
#endif
            if(used == 0) {
                line[i + 1] = cbyte;
            } else if(used + font->Width <= 8) {

            } else {
                unsigned char rem = 8 - used;
                unsigned char mask = ~((1 << font->Width) - 1);
                unsigned char lowmask = font->Width > rem ?
                    mask >> font->Width - rem :
                    mask << rem - font->Width;
                line[i] |= (unsigned char)((cbyte >> used) | lowmask);
                line[i + 1] = (unsigned char)(cbyte << rem);
            }
            used = (used + font->Width) % 8;
            i -= used == 0;
            rPtr++; // increment the read pointer even if we redo the write
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

void betterbitmap_test() {
    unsigned char *line = (unsigned char *)calloc(16,1);
    uint8_t *in = (uint8_t *)calloc(4,1);
    in[0] = 0x88;
    writebuf(in, line, 1, 5);
    in[0] = 0x84;
    in[1] = 0x42;
    in[2] = 0x21;
    in[3] = 0xFF;
    writebuf(in, line, 16, 25);

    printf("0x");
    for(int i = 0; i < 16; i++) {
        printf("%x", line[i]);
    }
    printf("\n");
    free(line);
    free(in);
}

int main(int argc, char* argv[]) {
    sFONT Font8 = {
        nullptr,
        5,  /* Width */
        8, /* Height */
    };
    Screen s = Screen(1);
    s.DefineSection(0, 20, &Font8);
    if(argc > 1) {
        s.AddText(0, argv[1]);
    } else {
        s.AddText(0, argv[0]);
    }
    // s.Print();
    // printf("%d\n", EPD_WIDTH / 7);
    // partialwrite_test();
    betterbitmap_test();
}

#endif
