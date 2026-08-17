#include <process/broadcast_channel.h>
#include <memory/kernel_heap.h>
#include <process/process_manager.h>
#include <process/resource_manager.h>

#include "stdfile.h"

CBroadcast_Channel::CBroadcast_Channel()
    : IFile(NFile_Type_Major::Broadcast) {
    //
}

CBroadcast_Channel::~CBroadcast_Channel() {
    if (mBuffer) {
        sKernelMem.Free(mBuffer);
    }
}

void CBroadcast_Channel::Reset(uint32_t size) {
    if (!mBuffer && size > 0) {
        mBuffer = reinterpret_cast<char*>(sKernelMem.Alloc(size));
    } else if (mBuffer && size == 0) {
        sKernelMem.Free(mBuffer);
        mBuffer = nullptr;
    } else if (size != mSize) {
        if (mBuffer) {
            sKernelMem.Free(mBuffer);
        }

        mSize = size;
        if (mSize > 0) {
            mBuffer = reinterpret_cast<char*>(sKernelMem.Alloc(mSize));
        }
    }

    mMessage_Length = 0;
    mGeneration = Broadcast_No_Message;
    mReader_Count = 0;
}

uint32_t CBroadcast_Channel::Read(char *buffer, uint32_t len) {
    uint32_t current_pid = sProcessMgr.Get_Current_Process()->pid;
    TReader_Info* reader_info = nullptr;

    mMutex.Lock();

    // Najdeme zaznam pro tento proces, nebo vytvorime novy
    for (uint32_t i = 0; i < mReader_Count; i++) {
        if (mReaders[i].pid == current_pid) {
            reader_info = &mReaders[i];
            break;
        }
    }

    if (!reader_info && mReader_Count < Max_Broadcast_Readers) {
        reader_info = &mReaders[mReader_Count++];
        reader_info->pid = current_pid;
        reader_info->last_read_generation = Broadcast_No_Message; // Nikdy nic necetl
    }

    // Pokud jsme nenasli/nevytvorili zaznam, je chyba (malo mista)
    if (!reader_info) {
        mMutex.Unlock();
        return 0;
    }

    // Cekame, dokud neni k dispozici nova generace zpravy
    while (mGeneration == Broadcast_No_Message || reader_info->last_read_generation == mGeneration) {
        Wait_Enqueue_Current();
        mMutex.Unlock();
        sProcessMgr.Block_Current_Process();
        mMutex.Lock();
    }

    // Precteme zpravu
    uint32_t read_len = (mMessage_Length < len) ? mMessage_Length : len;
    for (uint32_t i = 0; i < read_len; i++) {
        buffer[i] = mBuffer[i];
    }

    // Aktualizujeme generaci, kterou jsme cetli
    reader_info->last_read_generation = mGeneration;

    mMutex.Unlock();

    return read_len;
}

uint32_t CBroadcast_Channel::Write(const char *buffer, uint32_t len) {
    mMutex.Lock();

    // Zapiseme zpravu (prepiseme starou)
    uint32_t write_len = (len < mSize) ? len : mSize;
    for (uint32_t i = 0; i < write_len; i++) {
        mBuffer[i] = buffer[i];
    }
    mMessage_Length = write_len;

    // Zvysime generaci
    mGeneration++;
    if (mGeneration == Broadcast_No_Message) { // preteceni, 0 je specialni hodnota "zadna zprava"
        mGeneration = 1;
    }

    // Probudime vsechny cekajici ctenare
    Notify(NotifyAll);

    mMutex.Unlock();

    return write_len;
}

bool CBroadcast_Channel::Close() {
    mMutex.Lock();

    // Odstranime zaznam o ctenari pro tento proces
    uint32_t current_pid = sProcessMgr.Get_Current_Process()->pid;
    for (uint32_t i = 0; i < mReader_Count; i++) {
        if (mReaders[i].pid == current_pid) {
            // Presuneme posledni prvek na misto mazaneho (aby pole zustalo spojite)
            if (i != mReader_Count - 1) {
                mReaders[i] = mReaders[mReader_Count - 1];
            }
            mReader_Count--;
            break;
        }
    }

    mMutex.Unlock();

    // Uvolnime zdroj ve spravci zdroju (snizi pocitadlo referenci)
    sProcess_Resource_Manager.Free_Broadcast_Channel(this);

    return true;
}

bool CBroadcast_Channel::Wait(uint32_t count) {
    uint32_t current_pid = sProcessMgr.Get_Current_Process()->pid;
    TReader_Info* reader_info = nullptr;

    mMutex.Lock();

    // Najdeme zaznam pro tento proces, nebo vytvorime novy
    for (uint32_t i = 0; i < mReader_Count; i++) {
        if (mReaders[i].pid == current_pid) {
            reader_info = &mReaders[i];
            break;
        }
    }

    if (!reader_info && mReader_Count < Max_Broadcast_Readers) {
        reader_info = &mReaders[mReader_Count++];
        reader_info->pid = current_pid;
        reader_info->last_read_generation = Broadcast_No_Message; // Nikdy nic necetl
    }

    // Pokud jsme nenasli/nevytvorili zaznam, je chyba (malo mista)
    if (!reader_info) {
        mMutex.Unlock();
        return false;
    }

    // Cekame, dokud neni k dispozici nova generace zpravy
    while (mGeneration == Broadcast_No_Message || reader_info->last_read_generation == mGeneration) {
        Wait_Enqueue_Current();
        mMutex.Unlock();
        sProcessMgr.Block_Current_Process();
    }

    return true;
}

bool CBroadcast_Channel::Try_Wait_All_Reserve(uint32_t count) {

    // nic zatim nebylo zapsano = neni co cist a je nutne cekat
    if (mGeneration == Broadcast_No_Message) {
        return false;
    }

    uint32_t current_pid = sProcessMgr.Get_Current_Process()->pid;
    TReader_Info* reader_info = nullptr;

    mMutex.Lock();

    // Najdeme zaznam pro tento proces, nebo vytvorime novy
    for (uint32_t i = 0; i < mReader_Count; i++) {
        if (mReaders[i].pid == current_pid) {
            reader_info = &mReaders[i];
            break;
        }
    }

    if (!reader_info && mReader_Count < Max_Broadcast_Readers) {
        reader_info = &mReaders[mReader_Count++];
        reader_info->pid = current_pid;
        reader_info->last_read_generation = Broadcast_No_Message; // Nikdy nic necetl
    }

    // Pokud jsme nenasli/nevytvorili zaznam, jde o chybu; pokud jsme posledni zpravu uz precetli, bude nutne cekat
    if (!reader_info || reader_info->last_read_generation == mGeneration) {
        mMutex.Unlock();
        return false;
    }

    mMutex.Unlock();
    return true;
}
