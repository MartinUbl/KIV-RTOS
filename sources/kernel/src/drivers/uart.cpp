#include <drivers/uart.h>
#include <drivers/bcm_aux.h>
#include <drivers/gpio.h>

#include <stdstring.h>

#include "interrupt_controller.h"

CUART sUART0(sAUX);

CUART::CUART(CAUX& aux)
    : mAUX(aux), mOpened(false)
{
    spinlock_init(&mOpenLock);
    spinlock_init(&mRx_Lock);
}

bool CUART::Open()
{
    // zamek, kdyby se nahodou dva procesy pokouseli otevrit UART
    spinlock_lock(&mOpenLock);

    if (mOpened) {
        spinlock_unlock(&mOpenLock);
        return false;
    }

    // rezervujeme si TX a RX piny, exkluzivne pro nas (R i W, ackoliv je jeden jen vstupni a jeden jen vystupni)
    if (!sGPIO.Reserve_Pin(14, true, true)) {
        spinlock_unlock(&mOpenLock);
        return false;
    }

    if (!sGPIO.Reserve_Pin(15, true, true))
    {
        sGPIO.Free_Pin(14, true, true);
        spinlock_unlock(&mOpenLock);
        return false;
    }

    mAUX.Enable(hal::AUX_Peripherals::MiniUART);
    mAUX.Set_Register(hal::AUX_Reg::MU_IIR, 0);
    mAUX.Set_Register(hal::AUX_Reg::MU_IER, 1);
    mAUX.Set_Register(hal::AUX_Reg::MU_MCR, 0);
    mAUX.Set_Register(hal::AUX_Reg::MU_CNTL, 3); // RX and TX enabled

    // nastavime GPIO 14 a 15 na jejich alt funkci 5, coz je UART kanal 1
    sGPIO.Set_GPIO_Function(14, NGPIO_Function::Alt_5);
    sGPIO.Set_GPIO_Function(15, NGPIO_Function::Alt_5);

    // povolime preruseni UARTu
    sInterruptCtl.Enable_IRQ(hal::IRQ_Source::UART);

    mOpened = true;

    // nastavime vychozi rychlost a velikost znaku
    Set_Char_Length(NUART_Char_Length::Char_8);
    Set_Baud_Rate(NUART_Baud_Rate::BR_9600);
    Set_Blocking_Read(NUART_Blocking_Read::BLOCKING);

    spinlock_unlock(&mOpenLock);

    return true;
}

void CUART::Close()
{
    if (!mOpened)
        return;

    // zakazeme AUX periferii a preruseni
    sInterruptCtl.Disable_IRQ(hal::IRQ_Source::UART);
    mAUX.Disable(hal::AUX_Peripherals::MiniUART);
    mAUX.Set_Register(hal::AUX_Reg::MU_IER, 0);

    // piny 14 a 15 prepneme na Input (tak zerou nejmin proudu)
    sGPIO.Set_GPIO_Function(14, NGPIO_Function::Input);
    sGPIO.Set_GPIO_Function(15, NGPIO_Function::Input);

    // uvolnime piny
    sGPIO.Free_Pin(14, true, true);
    sGPIO.Free_Pin(15, true, true);

    mOpened = false;
}

bool CUART::Is_Opened() const
{
    return mOpened;
}

NUART_Char_Length CUART::Get_Char_Length()
{
    if (!mOpened)
        return NUART_Char_Length::Char_8;

    return static_cast<NUART_Char_Length>(mAUX.Get_Register(hal::AUX_Reg::MU_LCR) & 0x1);
}

void CUART::Set_Char_Length(NUART_Char_Length len)
{
    if (!mOpened)
        return;

    mAUX.Set_Register(hal::AUX_Reg::MU_LCR, (mAUX.Get_Register(hal::AUX_Reg::MU_LCR) & 0xFFFFFFFE) | static_cast<unsigned int>(len));
}

NUART_Blocking_Read CUART::Get_Blocking_Read()
{
    return mBlocking_Read;
}

void CUART::Set_Blocking_Read(NUART_Blocking_Read r)
{
    mBlocking_Read = r;
}

NUART_Baud_Rate CUART::Get_Baud_Rate()
{
    if (!mOpened)
        return NUART_Baud_Rate::BR_1200;

    return mBaud_Rate;
}

void CUART::Set_Baud_Rate(NUART_Baud_Rate rate)
{
    if (!mOpened)
        return;

    mBaud_Rate = rate;

    const unsigned int val = ((hal::Default_Clock_Rate / static_cast<unsigned int>(rate)) / 8) - 1;

    mAUX.Set_Register(hal::AUX_Reg::MU_CNTL, 0);

    mAUX.Set_Register(hal::AUX_Reg::MU_BAUD, val);

    mAUX.Set_Register(hal::AUX_Reg::MU_CNTL, 3);
}

void CUART::Write(char c)
{
    if (!mOpened)
        return;

    // dokud ma status registr priznak "vystupni fronta plna", nelze prenaset dalsi bit
    while (!(mAUX.Get_Register(hal::AUX_Reg::MU_LSR) & (1 << 5)))
        ;

    mAUX.Set_Register(hal::AUX_Reg::MU_IO, c);
}

void CUART::Write(const char* str)
{
    if (!mOpened)
        return;

    int i;

    for (i = 0; str[i] != '\0'; i++)
        Write(str[i]);
}

void CUART::Write(const char* str, unsigned int len)
{
    if (!mOpened)
        return;

    unsigned int i;

    for (i = 0; i < len; i++)
        Write(str[i]);
}

void CUART::Write(unsigned int num)
{
    if (!mOpened)
        return;

    static char buf[16];

    itoa(num, buf, 10);
    Write(buf);
}

void CUART::Write_Hex(unsigned int num)
{
    if (!mOpened)
        return;

    static char buf[16];

    itoa(num, buf, 16);
    Write(buf);
}

void CUART::IRQ_Callback()
{
    // 0 == pending interrupt
    if (mAUX.Get_Register(hal::AUX_Reg::MU_IIR) & 0b1)
        return;

    spinlock_lock(&mRx_Lock);

    while (mAUX.Get_Register(hal::AUX_Reg::MU_LSR) & 0x01)   // RX data ready
    {
        // Precteni znaku
        const auto c = static_cast<char>(mAUX.Get_Register(hal::AUX_Reg::MU_IO) & 0xFF);

        uint32_t next = (mRx_head + 1) % CUART_BUF_SIZE;

        if (next == mRx_tail)
        {
            // Buffer je plny -> zahodi se nejstarsi byte
            mRx_tail = (mRx_tail + 1) % CUART_BUF_SIZE;
        }

        mRx_Buf[mRx_head] = c;
        mRx_head = next;
    }

    // Pokud je cekajici soubor a buffer neni prazdny (head a tail jsou ruzne hodnoty), probudime proces
    if (mWaiting_File && mRx_head != mRx_tail) {
        mWaiting_File->Notify(1);
        mWaiting_File = nullptr;
    }

    spinlock_unlock(&mRx_Lock);
}

int CUART::ReadOrWait(char *str, unsigned int len, IFile* file)
{
    spinlock_lock(&mRx_Lock);

    while (mBlocking_Read == NUART_Blocking_Read::BLOCKING && mRx_head == mRx_tail) {
        if (!file) {
            spinlock_unlock(&mRx_Lock);
            return 0;
        }

        spinlock_unlock(&mRx_Lock);
        // Wait nad souborem pred zablokovanim zvola Wait_For_Event
        file->Wait(1);
        spinlock_lock(&mRx_Lock);
    }

    // zjisteni delky zpravy cekajici v bufferu
    int msg_len = (mRx_head + CUART_BUF_SIZE - mRx_tail) % CUART_BUF_SIZE;
    if (msg_len > len) {
        // omezeni cteni na velikost userspace bufferu
        msg_len = len;
    }

    // zkopirovani znaku do userspace bufferu
    for (uint32_t i = 0; i < msg_len; i++) {
        str[i] = mRx_Buf[(mRx_tail + i) % CUART_BUF_SIZE];
    }

    // posunuti ukazatele pro cteni z bufferu
    mRx_tail = (mRx_tail + msg_len) % CUART_BUF_SIZE;

    spinlock_unlock(&mRx_Lock);

    return msg_len;
}

// implementace Wait() nad souborem
void CUART::Wait_For_Event(IFile* file) {
    spinlock_lock(&mRx_Lock);

    mWaiting_File = file;

    spinlock_unlock(&mRx_Lock);
}
