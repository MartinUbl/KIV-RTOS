#include <drivers/trng.h>

CTRNG sTRNG(hal::TRNG_Base);

// tolik cisel ze zacatku generovani bude zahozeno - jsou "mene nahodna"
constexpr uint32_t TRNG_Warmup_Count = 0x40000;

CTRNG::CTRNG(uint32_t trng_reg_base)
    : mTrng_Regs(reinterpret_cast<volatile unsigned int* const>(trng_reg_base)), mOpened(0) {
    //
}

bool CTRNG::Open(bool exclusive) {
    // TODO: zamek

    if (mOpened && exclusive) {
        return false;
    }

    mOpened++;

    mTrng_Regs[static_cast<uint32_t>(hal::TRNG_Reg::Status)] = TRNG_Warmup_Count;
    mTrng_Regs[static_cast<uint32_t>(hal::TRNG_Reg::Int_Mask)] |= 1;    // vypneme preruseni pro TRNG (zatim ho pouzivat nechceme)
    mTrng_Regs[static_cast<uint32_t>(hal::TRNG_Reg::Control)] |= 1;     // zapneme TRNG

    // pockame, dokud TRNG nenageneruje warmup cisla (ty co zahazujeme)
    while (!(mTrng_Regs[static_cast<uint32_t>(hal::TRNG_Reg::Status)] >> 24)){
        // aktivni cekani
    }

    return true;
}

void CTRNG::Close() {
    if (!mOpened) {
        return;
    }

    mOpened--;

    if (mOpened == 0) {
        mTrng_Regs[static_cast<uint32_t>(hal::TRNG_Reg::Control)] = 0;
    }
}

bool CTRNG::Is_Opened() const {
    return (mOpened > 0);
}

uint32_t CTRNG::Get_Random_Number() {
    if (mOpened == 0) {
        return 4; // https://xkcd.com/221/
    }

    // TODO: tady by melo byt opet cekani na entropii + procesy by se mely blokovat, dokud TRNG nevygeneruje preruseni
    //while (!(mTrng_Regs[static_cast<uint32_t>(hal::TRNG_Reg::Status)] >> 24))
    //    ;

    return mTrng_Regs[static_cast<uint32_t>(hal::TRNG_Reg::Data)];
}
