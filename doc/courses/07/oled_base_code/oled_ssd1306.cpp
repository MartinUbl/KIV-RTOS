#include <drivers/oled_ssd1306.h>
#include <memory/kernel_heap.h>

CDisplay_SSD1306 sDisplay_SSD1306(sI2C1);

CDisplay_SSD1306::CDisplay_SSD1306(CI2C& i2c)
    : mI2C(i2c), mOpened(false), mBuffer(nullptr), mWidth(0), mHeight(0)
{

}

bool CDisplay_SSD1306::Open(int width, int height)
{
    if (!mI2C.Open())
        return false;

    mOpened = true;

    // zaokrouhlime nahoru na nasobek osmi (na cele stranky)
    if (height % 8 != 0)
        height += 8 - (height % 8);

    mWidth = width;
    mHeight = height;

    // alokujeme si buffer, ten je velky tak jako displej
    // dalo by se to optimalizovat, napr. primym kreslenim, bufferovanim vyrezu, apod.
    // ale to my ted nepotrebujeme, obzvlast kdyz spotrebujeme max jednotky kB a mame k dispozici >512MB RAM
    mBuffer = new uint8_t[mWidth * mHeight / 8];

    // vypneme displej, nastavime clock ratio (z datasheetu, doporucena hodnota), nastavime multiplex ratio (vysku displeje)
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << SSD1306_Cmd::Display_Off
            << SSD1306_Cmd::Set_Display_Clock_Div_Ratio
            << 0x80
            << SSD1306_Cmd::Set_Multiplex_Ratio;

        mI2C.End_Transaction(ta);
    }

    // pokracovani predchoziho - multiplex ratio
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << height - 1;

        mI2C.End_Transaction(ta);
    }

    // nastavime display offset (pametovy offset a jeho matching na realnou matici), pocatecni radek a vlastnosti vnitrniho menice (nabojova pumúa)
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << SSD1306_Cmd::Set_Display_Offset
            << 0x00
            << (static_cast<uint8_t>(SSD1306_Cmd::Set_Start_Line) | 0x00) // zacatek na radce 0
            << SSD1306_Cmd::Charge_Pump;

        mI2C.End_Transaction(ta);
    }

    // pokracovani predchoziho - nabojova pumpa, 0x14 je hodnota z datasheetu pro konkretni displej
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << 0x14;

        mI2C.End_Transaction(ta);
    }

    // nastavime mod adresace (0x00 - po sloupcich a po strankach) a segment remapping (to, jak je displej "obraceny")
    // "smer" skenovani vystupu ridiciho obvodu
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << SSD1306_Cmd::Memory_Addr_Mode
            << 0x00
            << (static_cast<uint8_t>(SSD1306_Cmd::Set_Segment_Remap) | 0x01)
            << SSD1306_Cmd::Com_Scan_Dir_Dec;

        mI2C.End_Transaction(ta);
    }

    // mapovani na piny ridiciho obvodu
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << SSD1306_Cmd::Set_Com_Pins;

        mI2C.End_Transaction(ta);
    }

    // pokracovani predchoziho - neinvertovany a sekvencni pristup
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << 0x02;

        mI2C.End_Transaction(ta);
    }

    // nastaveni kontrastu displeje
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << SSD1306_Cmd::Set_Contrast;

        mI2C.End_Transaction(ta);
    }

    // pokracovani predchoziho - kontrast nastaven na 0x8F
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << 0x8F;

        mI2C.End_Transaction(ta);
    }

    // perioda prednabiti
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << SSD1306_Cmd::Set_Precharge_Period;

        mI2C.End_Transaction(ta);
    }

    // pokracovani predchoziho - pro externi napajeni muze byt tato hodnota kratsi
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << 0xF1;

        mI2C.End_Transaction(ta);
    }

    // finalni aktivace displeje - uroven detekce vstupu, nahozeni panelu, neinvertovane barvy, neskrolujeme, zapneme podsviceni
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << SSD1306_Cmd::Set_VCOM_Detect
            << 0x40
            << SSD1306_Cmd::Display_All_On_Resume
            << SSD1306_Cmd::Normal_Display
            << SSD1306_Cmd::Deactivate_Scroll
            << SSD1306_Cmd::Display_On;

        mI2C.End_Transaction(ta);
    }

    Clear();

    return true;
}

void CDisplay_SSD1306::Send_Command(SSD1306_Cmd cmd, uint8_t lowPart)
{
    auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

    ta << SSD1306_Cmd::Command_Start
       << (static_cast<uint8_t>(cmd) | lowPart);

    mI2C.End_Transaction(ta);
}

void CDisplay_SSD1306::Close()
{
    if (!mOpened)
        return;

    // posleme prikaz z vypnuti displeje
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << SSD1306_Cmd::Display_Off;

        mI2C.End_Transaction(ta);
    }

    mI2C.Close();

    delete mBuffer;

    mOpened = false;
}

bool CDisplay_SSD1306::Is_Opened() const
{
    return mOpened;
}

void CDisplay_SSD1306::Clear(bool clearWhite)
{
    if (!mOpened)
        return;

    const uint8_t clearColor = clearWhite ? 0xFF : 0x00;

    const int maxIdx = mWidth * (mHeight / 8);

    for (int i = 0; i < maxIdx; i++)
        mBuffer[i] = clearColor;

    Flip();
}

void CDisplay_SSD1306::Set_Pixel(uint32_t x, uint32_t y, bool set)
{
    if (!mOpened)
        return;

    if (set)
        mBuffer[x + (y / 8) * mWidth] |= (1 << (y & 7));
    else
        mBuffer[x + (y / 8) * mWidth] &= ~(1 << (y & 7));
}

void CDisplay_SSD1306::Flip()
{
    if (!mOpened)
        return;

    int i;

    // nastavime kurzor na levy horni roh
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Command_Start
            << SSD1306_Cmd::Set_Page_Addr
            << 0x00
            << 0xFF
            << SSD1306_Cmd::Set_Column_Addr
            << 0x00
            << mWidth - 1;

        mI2C.End_Transaction(ta);
    }

    // budeme posilat pixely po balikach 4 sloupcu (po 8 pixelech)
    constexpr int PktSize = 4;

    const int maxIdx = mWidth * (mHeight / 8);

    for (int i = 0; i < maxIdx; i += PktSize)
    {
        auto& ta = mI2C.Begin_Transaction(SSD1306_Slave_Address);

        ta << SSD1306_Cmd::Data_Continue;
        for (int j = 0; j < PktSize; j++)
            ta << mBuffer[i + j];

        mI2C.End_Transaction(ta);
    }
}
