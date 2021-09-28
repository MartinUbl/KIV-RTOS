#pragma once

#include <hal/peripherals.h>
#include <fs/filesystem.h>

#include "spinlock.h"

class CMutex : public IFile
{
    private:
        // uzel fronty cekajicich procesu
        struct TProcess_Queue_Node
        {
            unsigned int pid;
            TProcess_Queue_Node* prev;
            TProcess_Queue_Node* next;
        };

        // kdo aktualne drzi mutex?
        unsigned int mHolder_PID = 0;

        // fronta cekajicich procesu
        TProcess_Queue_Node* mWaiting_Processes;

        // stav zamku - odemceny
        spinlock_t mLock_State = Lock_Unlocked;

    public:
        CMutex();

        // zamykani mutexu - muze blokovat proces, pokud zrovna neni mutex volny
        // vraci true kdyz se povede zamknout, false kdyz uz mutex je zamceny procesem, ktery o to zada
        bool Lock();

        // pokusi se zamknout mutex - nikdy neblokuje
        // vraci true kdyz se povede zamknout, false kdyz ne, nebo je mutex jiz zamceny procesem, ktery o to zada
        bool Try_Lock();

        // odemkne mutex - nikdy neblokuje
        // vraci true pokud se povede odemknout, false pokud je mutex jiz odemceny nebo je vlastneny jinym procesem
        bool Unlock();

        // rozhrani IFile - jen pro konformitu s rozhranim souboru
        
        virtual uint32_t Read(char* buffer, uint32_t num) override { return 0; };
        virtual uint32_t Write(const char* buffer, uint32_t num) override { return 0; };
        virtual bool Close() override { return false; };
        virtual bool IOCtl(NIOCtl_Operation dir, void* ctlptr) override { return false; };
};
