#include <stdstring.h>
#include <stdfile.h>
#include <stdmutex.h>

#include <drivers/gpio.h>
#include <process/process_manager.h>

#include <oled.h>

/**
 * SOS blinker task
 * 
 * Ceka na stisk tlacitka, po stisku vyblika LEDkou "SOS" signal
 **/

constexpr uint32_t symbol_tick_delay = 0x400;
constexpr uint32_t char_tick_delay = 0x1000;

uint32_t sos_led_1;
uint32_t sos_led_2;
uint32_t button;

void blink(bool short_blink) {
    write(sos_led_1, "1", 1);
    sleep(short_blink ? 0x800 : 0x1000);
    write(sos_led_1, "0", 1);
}

int main(int argc, char** argv) {
#if USE_EXPANSION_BOARD == KIVDPP02
    sos_led_1 = open("DEV:gpio/24", NFile_Open_Mode::Write_Only);
    sos_led_2 = open("DEV:gpio/23", NFile_Open_Mode::Write_Only);

    button = open("DEV:gpio/16", NFile_Open_Mode::Read_Only);
#elif USE_EXPANSION_BOARD == KIVDPP01
    sos_led_1 = open("DEV:gpio/18", NFile_Open_Mode::Write_Only);
    sos_led_2 = open("DEV:gpio/47", NFile_Open_Mode::Write_Only);

    button = open("DEV:gpio/16", NFile_Open_Mode::Read_Only);
#else
    // pokud neni pripojen zadny expansion board, tak tady nic delat nebudeme - nevime, kde jsou LEDky a tlacitko
    // fakticky by bylo samozrejme lepsi task vubec neinstancovat, ale kdyby se to nekomu nahodou povedlo, tak se nic nepokazi
    sleep(0x4000, Indefinite);
    return 0;
#endif

    NGPIO_Interrupt_Type irtype = NGPIO_Interrupt_Type::Rising_Edge;
    ioctl(button, NIOCtl_Operation::Enable_Event_Detection, &irtype);

    uint32_t logpipe = pipe("log", 32);

    while (true) {
        // pockame na stisk klavesy
        wait(button, 1, 0x300);

        // tady by se mohla hodit inverze priorit:
        // 1) pipe je plna
        // 2) my mame deadline 0x300
        // 3) log task ma deadline 0x1000
        // 4) jiny task ma deadline 0x500
        // jiny task dostane prednost pred log taskem, a pokud nesplni v kratkem case svou ulohu, tento task prekroci deadline
        // TODO: inverzi priorit bychom docasne zvysili prioritu (zkratili deadline) log tasku, aby vyprazdnil pipe a my se mohli odblokovat co nejdrive
        write(logpipe, "SOS!", 5);

        write(sos_led_2, "1", 1);

        blink(true);
        sleep(symbol_tick_delay);
        blink(true);
        sleep(symbol_tick_delay);
        blink(true);

        sleep(char_tick_delay);

        blink(false);
        sleep(symbol_tick_delay);
        blink(false);
        sleep(symbol_tick_delay);
        blink(false);
        sleep(symbol_tick_delay);

        sleep(char_tick_delay);

        blink(true);
        sleep(symbol_tick_delay);
        blink(true);
        sleep(symbol_tick_delay);
        blink(true);

        write(sos_led_2, "0", 1);
    }

    close(button);
    close(sos_led_1);
    close(sos_led_2);

    return 0;
}
