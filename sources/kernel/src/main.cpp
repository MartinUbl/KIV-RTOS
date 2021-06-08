#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <drivers/timer.h>
#include <interrupt_controller.h>

#include <memory/memmap.h>
#include <memory/kernel_heap.h>

#include <process/process_manager.h>

#include <stdstring.h>

// je LEDka zapnuta?
volatile bool LED_State = false;

// GPIO pin 47 je pripojeny na LED na desce (tzv. ACT LED)
constexpr uint32_t ACT_Pin = 47;

// externi funkce pro povoleni IRQ
extern "C" void enable_irq();

extern "C" void Timer_Callback()
{
	sProcessMgr.Schedule();

	sGPIO.Set_Output(ACT_Pin, LED_State);
	LED_State = !LED_State;
}

extern "C" void Process_1()
{
	volatile int i;

	sUART0.Write("Process 1\r\n");

	while (true)
	{
		sUART0.Write('1');

		for (i = 0; i < 0x10000; i++)
			;
	}
}

extern "C" void Process_2()
{
	volatile int i;

	sUART0.Write("Process 2\r\n");

	while (true)
	{
		sUART0.Write('2');

		for (i = 0; i < 0x10000; i++)
			;
	}
}

extern "C" int _kernel_main(void)
{
	// nastavime ACT LED pin na vystupni
	sGPIO.Set_GPIO_Function(ACT_Pin, NGPIO_Function::Output);
	sGPIO.Set_Output(ACT_Pin, false);

	// inicializujeme UART kanal 0
	sUART0.Set_Baud_Rate(NUART_Baud_Rate::BR_115200);
	sUART0.Set_Char_Length(NUART_Char_Length::Char_8);

	// vypiseme ladici hlasku
	sUART0.Write("Welcome to KIV/OS RPiOS kernel\r\n");

	sProcessMgr.Create_Main_Process();

	sProcessMgr.Create_Process(reinterpret_cast<unsigned long>(&Process_1));
	sProcessMgr.Create_Process(reinterpret_cast<unsigned long>(&Process_2));

	// zatim zakazeme IRQ casovace
	sInterruptCtl.Disable_Basic_IRQ(hal::IRQ_Basic_Source::Timer);

	// nastavime casovac - v callbacku se provadi planovani procesu
	sTimer.Enable(Timer_Callback, 0x20, NTimer_Prescaler::Prescaler_256);

	// povolime IRQ casovace
	sInterruptCtl.Enable_Basic_IRQ(hal::IRQ_Basic_Source::Timer);

	enable_irq();

	// nekonecna smycka - tadyodsud se CPU uz nedostane jinak, nez treba prerusenim
    while (1)
		;
	
	return 0;
}
