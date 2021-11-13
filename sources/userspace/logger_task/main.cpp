#include <stdstring.h>
#include <stdfile.h>
#include <stdmutex.h>

#include <drivers/bridges/uart_defs.h>

/**
 * Logger task
 * 
 * Prijima vsechny udalosti od ostatnich tasku a oznamuje je skrz UART hostiteli
 **/

static void fputs(uint32_t file, const char* string)
{
	write(file, string, strlen(string));
}

int main(int argc, char** argv)
{
	uint32_t uart_file = open("DEV:uart/0", NFile_Open_Mode::Write_Only);

	TUART_IOCtl_Params params;
	params.baud_rate = NUART_Baud_Rate::BR_115200;
	params.char_length = NUART_Char_Length::Char_8;
	ioctl(uart_file, NIOCtl_Operation::Set_Params, &params);

	fputs(uart_file, "UART task starting!");

	char buf[16];
	bzero(buf, 16);

	uint32_t last_tick = 0;

	while (true)
	{
		// TODO: neco delat

		uint32_t tick = get_tick_count();
		if (tick / 0x200 != last_tick)
		{
			last_tick = (tick / 0x200);

			fputs(uart_file, "\r\nTicks: ");
			fputs(uart_file, buf);
			itoa(last_tick * 0x200, buf, 16);
		}

		//sleep(0x1000);
	}

    return 0;
}
