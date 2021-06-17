#include <drivers/i2c.h>

#include <drivers/gpio.h>

CI2C sI2C1(hal::BSC1_Base, 2, 3);

CI2C::CI2C(unsigned long base, uint32_t pin_sda, uint32_t pin_scl)
    : mBSC_Base(reinterpret_cast<volatile uint32_t*>(base)), mOpened(false), mSDA_Pin(pin_sda), mSCL_Pin(pin_scl)
{
    //
}

volatile uint32_t& CI2C::Reg(hal::BSC_Reg reg)
{
    return mBSC_Base[static_cast<uint32_t>(reg)];
}

void CI2C::Wait_Ready()
{
    volatile uint32_t& s = Reg(hal::BSC_Reg::Status);

    while( !(s & (1 << 1)) )
        ;
}

bool CI2C::Open()
{
    if (!sGPIO.Reserve_Pin(mSDA_Pin, true, true))
        return false;

    if (!sGPIO.Reserve_Pin(mSCL_Pin, true, true))
    {
        sGPIO.Free_Pin(mSDA_Pin, true, true);
        return false;
    }   

    sGPIO.Set_GPIO_Function(mSDA_Pin, NGPIO_Function::Alt_0);
    sGPIO.Set_GPIO_Function(mSCL_Pin, NGPIO_Function::Alt_0);

    mOpened = true;

    return true;
}

void CI2C::Close()
{
    Reg(hal::BSC_Reg::Control) = 0;

    sGPIO.Set_GPIO_Function(mSDA_Pin, NGPIO_Function::Input);
    sGPIO.Set_GPIO_Function(mSCL_Pin, NGPIO_Function::Input);

    sGPIO.Free_Pin(mSDA_Pin, true, true);
    sGPIO.Free_Pin(mSCL_Pin, true, true);

    mOpened = false;
}

bool CI2C::Is_Opened() const
{
    return mOpened;
}

void CI2C::Send(uint16_t addr, const char* buffer, uint32_t len)
{
    Reg(hal::BSC_Reg::Slave_Address) = addr;
    Reg(hal::BSC_Reg::Data_Length) = len;

    for (uint32_t i = 0; i < len; i++)
        Reg(hal::BSC_Reg::Data_FIFO) = buffer[i];

    Reg(hal::BSC_Reg::Status) = (1 << 9) | (1 << 8) | (1 << 1); // reset "slave clock hold", "slave fail" a "status" bitu
    Reg(hal::BSC_Reg::Control) = (1 << 15) | (1 << 7); // zapoceti noveho prenosu (enable bsc + start transfer)

    Wait_Ready();
}

void CI2C::Receive(uint16_t addr, char* buffer, uint32_t len)
{
    Reg(hal::BSC_Reg::Slave_Address) = addr;
    Reg(hal::BSC_Reg::Data_Length) = len;

    Reg(hal::BSC_Reg::Status) = (1 << 9) | (1 << 8) | (1 << 1); // reset "slave clock hold", "slave fail" a "status" bitu
    Reg(hal::BSC_Reg::Control) = (1 << 15) | (1 << 7) | (1 << 4) | (1 << 0); // zapoceti cteni (enable bsc + clear fifo + start transfer + read)

    Wait_Ready();

    for (uint32_t i = 0; i < len; i++)
        buffer[i] = Reg(hal::BSC_Reg::Data_FIFO);
}
