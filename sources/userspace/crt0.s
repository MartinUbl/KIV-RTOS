.section .text

.global __crt0_init_bss

.global _start

_start:
    bl __crt0_init_bss

    bl _cpp_startup

    ;@ v beznem OS jsou parametry predany napr. prostrednictvim zasobniku
    ;@ my tady vlastne parametry ani nepotrebujeme, ponechme je zde jen pro jakousi konformitu s tim co zname
    mov r0, #0      ;@ argc
    mov r1, #0      ;@ argv

    bl main

    bl _cpp_shutdown

    ;@ TODO: zavolat terminate syscall

_hang:
    b _hang
