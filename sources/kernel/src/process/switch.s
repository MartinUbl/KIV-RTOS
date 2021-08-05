;@ konstanty rezimu procesoru
.equ    CPSR_MODE_USR,          0x10	;@ uzivatelsky rezim - v tom bezi uzivatelske procesy
.equ    CPSR_MODE_IRQ,          0x12	;@ IRQ rezim - v tom nam mj. bude tikat scheduler
.equ    CPSR_MODE_SYS,          0x1F	;@ systemovy rezim - v tom bezi systemove procesy (s plnymi pravy na hardware)

process_terminate:
	;@ TODO: terminate, volani syscallu exit
	b process_terminate	;@ az bude implementovany terminate syscall, tak se tato radka ani nespusti

.global user_process_bootstrap
;@ Process bootstrapping pro uzivatelsky proces - kernelovy "obal" procesu
;@ Vyzaduje na zasobniku pushnutou hodnotu vstupniho bodu procesu
user_process_bootstrap:
	cps #CPSR_MODE_USR	;@ prepneme se do uzivatelskeho rezimu
    add lr, pc, #8      ;@ ulozime do lr hodnotu PC+8, abychom se korektne vratili na instrukci po nasledujici
    pop {pc}            ;@ vyzvedneme si ulozenou hodnotu cile
    b process_terminate	;@ korektne musime proces ukoncit - odevzdat zdroje jadru a proces prevest do stavu ukonceno

.global system_process_bootstrap
;@ Process bootstrapping pro systemovy proces - kernelovy "obal" procesu
;@ Vyzaduje na zasobniku pushnutou hodnotu vstupniho bodu procesu
system_process_bootstrap:
	cps #CPSR_MODE_SYS	;@ prepneme se do systemoveho rezimu
    add lr, pc, #8      ;@ ulozime do lr hodnotu PC+8, abychom se korektne vratili na instrukci po nasledujici
    pop {pc}            ;@ vyzvedneme si ulozenou hodnotu cile
    b process_terminate	;@ korektne musime proces ukoncit - odevzdat zdroje jadru a proces prevest do stavu ukonceno

.global restore_irq_sp
;@ Obnovi SP do IRQ modu a vrati se zpet do SYS modu
;@ vyzaduje hodnotu SP jako prvni parametr (v r0)
restore_irq_sp:
	srsdb #CPSR_MODE_IRQ!
	cps #CPSR_MODE_IRQ
	mov r1, sp
	mov sp, r0
	rfeia r1!

.global context_switch
;@ Prepnuti procesu ze soucasneho na jiny, ktery jiz byl planovany
;@ r0 - novy proces
;@ r1 - stary proces
context_switch:
	;@ musime byt v SYS rezimu, protoze uz budeme nacitat registry procesu, ale stale budeme pak chtit moci
	;@ zmenit stav procesoru (msr instrukci) - to muzeme vlastne jen ze SYS rezimu
	cpsie i, #CPSR_MODE_SYS

	ldr sp, [r0, #4]        ;@ nacist SP noveho procesu
	pop {lr}
	pop {r0-r12}            ;@ obnovit registry noveho procesu
	rfeia sp!				;@ virtualni pop {cpsr}, pop {pc}

.global context_switch_first
;@ Prepnuti procesu ze soucasneho na jiny, ktery jeste nebyl planovany
;@ r0 - novy proces
;@ r1 - stary proces
context_switch_first:
	cps #CPSR_MODE_SYS

    ldr r3, [r0, #0]        ;@ "budouci" PC do r3 (entry point procesu)
    ldr r2, [r0, #8]        ;@ "vstupni" PC do r2 (bootstrap procesu v kernelu)
    ldr sp, [r0, #4]        ;@ nacteme stack pointer procesu
    push {r3}               ;@ budouci navratova adresa -> do zasobniku, bootstrap si ji tamodsud vyzvedne
    push {r2}               ;@ pushneme si i bootstrap adresu, abychom ji mohli obnovit do PC
    cpsie i                 ;@ povolime preruseni (v budoucich switchich uz bude flaga ulozena v cpsr/spsr)
    pop {pc}                ;@ vybereme ze zasobniku novou hodnotu PC (PC procesu)
