#include <stdstring.h>
#include <stdfile.h>

#include <drivers/bridges/uart_defs.h>
#include <drivers/bridges/display_protocol.h>

// bitova mapa znaku "A" na rastru 8x6
const uint8_t chr_A[] = {
	0b00010000,
	0b00101000,
	0b00101000,
	0b01000100,
	0b01111100,
	0b10000010,
};

void Process_1()
{
	volatile int i;

	uint32_t f = open("DEV:gpio/47", NFile_Open_Mode::Write_Only);

	uint32_t disp = open("DEV:oled", NFile_Open_Mode::Read_Write);

	// vymazeme displej
	TDisplay_Clear_Packet pkt1;
	pkt1.header.cmd = NDisplay_Command::Clear;
	pkt1.clearSet = 0;
	write(disp, reinterpret_cast<char*>(&pkt1), sizeof(pkt1));

	// napiseme "A"
	uint8_t data[sizeof(TDisplay_Pixels_To_Rect) + sizeof(chr_A) - 1];
	TDisplay_Pixels_To_Rect pkt2;
	pkt2.header.cmd = NDisplay_Command::Draw_Pixel_Array_To_Rect;
	pkt2.x1 = 0;
	pkt2.y1 = 0;
	pkt2.w = 8;
	pkt2.h = 6;
	memcpy(&pkt2, data, sizeof(TDisplay_Pixels_To_Rect));
	memcpy(chr_A, &data[sizeof(TDisplay_Pixels_To_Rect) - 1], sizeof(data));
	write(disp, reinterpret_cast<char*>(data), sizeof(data));

	// a flipneme na vystup
	TDisplay_NonParametric_Packet pkt3;
	pkt3.header.cmd = NDisplay_Command::Flip;
	write(disp, reinterpret_cast<char*>(&pkt3), sizeof(pkt3));

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

	while (true)
	{
		write(f, msg, strlen(msg));

        read(rndf, reinterpret_cast<char*>(&rdbuf), 4);

        bzero(numbuf, 16);
        itoa(rdbuf, numbuf, 10);

        write(f, numbuf, strlen(numbuf));

		for (i = 0; i < 0x80000; i++)
			;
	}

	close(f);
}