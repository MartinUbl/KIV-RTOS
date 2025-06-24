#pragma once

#include <hal/intdef.h>

// jednoduchy ELF loader - jedine, co udela, je ze nacte vsechny sekce (oznacene jako loadable) do pameti, kam maji byt nacteny
class CELF_Loader
{
    public:
        // rozparsuje ELF obraz a nacte vsechny sekce do pameti, kam maji byt nacteny
        // vraci vstupni bod programu
        static uint32_t Load_ELF32_Image(const uint8_t* elf_data, uint8_t* memory, uint32_t memory_size);

        // pokud se nepodari nacteni, vraci Invalid_Entry_Point
        constexpr static uint32_t Invalid_Entry_Point = 0xFFFFFFFF;
};