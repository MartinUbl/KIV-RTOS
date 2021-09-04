#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <drivers/timer.h>
#include <drivers/oled_ssd1306.h>
#include <interrupt_controller.h>

#include <memory/memmap.h>
#include <memory/kernel_heap.h>

#include <process/process_manager.h>

#include <fs/filesystem.h>

#include <stdstring.h>
#include <stdfile.h>

// externi funkce pro povoleni IRQ
extern "C" void enable_irq();

extern "C" void Timer_Callback()
{
	sProcessMgr.Schedule();
}

// "main" procesu 1
extern void Process_1();
// "main" procesu 2
extern void Process_2();

// kernelovy "idle task" - ten se bude planovat, kdyz zrovna nic jineho nebude
void idle_loop()
{
	// tady budeme mozna v budoucnu chtit resit treba uspavani, aby nas system nezral vic elektricke energie, nez je treba

    while (1)
		;
}

extern "C" int _kernel_main(void)
{
	// debug output, kdyz budeme neco ladit; jinak vyzadujeme, aby si proces UART otevrel a spravoval
	/*
	sUART0.Open();
	sUART0.Set_Baud_Rate(NUART_Baud_Rate::BR_115200);
	sUART0.Set_Char_Length(NUART_Char_Length::Char_8);

	sUART0.Write("Welcome to KIV/OS RPiOS kernel\r\n");
	//sUART0.Close();
	*/

	// inicializace souboroveho systemu
	sFilesystem.Initialize();

	// vytvoreni hlavniho systemoveho (idle) procesu
	sProcessMgr.Create_Process(reinterpret_cast<unsigned long>(&idle_loop), true);

	// vytvoreni jednoho testovaciho procesu
	//sProcessMgr.Create_Process(reinterpret_cast<unsigned long>(&Process_1), false);
	// vytvoreni druheho testovaciho procesu
	//sProcessMgr.Create_Process(reinterpret_cast<unsigned long>(&Process_2), false);

	// zatim zakazeme IRQ casovace
	sInterruptCtl.Disable_Basic_IRQ(hal::IRQ_Basic_Source::Timer);

	// nastavime casovac - v callbacku se provadi planovani procesu
	sTimer.Enable(Timer_Callback, 0x20, NTimer_Prescaler::Prescaler_1);

	// povolime IRQ casovace
	sInterruptCtl.Enable_Basic_IRQ(hal::IRQ_Basic_Source::Timer);

	// povolime IRQ a od tohoto momentu je vse v rukou planovace
	enable_irq();

	sProcessMgr.Schedule();

	// tohle uz se mockrat nespusti - dalsi IRQ preplanuje procesor na nejaky z tasku (bud systemovy nebo uzivatelsky)
	while (true)
		;
	
	return 0;
}
