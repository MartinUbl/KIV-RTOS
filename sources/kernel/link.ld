ENTRY(_start)

MEMORY
{
    initsys_base : ORIGIN = 0x8000, LENGTH = 0x10000
	kernel : ORIGIN = 0xF0000000, LENGTH = 0x20000
	kerneldata : ORIGIN = 0xC0000000, LENGTH = 0x20000
}

SECTIONS
{
	.initsys :
	{
		*(.initsys*)
	} > initsys_base

	.initsys.data :
	{
		*(.initsys.data*)
	} > initsys_base

	__kernel_high_code = ALIGN(4k);

    .text (__kernel_high_code + 0xF0000000) : AT(__kernel_high_code)
	{
		__kernel_code_memory_start = ABSOLUTE(.);
		*(.text*)
	} > kernel
	.rodata : {
		*(.rodata*)
	} > kernel

	__kernel_code_memory_end = ABSOLUTE(.);
	__kernel_high_data = ALIGN(4k);
	
	.data : AT(__kernel_high_data - 0xF0000000)
    {
		_data_start = ABSOLUTE(.);

		__CTOR_LIST__ = .; *(.ctors) *(.init_array) __CTOR_END__ = .; 
		__DTOR_LIST__ = .; *(.dtors) *(.fini_array) __DTOR_END__ = .;
        data = .;
		_data = .;
		__data = .;
        *(.data)
    } > kerneldata
	
	_bss_start = .;
	
    .bss :
	{
		*(.bss*)
		*(COMMON)
	} > kerneldata
	
	_bss_end = .;



	_phys_data_start = __kernel_high_data - 0xF0000000;
	_phys_bss_start = _phys_data_start + (_bss_start - _data_start);
	_phys_bss_end = _phys_data_start + (_bss_end - _data_start);
}

