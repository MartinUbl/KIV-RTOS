#include "peripherals.h"

#define BINCMD_SET_CURSOR  0x01
#define BINCMD_WRITE       0x02
#define BINCMD_WRITE_ZERO  0x03
#define BINCMD_SET_ENTRY   0x04
#define BINCMD_GO          0x05
#define BINCMD_SET_BAUD    0x06
#define BINARY_ACK         'K'
#define AUX_MU_BAUD_REG    0x20215068

static unsigned int uart_recv_u16be(void) {
    unsigned int v;

    v = uart_recv() & 0xFF;
    v <<= 8;
    v |= uart_recv() & 0xFF;

    return v;
}

static unsigned int uart_recv_u32be(void) {
    unsigned int v;

    v = uart_recv() & 0xFF;
    v <<= 8;
    v |= uart_recv() & 0xFF;
    v <<= 8;
    v |= uart_recv() & 0xFF;
    v <<= 8;
    v |= uart_recv() & 0xFF;

    return v;
}

static int uart_recv_binary_upgrade(void) {
    if (uart_recv() != 'B') {
        return 0;
    }
    if (uart_recv() != 'I') {
        return 0;
    }
    if (uart_recv() != 'N') {
        return 0;
    }
    if (uart_recv() != '1') {
        return 0;
    }

    return 1;
}

static void uart_set_baud_divisor(unsigned int divisor) {
    PUT32(AUX_MU_BAUD_REG, divisor);
}

static unsigned int write_uart_data(unsigned int addr, unsigned int count) {
    unsigned int data;

    while ((addr & 3) && count > 0) {
        PUT8(addr, uart_recv());
        addr++;
        count--;
    }

    while (count >= 4) {
        data = uart_recv() & 0xFF;
        data |= (uart_recv() & 0xFF) << 8;
        data |= (uart_recv() & 0xFF) << 16;
        data |= (uart_recv() & 0xFF) << 24;
        PUT32(addr, data);
        addr += 4;
        count -= 4;
    }

    while (count > 0) {
        PUT8(addr, uart_recv());
        addr++;
        count--;
    }

    return addr;
}

static unsigned int write_zeroes(unsigned int addr, unsigned int count) {
    while ((addr & 3) && count > 0) {
        PUT8(addr, 0);
        addr++;
        count--;
    }

    while (count >= 4) {
        PUT32(addr, 0);
        addr += 4;
        count -= 4;
    }

    while (count > 0) {
        PUT8(addr, 0);
        addr++;
        count--;
    }

    return addr;
}

int loader_main(void) {
    unsigned int state;
    unsigned int ra;
    unsigned int type;
    unsigned int count;
    unsigned int sum;
    unsigned int entry;
    unsigned int addr;
    unsigned int data;

    uart_init();
    uart_send('S');
    uart_send('R');
    uart_send('E');
    uart_send('C');
    uart_send('-');
    uart_send('2');
    uart_send('0');
    uart_send('0');
    uart_send(0x0D);
    uart_send(0x0A);

    data = 0;
    state = 0;
    count = 0;
    sum = 0;
    addr = 0;
    type = 0;

    entry = 0x00008000;

    while (1) {
        ra = uart_recv();
        switch (state) {
            case 0: {
                // S = ridici kod formatu SREC - nasleduje zbytek prikazu
                if (ra == 'S') {
                    short_blink();
                    sum = 0;
                    state++;
                }
                // G = ridici kod protokolu - nastartuje nahravany program
                else if (ra == 'g' || ra == 'G') {
                    uart_flush();
                    BRANCHTO(entry);
                }
                // P = ridici kod protokolu, overuje, zda se bootloader nacetl, zablika ACT LEDkou
                else if (ra == 'p' || ra == 'P') {
                    blink();
                }
                // U = ridici kod protokolu - prepnuti do binarniho rezimu prenosu
                else if (ra == 'U') {
                    if (!uart_recv_binary_upgrade()) {
                        failstring(3);
                        return 1;
                    }
                    state = 100;
                    uart_send(BINARY_ACK);
                }
                break;
            }
            case 100: {
                switch (ra) {
                    case BINCMD_SET_CURSOR: {
                        addr = uart_recv_u32be();
                        short_blink();
                        uart_send(BINARY_ACK);
                        break;
                    }
                    case BINCMD_WRITE: {
                        count = uart_recv_u16be();
                        addr = write_uart_data(addr, count);
                        short_blink();
                        uart_send(BINARY_ACK);
                        break;
                    }
                    case BINCMD_WRITE_ZERO: {
                        count = uart_recv_u16be();
                        addr = write_zeroes(addr, count);
                        short_blink();
                        uart_send(BINARY_ACK);
                        break;
                    }
                    case BINCMD_SET_ENTRY: {
                        entry = uart_recv_u32be();
                        uart_send(BINARY_ACK);
                        break;
                    }
                    case BINCMD_GO:
                    case 'g':
                    case 'G': {
                        uart_flush();
                        BRANCHTO(entry);
                        break;
                    }
                    case BINCMD_SET_BAUD: {
                        data = uart_recv_u16be();
                        uart_send(BINARY_ACK);
                        uart_flush();
                        uart_set_baud_divisor(data);
                        break;
                    }
                    case 'p':
                    case 'P': {
                        blink();
                        break;
                    }
                    default: {
                        failstring(4);
                        return 1;
                    }
                }
                break;
            }
            case 1: {
                switch (ra) {
                    case '0': {         // S0 = inicializacni retezec - ignorujeme vse co nasleduje
                        state = 0;
                        break;
                    }
                    case '3': {         // S3 = datova zprava - prijmeme a zapiseme do pameti
                        type = 3;
                        state++;
                        break;
                    }
                    case '7': {         // S7 = ukonceni nahravani, spusteni nove nahraneho programu
                        type = 7;
                        state++;
                        break;
                    }
                    default: {          // neznamy S-kod
                        failstring(0);
                        return 1;
                    }
                }
                break;
            }
            case 2: {
                count = ctonib(ra);
                state++;
                break;
            }
            case 3: {
                count <<= 4;
                count |= ctonib(ra);
                if (count < 5) {
                    failstring(1);
                    return 1;
                }

                sum += count&0xFF;
                addr = 0;
                state++;
                break;
            }
            case  4:
            case  6:
            case  8:
            case 10: {
                addr <<= 4;
                addr |= ctonib(ra);
                state++;
                break;
            }
            case  5:
            case  7:
            case  9: {
                count--;
                addr <<= 4;
                addr |= ctonib(ra);
                sum += addr&0xFF;
                state++;
                break;
            }
            case 11: {
                count--;
                addr <<= 4;
                addr |= ctonib(ra);
                sum += addr&0xFF;
                state++;
                break;
            }
            case 12: {
                data = ctonib(ra);
                state++;
                break;
            }
            case 13: {
                data <<= 4;
                data |= ctonib(ra);
                sum += data&0xFF;
                count--;
                if (count == 0) {
                    if (type == 7) {
                        entry = addr;
                    }

                    sum &= 0xFF;
                    if (sum != 0xFF) {
                        failstring(2);
                        return 1;
                    }
                    state = 0;
                } else {
                    PUT8(addr,data);
                    addr++;
                    state = 12;
                }
                break;
            }
        }

    }
    return 0;
}
