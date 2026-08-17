#pragma once

#include <drivers/uart.h>
#include <hal/peripherals.h>
#include <memory/kernel_heap.h>
#include <fs/filesystem.h>
#include <stdstring.h>

// virtualni UART soubor
class CUART_File final : public IFile {
    private:
        // UART kanal
        int mChannel;

    public:
        CUART_File(int channel)
            : IFile(NFile_Type_Major::Character), mChannel(channel) {
            //
        }

        ~CUART_File() {
        }

        virtual uint32_t Read(char* buffer, uint32_t num) override {
            if (num > 0 && buffer != nullptr) {
                if (mChannel == 0) {
                    return sUART0.Read_Or_Wait(buffer, num, this);
                }
            }

            return 0;
        }

        virtual bool Wait(uint32_t count) override {
            if (mChannel == 0) {
                Wait_Enqueue_Current();
                sUART0.Wait_For_Event(this);

                // zablokujeme, probudi nas az notify
                sProcessMgr.Block_Current_Process();
                return true;
            }

            return false;
        }

        virtual uint32_t Write(const char* buffer, uint32_t num) override {
            if (num > 0 && buffer != nullptr) {
                if (mChannel == 0) {
                    sUART0.Write(buffer, num);
                    return num;
                }
            }

            return 0;
        }

        virtual bool Close() override {
            if (mChannel < 0) {
                return false;
            }

            if (mChannel == 0) {
                sUART0.Close();
            }
            mChannel = -1;

            return IFile::Close();
        }

        virtual bool IOCtl(NIOCtl_Operation dir, void* ctlptr) override {
            // proces chce ziskat parametry - naformatujeme mu je do jim dodane struktury (v jeho adr. prostoru)
            if (dir == NIOCtl_Operation::Get_Params) {
                TUART_IOCtl_Params* params = reinterpret_cast<TUART_IOCtl_Params*>(ctlptr);
                if (mChannel == 0) {
                    params->baud_rate = sUART0.Get_Baud_Rate();
                    params->char_length = sUART0.Get_Char_Length();
                    params->blocking_state = sUART0.Get_Blocking_State();
                    return true;
                }
            }
            // proces chce nastavit parametry
            else if (dir == NIOCtl_Operation::Set_Params) {
                TUART_IOCtl_Params* params = reinterpret_cast<TUART_IOCtl_Params*>(ctlptr);
                if (mChannel == 0) {
                    sUART0.Set_Baud_Rate(params->baud_rate);
                    sUART0.Set_Char_Length(params->char_length);
                    sUART0.Set_Blocking_State(params->blocking_state);
                    return true;
                }
            }
            return false;
        }
};

class CUART_FS_Driver : public IFilesystem_Driver {
    public:
        virtual void On_Register() override {
            //
        }

        virtual IFile* Open_File(const char* path, NFile_Open_Mode mode) override {
            // jedina slozka path - kanal UARTu

            int channel = atoi(path);
            if (channel != 0) { // mame jen jeden kanal
                return nullptr;
            }

            if (!sUART0.Open()) {
                return nullptr;
            }

            CUART_File* f = new CUART_File(channel);

            return f;
        }
};

CUART_FS_Driver fsUART_FS_Driver;
