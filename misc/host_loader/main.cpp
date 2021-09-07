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

        std::string str((std::istreambuf_iterator<char>(ifile)),
                         std::istreambuf_iterator<char>());
        ifile.seekg(0, std::ios::beg);

        int linecnt = 0;
        int linecounter = 0;
        int lineinc = 0;
        for (int j = 0; j < str.size(); j++)
        {
            if (str[j] == '\n')
                linecnt++;
        }

        std::cout << "Line count: " << linecnt << std::endl;

        std::cout << "Uploading..." << std::endl;

        while (std::getline(ifile, ln))
        {
            linecounter++;
            WriteFile(hComm, ln.c_str(), static_cast<DWORD>(ln.length()), &l, NULL);

            if (lineinc < ((100 * linecounter) / linecnt))
            {
                lineinc = ((100 * linecounter) / linecnt);
                std::cout << "Progress: " << lineinc << " %" << std::endl;
            }

            //Sleep(10);
        }
    }

    std::cout << "Launching..." << std::endl;

    ln = "G";
    WriteFile(hComm, ln.c_str(), static_cast<DWORD>(ln.length()), &l, NULL);

    std::cout << "Done. Switching to listener mode." << std::endl << std::endl;

    char c;
    while (ReadFile(hComm, &c, 1, &l, NULL))
        std::cout << c;

    CloseHandle(hComm);

    return 0;
}
