#include "screen.h"

#pragma region Init

Screen::Screen(int sectors) {
    sects = sectors;
    secDescs = (struct Section **)calloc(sects, sizeof(struct Section *));
    secPtrs = (const uint8_t ***)malloc(sects * sizeof(const uint8_t *));
}

Screen::~Screen() {
    for(int i = 0; i < sects; i++) {
        if(secPtrs[i] != nullptr) {
            free((void *)secPtrs[i]);
        }
        if(secDescs[i] != nullptr) {
            free(secDescs[i]);
        }
    }
    free(secPtrs);
    free(secDescs);
}

/* 
Configures a section of the screen, giving it a fixed number of lines
for the provided font size.
## sections must be defined in order ## 
*/
int Screen::DefineSection(int section, int lines, sFONT *font)
{
    if (section < sects && section >= 0)
    {
        secDescs[section] = (struct Section *)malloc(sizeof(struct Section));
        secDescs[section]->font = font;
        secDescs[section]->height = lines;
        secDescs[section]->cap = section == 0 ? font->Height * lines : font->Height * lines + secDescs[section - 1]->cap;
        secDescs[section]->width = EPD_WIDTH / font->Width;
        int charC = (secDescs[section]->width) * lines;
        secPtrs[section] = (const uint8_t **)malloc(charC * sizeof(void *));
        return 0;
    }
    return 1;
}

#pragma endregion

#pragma region Utils

inline unsigned char rev_byte(unsigned char c)
{
    unsigned char b = 0;
    for (int i = 0; i < 8; i++)
    {
        b = b << 1;
        b |= (c >> i) & 0x1;
    }
    return b;
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

#pragma endregion

#pragma region Input
/* print txt to the next line in the specified section. Performs any requested formatting
 -- txt should not include any unprintable characters except newline and null termination*/
void Screen::Print(int section, char *txt, int align=ALIGN_LEFT) {
    int w = secDescs[section]->width;
    int h = secDescs[section]->height;
    char *buffer = (char *)malloc((w * h * sizeof(char)) + 1);
    int start = 0; int end = 0;
    for(int line = 0; line < h; line++) {
        // get next line
        while(txt[end] != '\0' && txt[end] != '\n' && end - start != w)
            end++;
        if((end - start) == w || align == ALIGN_LEFT) {
            for (int i = 0; i < w; i++)
                buffer[line * w + i] = i < end-start ? txt[start + i] : ' ';
        } else if(align == ALIGN_RIGHT) {
            int bc = w - end + start; //begin characters
            for(int i = 0; i < w; i++)
                buffer[line*w + i] = i < bc ? ' ' : txt[start + i - bc];
        } else { // align = center
            int ws0 = (w - end + start) / 2; // whitespace 0
            int ws1 = ws0 + end - start;
            for(int i = 0; i < w; i++)
                buffer[line * w + i] = i < ws0 || i >= ws1 ? ' ' : txt[start + i - ws0];
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
    int w = secDescs[section]->width;
    int h = secDescs[section]->height;
    sFONT *font = secDescs[section]->font;
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

#pragma endregion

#pragma region Output

/*
Get a line from the indicated section; x is the line starting at base 0
*/
unsigned char *Screen::GetLineFromSection(int section, int x) {
    // screen is exactly 15.25 bytes wide but screen expects to receive LINEBYTES bytes
    unsigned char *line = (unsigned char *)calloc(LINEBYTES, 1);
    sFONT *font = secDescs[section]->font;
    uint8_t subln = x % font->Height;
    int ln = x / font->Height;

    if(ln >= secDescs[section]->height) {
        for(int i = 0; i < LINEBYTES; i++) 
            line[i] = 0xFF;
    } else {
        uint8_t bytes = (font->Width / 8) + ((font->Width % 8) != 0);
        const uint8_t **data = secPtrs[section];
        line[0] = 0xFF; // avoid the cutoff
        uint8_t wptr = 8;
        unsigned char *cbyte = (unsigned char *)calloc(bytes,1);
        for (uint8_t rptr = 0; rptr < secDescs[section]->width; rptr++)
        {
            const uint8_t *frame = data[ln * secDescs[section]->width + rptr];
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
        if(x < secDescs[s]->cap) {
            int oft = s == 0 ? x : x - secDescs[s-1]->cap;
            return GetLineFromSection(s, oft);
        }
    }
    unsigned char *blank = (unsigned char *)malloc(LINEBYTES);
    for(int i = 0; i < LINEBYTES; i++) {
        blank[i] = 0xFF;
    }
    return blank;
}
#pragma endregion

#pragma region EpdUtils

/**
 *  @brief: basic function for sending commands
 */
void Screen::SendCommand(unsigned char command)
{
    DigitalWrite(DC_PIN, LOW);
    SpiTransfer(command);
}

/**
 *  @brief: basic function for sending data
 */
void Screen::SendData(unsigned char data)
{
    DigitalWrite(DC_PIN, HIGH);
    SpiTransfer(data);
}

/**
 *  @brief: Wait until the BUSY_PIN goes HIGH
 */
void Screen::WaitUntilIdle(void)
{
    while (DigitalRead(BUSY_PIN) == 1)
    { //LOW: idle, HIGH: busy
        DelayMs(100);
    }
    DelayMs(200);
}

int Screen::EpdInit()
{
    /* this calls the peripheral hardware interface, see epdif */
    if (IfInit() != 0)
    {
        return -1;
    }

    Reset();

    int count;

    WaitUntilIdle();
    SendCommand(0x12); // soft reset
    WaitUntilIdle();

    SendCommand(0x74); //set analog block control
    SendData(0x54);
    SendCommand(0x7E); //set digital block control
    SendData(0x3B);

    SendCommand(0x01); //Driver output control
    SendData(0xF9);
    SendData(0x00);
    SendData(0x00);

    SendCommand(0x11); //data entry mode
    SendData(0x01);

    SendCommand(0x44); //set Ram-X address start/end position
    SendData(0x00);
    SendData(0x0F); //0x0C-->(15+1)*8=128

    SendCommand(0x45); //set Ram-Y address start/end position
    SendData(0xF9);    //0xF9-->(249+1)=250
    SendData(0x00);
    SendData(0x00);
    SendData(0x00);

    SendCommand(0x3C); //BorderWavefrom
    SendData(0x03);

    SendCommand(0x2C); //VCOM Voltage
    SendData(0x55);    //

    SendCommand(0x03);
    SendData(lut_full_update[70]);

    SendCommand(0x04); //
    SendData(lut_full_update[71]);
    SendData(lut_full_update[72]);
    SendData(lut_full_update[73]);

    SendCommand(0x3A); //Dummy Line
    SendData(lut_full_update[74]);
    SendCommand(0x3B); //Gate time
    SendData(lut_full_update[75]);

    SendCommand(0x32);
    for (count = 0; count < 70; count++)
    {
        SendData(lut_full_update[count]);
    }

    SendCommand(0x4E); // set RAM x address count to 0;
    SendData(0x00);
    SendCommand(0x4F); // set RAM y address count to 0X127;
    SendData(0xF9);
    SendData(0x00);
    WaitUntilIdle();

    return 0;
}

/**
 *  @brief: module reset.
 *          often used to awaken the module in deep sleep,
 *          see Epd::Sleep();
 */
void Screen::Reset(void)
{
    DigitalWrite(RST_PIN, HIGH);
    DelayMs(200);
    DigitalWrite(RST_PIN, LOW); //module reset
    DelayMs(10);
    DigitalWrite(RST_PIN, HIGH);
    DelayMs(200);
}

void Screen::Clear()
{
    int w, h;
    w = (EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8) : (EPD_WIDTH / 8 + 1);
    h = EPD_HEIGHT;
    SendCommand(0x24);
    for (int j = 0; j < h; j++)
    {
        for (int i = 0; i < w; i++)
        {
            SendData(0xFF);
        }
    }

    //DISPLAY REFRESH
    SendCommand(0x22);
    SendData(0xC7);
    SendCommand(0x20);
    WaitUntilIdle();
}

/**
 *  @brief: After this command is transmitted, the chip would enter the
 *          deep-sleep mode to save power.
 *          The deep sleep mode would return to standby by hardware reset.
 *          The only one parameter is a check code, the command would be
 *          executed if check code = 0xA5.
 *          You can use Epd::Init() to awaken
 */
void Screen::Sleep()
{
    SendCommand(0x22); //POWER OFF
    SendData(0xC3);
    SendCommand(0x20);

    SendCommand(0x10); //enter deep sleep
    SendData(0x01);
    DelayMs(200);

    DigitalWrite(RST_PIN, LOW);
}

void Screen::Draw()
{
    SendCommand(0x24);
    for (int line = 0; line < EPD_HEIGHT; line++)
    {
        unsigned char *l = GetLine(line);
        for (int h = 15; h >= 0; h--)
        {
            SendData(rev_byte(l[h]));
        }
        free(l);
    }

    //DISPLAY REFRESH
    SendCommand(0x22);
    SendData(0xC7);
    SendCommand(0x20);
    WaitUntilIdle();
}

#pragma endregion

#pragma region UnitTesting

#ifdef UNIT
void Screen::Print() {
    const uint8_t **data;
    for(int s = 0; s < sects; s++) {
        printf("W %d H %d C %d\n", secDescs[s]->width, secDescs[s]->height, secDescs[s]->cap);
        data = secPtrs[s];
        int w = secDescs[s]->width;
        int h = secDescs[s]->height;
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
    s.DefineSection(0, 1, &Font8);
    s.DefineSection(1, 4, &Font8);
    if(argc > 1) {
        s.Print(0, argv[1], ALIGN_RIGHT);
        s.Print(1, "\nonwards\n&\nupwards", ALIGN_CENTER);
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

#pragma endregion