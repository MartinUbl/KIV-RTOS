#include <process/pipe.h>
#include <memory/kernel_heap.h>
#include <process/resource_manager.h>

CPipe::CPipe()
    : IFile(NFile_Type_Major::Pipe)
{
    //
}

CPipe::~CPipe()
{
    //
}

void CPipe::Reset(uint32_t size)
{
    if (mSem_Free)
    {
        delete mSem_Free;
        mSem_Free = nullptr;
    }

    if (mSem_Busy)
    {
        delete mSem_Busy;
        mSem_Busy = nullptr;
    }

    if (mBuffer)
    {
        sKernelMem.Free(mBuffer);
        mBuffer = nullptr;
    }

    // pokud jsme prave alokovani, alokujeme semaory a buffer
    if (size > 0)
    {
        mSem_Free = new CSemaphore;
        mSem_Free->Reset(size);

        mSem_Busy = new CSemaphore;
        mSem_Busy->Reset(size);
        mSem_Busy->Wait(size);

        mBuffer = reinterpret_cast<char*>(sKernelMem.Alloc(sizeof(char) * size));

        spinlock_init(&mBuffer_Lock);
    }
}

uint32_t CPipe::Read(char* buffer, uint32_t num)
{
    // pockame az bude tolik prostredku k dispozici
    mSem_Busy->Wait(num);

    // kriticka sekce
    spinlock_lock(&mBuffer_Lock);

    for (uint32_t i = 0; i < num; i++)
    {
        buffer[i] = mBuffer[mRead_Cur++];

        if (mRead_Cur >= mSem_Busy->Get_Max_Count())
            mRead_Cur = 0;
    }

    spinlock_unlock(&mBuffer_Lock);

    // notifikujeme producenty
    mSem_Free->Notify(num);

    return num;
}

uint32_t CPipe::Write(const char* buffer, uint32_t num)
{
    // pockame az bude tolik mista k dispozici
    mSem_Free->Wait(num);

    // kriticka sekce
    spinlock_lock(&mBuffer_Lock);

    for (uint32_t i = 0; i < num; i++)
    {
        mBuffer[mWrite_Cur++] = buffer[i];

        if (mWrite_Cur >= mSem_Busy->Get_Max_Count())
            mWrite_Cur = 0;
    }

    spinlock_unlock(&mBuffer_Lock);

    // notifikujeme konzumenty
    mSem_Busy->Notify(num);

    return num;
}

bool CPipe::Close()
{
    sProcess_Resource_Manager.Free_Pipe(this);

    return true;
}

bool CPipe::Wait(uint32_t count)
{
    return false;
}

uint32_t CPipe::Notify(uint32_t count)
{
    return 0;
}
