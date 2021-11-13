#include <stdstring.h>
#include <stdfile.h>
#include <stdmutex.h>

#include <drivers/gpio.h>
#include <process/process_manager.h>

/**
 * Tilt task
 * 
 * Ceka na vstup ze senzoru naklonu, a prehraje neco na buzzeru (PWM) dle naklonu
 **/

int main(int argc, char** argv)
{
	char state = '0';

	uint32_t tiltsensor_file = open("DEV:gpio/23", NFile_Open_Mode::Read_Only);
	// TODO: otevrit PWM

	NGPIO_Interrupt_Type irtype;
	
	irtype = NGPIO_Interrupt_Type::Rising_Edge;
	ioctl(tiltsensor_file, NIOCtl_Operation::Enable_Event_Detection, &irtype);

	irtype = NGPIO_Interrupt_Type::Falling_Edge;
	ioctl(tiltsensor_file, NIOCtl_Operation::Enable_Event_Detection, &irtype);

	set_task_deadline(Indefinite);

	while (true)
	{
		set_task_deadline(Indefinite);
		/*wait(tiltsensor_file);
		set_task_deadline(0x2000);

		read(tiltsensor_file, &state, 1);

		if (state == '0')
		{
			// TODO: prehrat neco
		}
		else
		{
			// TODO: prehrat neco jineho
		}*/

		sleep(0x1000);
	}

	// TODO zavrit PWM
	close(tiltsensor_file);

    return 0;
}
