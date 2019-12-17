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

/* write the provided input into the destination (assume that the input is aligned left)  */
inline void writebuf(unsigned char *input, unsigned char *dst, uint8_t startBit, uint8_t lengthBits) {
    unsigned char byte = input[0];
    // write non-aligned
    uint8_t mask, rem, oft, index = startBit / 8;
    oft = startBit % 8;
    rem = 8-oft;
    mask = ((1 << rem) - 1); // 0^oft||1^(rem)
    // if whole thing fits in first byte
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
    for(i = 0; i < alignedBytes; i++) {\
        // mask in case of arithmetic shift
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

/* print txt to the next line in the specified section. Performs any requested formatting
 -- txt should not include any unprintable characters except newline and null termination*/
void Screen::Print(int section, char *txt, int align=ALIGN_LEFT) {
    int w = secWidth[section];
    int h = secHeight[section];
    char *buffer = (char *)malloc((w * h * sizeof(char)) + 1);
    int start = 0; int end = 0;
    for(int line = 0; line < h; line++) {
        // get next line
        while(txt[end] != '\0' && txt[end] != '\n' && end - start != w)
            end++;
        if((end - start) == w || align == ALIGN_LEFT) {
            for (int i = 0; i < w; i++)
                buffer[line * w + i] = i < end-start ? txt[start + i] : '_';
        } else if(align == ALIGN_RIGHT) {
            int bc = w - end + start; //begin characters
            for(int i = 0; i < w; i++)
                buffer[line*w + i] = i < bc ? '_' : txt[start + i - bc];
        } else { // align = center
            int ws0 = (w - end + start) / 2; // whitespace 0
            int ws1 = ws0 + end - start;
            for(int i = 0; i < w; i++)
                buffer[line * w + i] = i < ws0 || i >= ws1 ? '_' : txt[start + i - ws0];
        }
        if(txt[end] == '\n') {
            start = end+1;
            end = start;
        }
        else if(txt[end] == '\0') {
            buffer[(line+1) * w] = '\0';
            break;
        } else {
            start = end;
            end = start;
        }
    }

    AddText(section, buffer);
    free(buffer);
}

/* Write text to the specified section, overwriting any previous text*/
void Screen::AddText(int section, char *txt) {
    const uint8_t **secData = secPtrs[section];
    int w = secWidth[section];
    int h = secHeight[section];
    sFONT *font = secFonts[section];
    bool nullTerm = false;
    unsigned int char_offset;
    unsigned int factor = font->Height * (font->Width / 8 + (font->Width % 8 ? 1 : 0));
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
    // screen is exactly 15.25 bytes wide but screen expects to receive LINEBYTES bytes
    unsigned char *line = (unsigned char *)calloc(LINEBYTES, 1);
    sFONT *font = secFonts[section];
    uint8_t subln = x % font->Height;
    int ln = x / font->Height;

    if(ln >= secHeight[section]) {
        for(int i = 0; i < LINEBYTES; i++) 
            line[i] = 0xFF;
    } else {
        uint8_t bytes = (font->Width / 8) + ((font->Width % 8) != 0);
        const uint8_t **data = secPtrs[section];
        line[0] = 0xFF; // avoid the cutoff
        uint8_t wptr = 8;
        unsigned char *cbyte = (unsigned char *)calloc(bytes,1);
        for (uint8_t rptr = 0; rptr < secWidth[section]; rptr++)
        {
            const uint8_t *frame = data[ln * secWidth[section] + rptr];
#ifdef UNIT
            cbyte[0] = (unsigned char)frame;
#else
            for(uint8_t b = 0; b < bytes; b++) {
                cbyte[b] = frame == nullptr ? 0xFF : ~pgm_read_byte(frame + bytes*subln + b);
            }
#endif
            writebuf(cbyte, line, wptr, font->Width);
            wptr += font->Width;
        }
        if(wptr < LINEBITS) {
            for(ln = 0; ln < bytes; ln++)
                cbyte[ln] = 0xFF;
            writebuf(cbyte, line, wptr, LINEBITS-wptr);
        }
        free(cbyte);
    }
    return line;
}

/* get line x of the screen */
unsigned char * Screen::GetLine(int x) {
    for(int s = 0; s < sects; s++) {
        if(x < secCap[s]) {
            int oft = s == 0 ? x : x - secCap[s-1];
            return GetLineFromSection(s, oft);
        }
    }
    unsigned char *blank = (unsigned char *)malloc(LINEBYTES);
    for(int i = 0; i < LINEBYTES; i++) {
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
                printf("%c ", (char)data[i * w + j]);
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
    unsigned char *line = (unsigned char *)calloc(LINEBYTES,1);
    uint8_t *in = (uint8_t *)calloc(4,1);
    in[0] = 0x88;
    writebuf(in, line, 1, 5);
    in[0] = 0x84;
    in[1] = 0x42;
    in[2] = 0x21;
    in[3] = 0xFF;
    writebuf(in, line, LINEBYTES, 25);

    printf("0x");
    for(int i = 0; i < LINEBYTES; i++) {
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
    Screen s = Screen(2);
    s.DefineSection(0, 3, &Font8);
    s.DefineSection(1, 3, &Font8);
    if(argc > 1) {
        s.Print(0, argv[1], ALIGN_RIGHT);
        s.Print(1, "hello\nworld", ALIGN_RIGHT);
    }
    else {
        s.AddText(0, argv[0]);
    }
    s.Print();
    // printf("%d\n", EPD_WIDTH / 7);
    // partialwrite_test();
    // betterbitmap_test();
}

#endif
