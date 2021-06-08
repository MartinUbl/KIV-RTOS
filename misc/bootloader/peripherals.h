#ifndef __PERIPHERALS_H__
#define __PERIPHERALS_H__

extern void PUT8(unsigned int, unsigned int);
extern void PUT32(unsigned int, unsigned int);
extern void PUT16(unsigned int, unsigned int);
extern unsigned int GET32(unsigned int);
extern void BRANCHTO(unsigned int);

extern void uart_init(void);
extern void uart_send(unsigned int);
extern unsigned int uart_recv(void);
extern void uart_flush(void);

void failstring(unsigned int d);
void okstring(void);

extern void dummy(unsigned int);

extern void init_led(void);
extern void blink(void);
extern void short_blink(void);

unsigned int ctonib(unsigned int c);

#endif
