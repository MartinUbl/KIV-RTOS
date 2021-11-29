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
	char tickbuf[16];
	bzero(buf, 16);
	bzero(tickbuf, 16);

	uint32_t last_tick = 0;

	uint32_t logpipe = pipe("log", 32);

	while (true)
	{
		wait(logpipe, 1, 0x1000);

		uint32_t v = read(logpipe, buf, 15);
		if (v > 0)
		{
			buf[v] = '\0';
			fputs(uart_file, "\r\n[ ");
			uint32_t tick = get_tick_count();
			itoa(tick, tickbuf, 16);
			fputs(uart_file, tickbuf);
			fputs(uart_file, "]: ");
			fputs(uart_file, buf);
		}
	}

    return 0;
}
