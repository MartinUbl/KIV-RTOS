#include <stdstring.h>
#include <stdfile.h>
#include <stdmutex.h>

#include <drivers/bridges/uart_defs.h>
#include <drivers/gpio.h>

int main(int argc, char** argv)
{
	volatile int i;

	const char* msg = "\r\nRandom number: ";

	uint32_t f = open("DEV:uart/0", NFile_Open_Mode::Read_Write);
    uint32_t rndf = open("DEV:trng", NFile_Open_Mode::Read_Only);

	TUART_IOCtl_Params params;
	params.baud_rate = NUART_Baud_Rate::BR_115200;
	params.char_length = NUART_Char_Length::Char_8;
	ioctl(f, NIOCtl_Operation::Set_Params, &params);

    uint32_t rdbuf;
    char numbuf[16];

    uint32_t srf = open("DEV:sr", NFile_Open_Mode::Write_Only);

    write(srf, "\x00", 1);
    for (i = 0; i < 0x80000; i++)
			;
    write(srf, "\xFF", 1);

    close(srf);

    srf = open("DEV:segd", NFile_Open_Mode::Write_Only);
    write(srf, "4", 1);

	int p = 0;

	mutex_t mtx = mutex_create("test_mtx");

	uint32_t btnfile = open("DEV:gpio/16", NFile_Open_Mode::Read_Only);
	NGPIO_Interrupt_Type irtype = NGPIO_Interrupt_Type::Rising_Edge;
	ioctl(btnfile, NIOCtl_Operation::Enable_Event_Detection, &irtype);

	while (true)
	{
		wait(btnfile);

		write(f, msg, strlen(msg));

		for (i = 0; i < 0x20000; i++)
			;

		mutex_lock(mtx);

        read(rndf, reinterpret_cast<char*>(&rdbuf), 4);

		// timto jen muzeme overit, ze nam zasobnik umele nebobtna vlivem spatne implementace context switche
		//asm volatile("mov %0, sp\n\t" : "=r" (rdbuf) );

        bzero(numbuf, 16);
        itoa(rdbuf, numbuf, 16);

        write(f, numbuf, strlen(numbuf));

		for (i = 0; i < 0x80000; i++)
			;

		mutex_unlock(mtx);

		// nasledujici instrukce v uzivatelskem rezimu neprojde
		// v systemovem rezimu (tedy nez byly procesy presunuty do "userspace") by to zakazalo preruseni
		// a mj. ukradlo vsechen procesorovy cas pro tento proces
		//asm volatile("cpsid i\r\n");
	}

	close(f);

    return 0;
}
