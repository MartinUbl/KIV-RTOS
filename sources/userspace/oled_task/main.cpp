#include <stdstring.h>
#include <stdfile.h>
#include <stdmutex.h>

#include <oled.h>

#include <drivers/bridges/uart_defs.h>
#include <drivers/gpio.h>

#include <process/process_manager.h>

/**
 * Displejovy task
 * 
 * Zobrazuje cyklicky hlasky na OLED displeji
 **/

const char* messages[] = {
	"Resistance is futile (if < 1 Ohm)",
	"I see dead pixels.",
	"There's no place like 127.0.0.1",
	"My favourite sport is ARM wrestling",
	"B || !B, that is the question.",
	"Segmentation fault (core dumped)"
};

int main(int argc, char** argv)
{
	COLED_Display disp("DEV:oled");
	disp.Clear(false);
	disp.Put_String(10, 8, "KIV-RTOS init...");

#if USE_EXPANSION_BOARD == KIVDPP02
	disp.Put_String(10, 18, "KIV-DPP-02");
#elif USE_EXPANSION_BOARD == KIVDPP01
	disp.Put_String(10, 18, "KIV-DPP-01");
#else
	disp.Put_String(10, 18, "No expansion board");
#endif

	disp.Flip();

	uint32_t trng_file = open("DEV:trng", NFile_Open_Mode::Read_Only);
	uint32_t num = 0;

	sleep(0x1000, 0x800);

	while (true)
	{
		// ziskame si nahodne cislo a vybereme podle toho zpravu
		read(trng_file, reinterpret_cast<char*>(&num), sizeof(num));
		const char* msg = messages[num % (sizeof(messages) / sizeof(const char*))];

		disp.Clear(false);
		disp.Put_String(0, 0, msg);
		disp.Flip();

		sleep(0x4000, 0x800);
	}

    return 0;
}
