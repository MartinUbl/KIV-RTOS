;@ konstanty rezimu procesoru
.equ    CPSR_MODE_USR,          0x10	;@ uzivatelsky rezim - v tom bezi uzivatelske procesy
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

.global context_switch
;@ Prepnuti procesu ze soucasneho na jiny, ktery jiz byl planovany
;@ r0 - novy proces
;@ r1 - stary proces
context_switch:
	mrs r12, cpsr           ;@ ulozit CPU state do r12
	push {lr}               ;@ push LR
	push {r0}               ;@ push SP
	push {r0-r12}           ;@ push registru
	str sp, [r1, #4]        ;@ ulozit SP stareho procesu

	ldr sp, [r0, #4]        ;@ nacist SP noveho procesu
	pop {r0-r12}            ;@ obnovit registry noveho procesu
	msr cpsr_c, r12         ;@ obnovit CPU state
	pop {lr}
	pop {pc}				;@ navrat do kontextu provadeni noveho procesu - do PC se nahraje puvodni LR (navratova adresa)

.global context_switch_first
;@ Prepnuti procesu ze soucasneho na jiny, ktery jeste nebyl planovany
;@ r0 - novy proces
;@ r1 - stary proces
context_switch_first:
	mrs r12, cpsr           ;@ ulozit CPU state do r12
	push {lr}               ;@ push LR
	push {r13}              ;@ push SP
	push {r0-r12}           ;@ push registru
	str sp, [r1, #4]        ;@ ulozit SP stareho procesu

	ldr sp, [r0, #4]        ;@ nacteme stack pointer procesu
    ldr r3, [r0, #0]        ;@ "budouci" PC do r3 (entry point procesu)
    ldr r2, [r0, #8]        ;@ "vstupni" PC do r2 (bootstrap procesu v kernelu)
    push {r3}               ;@ budouci navratova adresa -> do zasobniku, bootstrap si ji tamodsud vyzvedne
    push {r2}               ;@ pushneme si i bootstrap adresu, abychom ji mohli obnovit do PC
    cpsie i                 ;@ povolime preruseni (v budoucich switchich uz bude flaga ulozena v cpsr/spsr)
    pop {pc}                ;@ vybereme ze zasobniku novou hodnotu PC (PC procesu)
