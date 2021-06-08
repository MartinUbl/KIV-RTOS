#include "peripherals.h"

#define PBASE 0x20000000

#define ARM_TIMER_CTL   (PBASE+0x0000B408)
#define ARM_TIMER_CNT   (PBASE+0x0000B420)

#define GPFSEL1         (PBASE+0x00200004)
#define GPFSEL3         (PBASE+0x0020000C)
#define GPFSEL4         (PBASE+0x00200010)
#define GPSET0          (PBASE+0x0020001C)
#define GPCLR0          (PBASE+0x00200028)
#define GPSET1          (PBASE+0x00200020)
#define GPCLR1          (PBASE+0x2020002C)

#define AUX_ENABLES     (PBASE+0x00215004)
#define AUX_MU_IO_REG   (PBASE+0x00215040)
#define AUX_MU_IER_REG  (PBASE+0x00215044)
#define AUX_MU_IIR_REG  (PBASE+0x00215048)
#define AUX_MU_LCR_REG  (PBASE+0x0021504C)
#define AUX_MU_MCR_REG  (PBASE+0x00215050)
#define AUX_MU_LSR_REG  (PBASE+0x00215054)
#define AUX_MU_MSR_REG  (PBASE+0x00215058)
#define AUX_MU_SCRATCH  (PBASE+0x0021505C)
#define AUX_MU_CNTL_REG (PBASE+0x00215060)
#define AUX_MU_STAT_REG (PBASE+0x00215064)
#define AUX_MU_BAUD_REG (PBASE+0x00215068)

#define ACT_LED_BIT     (1 << (47-32))

unsigned int uart_lcr(void)
{
    return GET32(AUX_MU_LSR_REG);
}

unsigned int uart_recv(void)
{
    while (!(GET32(AUX_MU_LSR_REG) & 0x01))
        ;

    return GET32(AUX_MU_IO_REG) & 0xFF;
}

unsigned int uart_check(void)
{
    if (GET32(AUX_MU_LSR_REG) & 0x01)
        return 1;
    return 0;
}

void uart_send(unsigned int c)
{
    while(!(GET32(AUX_MU_LSR_REG) & 0x20))
        ;

    PUT32(AUX_MU_IO_REG,c);
}

void uart_flush(void)
{
    while(!(GET32(AUX_MU_LSR_REG)&0x40))
        ;
}

void failstring(unsigned int d)
{
    uart_send('F');
    uart_send('0' + (d / 100) % 10);
    uart_send('0' + (d / 10) % 10);
    uart_send('0' + (d % 10));
    uart_send(0x0D);
    uart_send(0x0A);
}

void okstring(void)
{
    uart_send('X');
    uart_send('F');
    uart_send('O');
    uart_send('K');
    uart_send(0x0D);
    uart_send(0x0A);
}

void uart_init(void)
{
    unsigned int ra;

    PUT32(AUX_ENABLES,1);
    PUT32(AUX_MU_IER_REG,0);
    PUT32(AUX_MU_CNTL_REG,0);
    PUT32(AUX_MU_LCR_REG,3);
    PUT32(AUX_MU_MCR_REG,0);
    PUT32(AUX_MU_IER_REG,0);
    PUT32(AUX_MU_IIR_REG,0xC6);
    PUT32(AUX_MU_BAUD_REG,270);
    ra = GET32(GPFSEL1);
    ra &= ~(7<<12); // gpio14
    ra |= 2<<12;    // alt5
    ra &= ~(7<<15); // gpio15
    ra |= 2<<15;    // alt5
    PUT32(GPFSEL1,ra);
    PUT32(AUX_MU_CNTL_REG,3);
}

// aktivni "spanek" - spali nekolik taktu procesoru naprazdno
void active_sleep(unsigned int ticks)
{
    unsigned int ra;
    for (ra = 0; ra < ticks; ra++)
        dummy(ra);
}

void init_led(void)
{
    unsigned int ra;

    ra = GET32(GPFSEL4);
    ra &= ~(7<<21);
    ra |= 1<<21;
    PUT32(GPFSEL4,ra);

    PUT32(GPSET1, ACT_LED_BIT);
}

void blink(void)
{
    PUT32(GPCLR1, ACT_LED_BIT);
    active_sleep(0x80000);
    PUT32(GPSET1, ACT_LED_BIT);
    active_sleep(0x80000);
    PUT32(GPCLR1, ACT_LED_BIT);
    active_sleep(0x80000);
    PUT32(GPSET1, ACT_LED_BIT);
    active_sleep(0x300000);
}

int blinkstate = 0;

void short_blink(void)
{
    if (blinkstate == 0)
    {
        PUT32(GPCLR1, ACT_LED_BIT);
        blinkstate = 1;
    }
    else
    {
        PUT32(GPSET1, ACT_LED_BIT);
        blinkstate = 0;
    }
}

unsigned int ctonib(unsigned int c)
{
    if (c > '9')
        c -= 7;
    return (c & 0x0F);
}
