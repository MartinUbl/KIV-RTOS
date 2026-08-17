#include <stdstring.h>
#include <stdfile.h>
#include <stdmutex.h>

#include <drivers/gpio.h>
#include <process/process_manager.h>

/**
 * Tilt task
 * 
 * Ceka na vstup ze senzoru naklonu, a prehraje neco na buzzeru (PWM) dle naklonu
 * Dostupne pouze na KIV-DPP-01
 **/

int main(int argc, char** argv) {
#if USE_EXPANSION_BOARD == KIVDPP01
    char state = '0';

    uint32_t tiltsensor_file = open("DEV:gpio/23", NFile_Open_Mode::Read_Only);

    NGPIO_Interrupt_Type irtype;

    irtype = NGPIO_Interrupt_Type::Falling_Edge;
    ioctl(tiltsensor_file, NIOCtl_Operation::Enable_Event_Detection, &irtype);

    uint32_t logpipe = pipe("log", 32);

    while (true) {
        wait(tiltsensor_file, 0x800);

        // "debounce" - tilt senzor bude chvili flappovat mezi vysokou a nizkou urovni
        //sleep(0x100, Deadline_Unchanged);

        read(tiltsensor_file, &state, 1);

        if (state == '0') {
            write(logpipe, "Tilt UP", 7);
        } else {
            write(logpipe, "Tilt DOWN", 10);
        }

        sleep(0x1000, Indefinite/*0x100*/);
    }

    close(tiltsensor_file);

    return 0;
#else
    // pokud neni pripojen KIV-DPP-01, tak tady nic delat nebudeme
    // fakticky by bylo samozrejme lepsi task vubec neinstancovat, ale kdyby se to nekomu nahodou povedlo, tak se nic nepokazi
    while (true) {
        sleep(0x4000, Indefinite);
    }
    return 0;
#endif
}
