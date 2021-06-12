#include <stdstring.h>
#include <stdfile.h>
#include <drivers/uart.h> // tohle bude jeste predmetem oddeleni od kernel zdrojaku

void Process_1()
{
	volatile int i;

	uint32_t f = open("DEV:gpio/18", NFile_Open_Mode::Write_Only);

	while (true)
	{
		write(f, "1", 1);

		for (i = 0; i < 0x40000; i++)
			;

		write(f, "0", 1);

		for (i = 0; i < 0x40000; i++)
			;
	}

	close(f);
}

void Process_2()
{
	volatile int i;

	const char* msg = "Hello!\r\n";

	uint32_t f = open("DEV:uart/0", NFile_Open_Mode::Read_Write);

	TUART_IOCtl_Params params;
	params.baud_rate = NUART_Baud_Rate::BR_115200;
	params.char_length = NUART_Char_Length::Char_8;
	ioctl(f, NIOCtl_Operation::Set_Params, &params);

	while (true)
	{
		write(f, msg, strlen(msg));

		for (i = 0; i < 0x80000; i++)
			;
	}

	close(f);
}