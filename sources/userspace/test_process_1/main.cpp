#include <stdstring.h>
#include <stdfile.h>
#include <stdmutex.h>

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

	mutex_t mtx = mutex_create("test_mtx");

	while (true)
	{
		write(f, "1", 1);

		for (i = 0; i < 0x40000; i++)
			;

		mutex_lock(mtx);

		write(f, "0", 1);

		for (i = 0; i < 0x800000; i++)
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
            {
				disp.Put_String(10, 10, "Hello world!");

                // toto nutne vyvola data abort (pokus o cteni/zapis pameti, na kterou nemame prava dle tabulky stranek):
                //volatile int* sr = (int*)(0xC0001000);
                //*sr = 15;
            }

			disp.Flip();

			lcd_state = (lcd_state + 1) % 3;
		}

		mutex_unlock(mtx);
	}

	close(f);

    return 0;
}
