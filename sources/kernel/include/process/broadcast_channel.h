#pragma once

#include <fs/filesystem.h>
#include "mutex.h"

constexpr uint32_t Max_Broadcast_Readers = 16;

class CBroadcast_Channel : public IFile
{
private:
    struct TReader_Info
    {
        uint32_t pid;
        uint32_t last_read_generation;
    };

    char *mBuffer;
    uint32_t mSize;
    uint32_t mMessage_Length;
    uint32_t mGeneration;

    TReader_Info mReaders[Max_Broadcast_Readers];
    uint32_t mReader_Count;

    CMutex mMutex;

public:
    CBroadcast_Channel();
    ~CBroadcast_Channel();

    void Reset(uint32_t size);

    virtual uint32_t Read(char *buffer, uint32_t len) override;
    virtual uint32_t Write(const char *buffer, uint32_t len) override;
    virtual bool Close() override;
};
