#include <process/resource_manager.h>
#include <stdstring.h>

CProcess_Resource_Manager sProcess_Resource_Manager;

CProcess_Resource_Manager::CProcess_Resource_Manager()
{
    for (uint32_t i = 0; i < Mutex_Count; i++)
    {
        mMutexes[i].name[0] = '\0';
        mMutexes[i].alloc_count = 0;
    }
}

CProcess_Resource_Manager::~CProcess_Resource_Manager()
{
    //
}

CMutex* CProcess_Resource_Manager::Alloc_Mutex(const char* name)
{
    for (uint32_t i = 0; i < Mutex_Count; i++)
    {
        if (mMutexes[i].alloc_count > 0)
        {
            //if (strncmp(mMutexes[i].name, name, Max_Mutex_Name_Length) == 0)
            {
                mMutexes[i].alloc_count++;
                return &mMutexes[i].mtx;
            }
        }
    }

    for (uint32_t i = 0; i < Mutex_Count; i++)
    {
        if (mMutexes[i].alloc_count == 0)
        {
            strncpy(mMutexes[i].name, name, Max_Mutex_Name_Length);
            mMutexes[i].alloc_count++;
            return &mMutexes[i].mtx;
        }
    }

    return nullptr;
}

void CProcess_Resource_Manager::Free_Mutex(CMutex* mtx)
{
    for (uint32_t i = 0; i < Mutex_Count; i++)
    {
        if (&mMutexes[i].mtx == mtx && mMutexes[i].alloc_count > 0)
        {
            mMutexes[i].alloc_count--;
            return;
        }
    }
}