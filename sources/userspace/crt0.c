
extern unsigned int __bss_start;
extern unsigned int __bss_end;

void __crt0_init_bss()
{
    unsigned int* begin = (unsigned int*)__bss_start;
    for (; begin < (unsigned int*)__bss_end; begin++)
        *begin = 0;
}
