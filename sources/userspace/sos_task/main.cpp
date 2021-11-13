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

uint32_t sos_led;
uint32_t button;

void blink(bool short_blink)
{
	write(sos_led, "1", 1);
	sleep(short_blink ? 0x800 : 0x1000);
	write(sos_led, "0", 1);
}

int main(int argc, char** argv)
{
	sos_led = open("DEV:gpio/18", NFile_Open_Mode::Write_Only);
	button = open("DEV:gpio/16", NFile_Open_Mode::Read_Only);

	NGPIO_Interrupt_Type irtype = NGPIO_Interrupt_Type::Rising_Edge;
	ioctl(button, NIOCtl_Operation::Enable_Event_Detection, &irtype);

	while (true)
	{
		set_task_deadline(Indefinite);

		// pockame na stisk klavesy
		wait(button);
		// nastavime nejaky deadline, do kdy je treba vyblikat SOS
		set_task_deadline(0x200000);

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
	}

	close(button);
	close(sos_led);

    return 0;
}
