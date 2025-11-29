#include <stdstring.h>
#include <stdfile.h>
#include <drivers/bridges/uart_defs.h>
#include <oled.h>   // pro OLED podporu

/**
 * Telemetry task
 *
 * Periodicky čte snapshot telemetrie z "DEV:telemetry"
 * a vypisuje jej buď přes UART nebo OLED.
 */

// 0 = OLED, 1 = UART
#define USE_UART_OUTPUT 0

static void fputs(uint32_t file, const char* str)
{
    write(file, str, strlen(str));
}

int main(int argc, char** argv)
{
    uint32_t telem = open("DEV:telemetry", NFile_Open_Mode::Read_Only);

#if USE_UART_OUTPUT
    uint32_t out = open("DEV:uart/0", NFile_Open_Mode::Write_Only);

    TUART_IOCtl_Params params;
    params.baud_rate   = NUART_Baud_Rate::BR_115200;
    params.char_length = NUART_Char_Length::Char_8;
    ioctl(out, NIOCtl_Operation::Set_Params, &params);

    auto write_output = [&](const char* s) { fputs(out, s); };

#else
    COLED_Display disp("DEV:oled");
    disp.Clear(false);
    disp.Put_String(0, 0, "Telemetry ready");
    disp.Flip();

    char line[32];
    auto write_output = [&](const char* s)
    {
        disp.Clear(false);
        disp.Put_String(0, 0, s);
        disp.Flip();
    };
#endif

    char buf[256];

    while (true)
    {
        // přečteme snapshot z telemetry
        uint32_t n = read(telem, buf, sizeof(buf) - 1);
        buf[n] = '\0';

#if USE_UART_OUTPUT
        write_output("\r\n--- Telemetry ---\r\n");
        write_output(buf);
        write_output("-----------------\r\n");
#else
        // OLED verze – zkrácený přehled
        write_output(buf);
#endif

        // pauza (cca 1 sekunda)
        for (volatile int i = 0; i < 0x100000; i++);
    }

    close(telem);
#if USE_UART_OUTPUT
    close(out);
#endif
    return 0;
}
