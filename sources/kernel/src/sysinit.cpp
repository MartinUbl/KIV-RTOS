#include <hal/intdef.h>

// zacatek .bss sekce
extern "C" int _phys_bss_start;
// konec .bss sekce
extern "C" int _phys_bss_end;

extern "C" int __attribute__((section(".initsys"))) _c_startup(void)
{
	int* i;
	
	// vynulujeme .bss sekci
	for (i = (int*)_phys_bss_start; i < (int*)_phys_bss_end; i++)
		*i = 0;
	
	return 0;
}

extern "C" void __attribute__((section(".text"))) kernel_mode_start();

extern "C" void __attribute__((section(".text"))) _init_system_memory_high();

extern unsigned int _phys_data_start;

static unsigned int *initpagetable = (unsigned int * const)0x4000; /* 16K */
static unsigned int *kerneldatatable = (unsigned int * const)0x3c00; /* 1K */

extern "C" void __attribute__((section(".initsys"))) _init_system_memory()
{
    register unsigned int x;
	register unsigned int pt_addr;
	register unsigned int control;

    for (x = 0; x < 4096; x++)
	{
		if ((x >= (0x80000000 >> 20)) && (x < (0xa1000000 >> 20)))
			initpagetable[x] = (x - (0x80000000 >> 20)) << 20 | 0x0410 | 2; // R/W privilegovany
		else
			initpagetable[x] = 0;
	}

	initpagetable[0] = 0<<20 | 0x0400 | 2;      // mapovani 0x00000000-0x000FFFFF (fyz) na 0x00000000-0x000FFFFF (virt)
    initpagetable[3840] = 0<<20 | 0x8400 | 2;   // mapovani 0x00000000-0x000FFFFF (fyz) na 0xF0000000-0xF00FFFFF (virt)

	initpagetable[3072] = (unsigned int)kerneldatatable | 1; // mapovani 0x00012000-??? (fyz) zanorenou strankou

	for (x = 0; x < 256; x++)
	{
		if(x <= ((unsigned int)&_phys_bss_end >> 12))
			kerneldatatable[x] = ((unsigned int)&_phys_data_start + (x<<12)) | 0x0010 | 2;
		else
			kerneldatatable[x] = 0;
	}

    pt_addr = (unsigned int) initpagetable;

	asm volatile("mcr p15, 0, %[addr], c2, c0, 0" : : [addr] "r" (pt_addr));
	asm volatile("mcr p15, 0, %[addr], c2, c0, 1" : : [addr] "r" (pt_addr));
	asm volatile("mcr p15, 0, %[n], c2, c0, 2" : : [n] "r" (0));
	asm volatile("mcr p15, 0, %[r], c3, c0, 0" : : [r] "r" (0x1));
	asm volatile("mrc p15, 0, %[control], c1, c0, 0" : [control] "=r" (control));
	control |= 1;
	control |= (1<<23);
	asm volatile("mcr p15, 0, %[control], c1, c0, 0" : : [control] "r" (control));







    asm volatile("mov lr, %[_init_system_memory_high]" : : [_init_system_memory_high] "r" ((unsigned int)&_init_system_memory_high) );
	asm volatile("bx lr");
}

unsigned int pagetable0[64]	__attribute__ ((aligned (256)));

#define mem_p2v(X) (X+0x80000000)

static unsigned int *pagetable = (unsigned int * const) mem_p2v(0x4000); /* 16k */

unsigned int mem_v2p(unsigned int virtualaddr)
{
	unsigned int pt_data = pagetable[virtualaddr >> 20];
	unsigned int cpt_data, physaddr;

	if ((pt_data & 3) == 0 || (pt_data & 3) == 3)
		return 0xffffffff;

	if ((pt_data & 3) == 2)
	{
		physaddr = pt_data & 0xfff00000;

		if (pt_data & (1<<18))
			physaddr += virtualaddr & 0x00ffffff;
		else
			physaddr += virtualaddr & 0x000fffff;

		return physaddr;
	}

	cpt_data = ((unsigned int *)(0x80000000 + (pt_data & 0xfffffc00)))[(virtualaddr>>12)&0xff] ;

	if ((cpt_data & 3) == 0)
		return 0xffffffff;

	if (cpt_data & 2)
		return (cpt_data & 0xfffff000) + (virtualaddr & 0xfff);

	return (cpt_data & 0xffff0000) + (virtualaddr & 0xffff);
}

extern "C" void __attribute__((section(".text"))) _init_system_memory_high()
{
    unsigned int x;
	unsigned int pt0_addr;

	for (x = 0; x < 64; x++)
		pagetable0[x] = 0;

	pt0_addr = mem_v2p((unsigned int) &pagetable0);

	asm volatile("mcr p15, 0, %[n], c2, c0, 2" : : [n] "r" (6));
	asm volatile("mcr p15, 0, %[addr], c2, c0, 0" : : [addr] "r" (pt0_addr));
	asm volatile("mcr p15, 0, %[data], c8, c7, 0" : : [data] "r" (0));

    asm volatile("mov lr, %[kernel_mode_start]" : : [kernel_mode_start] "r" ((unsigned int)&kernel_mode_start) );
	asm volatile("bx lr");
}