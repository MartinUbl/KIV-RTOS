#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage:  uart_flasher.exe <filename> <port identifier>" << std::endl;
        return 3;
    }

    std::string filename = argv[1];

    HANDLE hComm;

    hComm = CreateFileA(argv[2], GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (hComm == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Error in opening serial port" << std::endl;
        return 1;
    }

    DCB serialParams;

    GetCommState(hComm, &serialParams);
    serialParams.BaudRate = CBR_115200;
    serialParams.ByteSize = 8;
    serialParams.StopBits = ONESTOPBIT;
    serialParams.Parity = NOPARITY;
    SetCommState(hComm, &serialParams);

    std::cout << "Opening serial port successful" << std::endl;

    DWORD l;
    std::string ln;

    {
        std::ifstream ifile(argv[1]);
        if (!ifile.is_open())
        {
            std::cerr << "Error in opening input file" << std::endl;
            CloseHandle(hComm);
            return 1;
        }

        std::cout << "Uploading..." << std::endl;

        while (std::getline(ifile, ln))
        {
            WriteFile(hComm, ln.c_str(), ln.length(), &l, NULL);
            Sleep(10);
        }
    }

    std::cout << "Launching..." << std::endl;

    ln = "G";
    WriteFile(hComm, ln.c_str(), ln.length(), &l, NULL);

    std::cout << "Done. Switching to listener mode." << std::endl << std::endl;

    char c;
    while (ReadFile(hComm, &c, 1, &l, NULL))
        std::cout << c;

    CloseHandle(hComm);

    return 0;
}
