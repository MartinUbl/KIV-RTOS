#include <stdstring.h>
#include <stdfile.h>

#include <drivers/bridges/uart_defs.h>

#include <oled.h>

int main(int argc, char** argv)
{
	volatile int i;

	uint32_t f = open("DEV:gpio/18", NFile_Open_Mode::Write_Only);

	COLED_Display disp("DEV:oled");
	disp.Clear(false);
	disp.Put_String(10, 10, "ABC abc 123");
	disp.Flip();

	int lcd_state = 0;
	const int counter_rst = 10;
	int counter = counter_rst;

	while (true)
	{
		write(f, "1", 1);

		for (i = 0; i < 0x40000; i++)
			;

		write(f, "0", 1);

		for (i = 0; i < 0x40000; i++)
			;

		if (--counter == 0)
		{
			counter = counter_rst;

			disp.Clear(false);

			if (lcd_state == 0)
				disp.Put_String(10, 10, "Welcome");
			else if (lcd_state == 1)
				disp.Put_String(10, 10, "in KIV-RTOS");
			else if (lcd_state == 2)
				disp.Put_String(10, 10, "Hello world!");

			disp.Flip();

			lcd_state = (lcd_state + 1) % 3;
		}
	}

	close(f);

    return 0;
}
