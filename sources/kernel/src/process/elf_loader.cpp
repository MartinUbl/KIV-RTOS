#include <process/elf_loader.h>
#include <stdstring.h>

// delka identifikacniho pole ELF souboru
#define EI_NIDENT 16
// oznaceni loadable sekce
#define PT_LOAD   1

// hlavicka ELF souboru
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf32_Ehdr;

// hlavicka ELF sekce
typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;

uint32_t CELF_Loader::Load_ELF32_Image(const uint8_t* elf_data, uint8_t* memory, uint32_t memory_size) {
    const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(elf_data);

    // ELF magic - pokud neco z toho nesouhlasi, nejde o ELF soubor
    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' ||
        ehdr->e_ident[2] != 'L'  || ehdr->e_ident[3] != 'F') {
        return Invalid_Entry_Point;
    }

    // vyzadujeme pouze ELF32 (index 4 = 1) a little-endian (index 5 = 1)
    if (ehdr->e_ident[4] != 1 || ehdr->e_ident[5] != 1) {
        return Invalid_Entry_Point;
    }

    // nacteme hlavicky sekci (program headers)
    const Elf32_Phdr* phdrs = reinterpret_cast<const Elf32_Phdr*>(elf_data + ehdr->e_phoff);

    // a nacteme vsechny loadable sekce do pameti
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        const Elf32_Phdr* ph = &phdrs[i];
        if (ph->p_type != PT_LOAD) {
            continue;
        }

        if (ph->p_vaddr + ph->p_memsz > memory_size) {
            return Invalid_Entry_Point;
        }

        uint32_t copy_size = ph->p_filesz;
        if (ph->p_memsz > ph->p_filesz) {
            // pokud je velikost pameti vetsi nez velikost souboru, vyplnime zbytek nulami (viz specifikace ELF)
            copy_size = ph->p_memsz;
            memset(memory + ph->p_vaddr, 0, copy_size);
        }

        memcpy(elf_data + ph->p_offset, memory + ph->p_vaddr, copy_size);
    }

    return ehdr->e_entry;
}
