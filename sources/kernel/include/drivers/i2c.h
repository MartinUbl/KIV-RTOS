#pragma once

#include <hal/peripherals.h>

class CI2C
{
    private:
        volatile uint32_t* const mBSC_Base;
        bool mOpened;

        uint32_t mSDA_Pin;
        uint32_t mSCL_Pin;

    protected:
        volatile uint32_t& Reg(hal::BSC_Reg reg);

        void Wait_Ready();

    public:
        CI2C(unsigned long base, uint32_t pin_sda, uint32_t pin_scl);

        bool Open();
        void Close();

        bool Is_Opened() const;

        void Send(uint16_t addr, const char* buffer, uint32_t len);
        void Receive(uint16_t addr, char* buffer, uint32_t len);
};

// TODO: I2C0 a 2
extern CI2C sI2C1;
