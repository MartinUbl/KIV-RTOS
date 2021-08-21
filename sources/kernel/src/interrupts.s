.equ    CPSR_MODE_FIQ,          0x11
.equ    CPSR_MODE_IRQ,          0x12
.equ    CPSR_MODE_SVR,          0x13
.equ    CPSR_MODE_SYS,          0x1F
.equ    CPSR_IRQ_INHIBIT,       0x80
.equ    CPSR_FIQ_INHIBIT,       0x40

.global undefined_instruction_handler
.global software_interrupt_handler
.global irq_handler
.global prefetch_abort_handler
.global data_abort_handler


;@ tady budou ostatni symboly, ktere nevyzaduji zadne specialni misto
.section .text

hang:
    b hang

.global enable_irq
enable_irq:
	;@ kompletni sekvence instrukci:
    mrs r0, cpsr		;@ presun ridiciho registru (CPSR) do general purpose registru (R0)
    bic r0, r0, #0x80	;@ vypne bit 7 v registru r0 ("IRQ mask bit")
    msr cpsr_c, r0		;@ nacteme upraveny general purpose (R0) registr do ridiciho (CPSR)

	;@ nebo lze udelat jen tohle:
    cpsie i				;@ povoli preruseni

    bx lr

undefined_instruction_handler:
	b hang

.global _internal_software_interrupt_handler
software_interrupt_handler:
	stmfd sp!,{r2-r12,lr}		;@ ulozime na zasobnik stav

	;@ tady budeme mozna chtit prepinat do SYS rezimu v budoucnu

	ldr r3,[lr,#-4]				;@ do registru r3 nacteme instrukci, ktera vyvolala preruseni (lr = navratova adresa, -4 proto, ze ukazuje na nasledujici instrukci)
    bic r3,r3,#0xff000000		;@ vymaskujeme parametr (dolnich 24 bitu) a nechame ho v r3
	bl _internal_software_interrupt_handler		;@ zavolame nas interni handler
	mov r2, r0					;@ ten vraci pointer na result kontejner v r0, presuneme do r2 - potrebujeme obsah dostat do r0 a r1
	ldr r0, [r2, #0]			;@ nacteme navratove hodnoty do registru
	ldr r1, [r2, #4]

	ldmfd sp!, {r2-r12,pc}^		;@ obnovime ze zasobniku stav (jen puvodni lr nacteme do pc)


.global _internal_irq_handler
irq_handler:
	sub lr, lr, #4

	srsdb #CPSR_MODE_SYS!		;@ ekvivalent k push lr a push spsr --> uklada do zasobniku specifikovaneho rezimu!
	cpsid if, #CPSR_MODE_SYS	;@ prechod do SYS modu + zakazeme preruseni
	push {r0-r12}				;@ ulozime registry (pro ted proste vsechny)
	push {lr}

	and r4, sp, #7				;@ zarovname SP na nasobek 8 (viz volaci konvence ARM)
	sub sp, sp, r4

	bl _internal_irq_handler	;@ zavolame handler IRQ

	add sp, sp, r4				;@ "odzarovname" SP -> vracime do puvodniho stavu

	pop {lr}
	pop {r0-r12}		    	;@ obnovime registry
	rfeia sp!					;@ vracime se do puvodniho stavu (ktery ulozila instrukce srsdb, takze vlastne delame pop cpsr, pop lr)

prefetch_abort_handler:
	;@ tady pak muzeme osetrit, kdyz program zasahne do mista, ktere nema mapovane ve svem virtualnim adr. prostoru
	;@ a treba vyvolat nasi obdobu segfaultu
	b hang

data_abort_handler:
	;@ tady pak muzeme osetrit, kdyz program zasahne do mista, ktere nema mapovane ve svem virtualnim adr. prostoru
	;@ a treba vyvolat nasi obdobu segfaultu
	b hang

