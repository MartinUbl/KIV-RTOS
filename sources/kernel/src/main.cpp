#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <drivers/timer.h>
#include <interrupt_controller.h>

// GPIO pin 47 je pripojeny na LED na desce (tzv. ACT LED)
constexpr uint32_t ACT_Pin = 47;

// externi funkce pro povoleni IRQ
extern "C" void enable_irq();

// je LEDka zapnuta?
volatile int LED_State = 0;

extern "C" void ACT_LED_Blinker()
{
	// prepinani LED pri kazdem vytikani casovace

	if (LED_State)
	{
		LED_State = 0;
		sGPIO.Set_Output(ACT_Pin, true);
	}
	else
	{
		LED_State = 1;
		sGPIO.Set_Output(ACT_Pin, false);
	}
}

extern "C" int _kernel_main(void)
{
	// nastavime ACT LED pin na vystupni
	sGPIO.Set_GPIO_Function(ACT_Pin, NGPIO_Function::Output);

	// inicializujeme UART kanal 0
	sUART0.Set_Baud_Rate(NUART_Baud_Rate::BR_115200);
	sUART0.Set_Char_Length(NUART_Char_Length::Char_8);

	// vypiseme ladici hlasku
	sUART0.Write("Welcome to KIV/OS RPiOS kernel\r\n");

	// zatim zakazeme IRQ casovacé
	sInterruptCtl.Disable_Basic_IRQ(hal::IRQ_Basic_Source::Timer);

	// nastavime casovac - budeme pro ted blikat LEDkou, v budoucnu muzeme treba spoustet planovac procesu
	sTimer.Enable(ACT_LED_Blinker, 0x200, NTimer_Prescaler::Prescaler_256);

	// povolime IRQ casovace
	sInterruptCtl.Enable_Basic_IRQ(hal::IRQ_Basic_Source::Timer);

	enable_irq();
	
	// nekonecna smycka - tadyodsud se CPU uz nedostane jinak, nez treba prerusenim
    while (1)
		;
	
	return 0;
}
