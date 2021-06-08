#include "peripherals.h"

int loader_main(void)
{
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
    uart_send(0x0D);
    uart_send(0x0A);

    data = 0;
    state = 0;
    count = 0;
    sum = 0;
    addr = 0;
    type = 0;

    entry = 0x00008000;

    while (1)
    {
        ra = uart_recv();
        switch (state)
        {
            case 0:
            {
                // S = ridici kod formatu SREC - nasleduje zbytek prikazu
                if (ra == 'S')
                {
                    short_blink();
                    sum = 0;
                    state++;
                }
                // G = ridici kod protokolu - nastartuje nahravany program
                else if (ra == 'g' || ra == 'G')
                {
                    uart_flush();
                    BRANCHTO(entry);
                }
                // P = ridici kod protokolu, overuje, zda se bootloader nacetl, zablika ACT LEDkou
                else if (ra == 'p' || ra == 'P')
                {
                    blink();
                }
                break;
            }
            case 1:
            {
                switch (ra)
                {
                    case '0':           // S0 = inicializacni retezec - ignorujeme vse co nasleduje
                    {
                        state = 0;
                        break;
                    }
                    case '3':           // S3 = datova zprava - prijmeme a zapiseme do pameti
                    {
                        type = 3;
                        state++;
                        break;
                    }
                    case '7':           // S7 = ukonceni nahravani, spusteni nove nahraneho programu
                    {
                        type = 7;
                        state++;
                        break;
                    }
                    default:            // neznamy S-kod
                    {
                        failstring(0);
                        return 1;
                    }
                }
                break;
            }
            case 2:
            {
                count = ctonib(ra);
                state++;
                break;
            }
            case 3:
            {
                count <<= 4;
                count |= ctonib(ra);
                if (count < 5)
                {
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
            case 10:
            {
                addr <<= 4;
                addr |= ctonib(ra);
                state++;
                break;
            }
            case  5:
            case  7:
            case  9:
            {
                count--;
                addr <<= 4;
                addr |= ctonib(ra);
                sum += addr&0xFF;
                state++;
                break;
            }
            case 11:
            {
                count--;
                addr <<= 4;
                addr |= ctonib(ra);
                sum += addr&0xFF;
                state++;
                break;
            }
            case 12:
            {
                data = ctonib(ra);
                state++;
                break;
            }
            case 13:
            {
                data <<= 4;
                data |= ctonib(ra);
                sum += data&0xFF;
                count--;
                if (count == 0)
                {
                    if (type == 7)
                        entry = addr;

                    sum &= 0xFF;
                    if (sum != 0xFF)
                    {
                        failstring(2);
                        return 1;
                    }
                    state = 0;
                }
                else
                {
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
