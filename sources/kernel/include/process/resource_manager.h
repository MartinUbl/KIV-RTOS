#pragma once

#include <hal/intdef.h>

#include "mutex.h"

// pocet predalokovanych mutexu (a zaroven max. pocet)
constexpr uint32_t Mutex_Count = 10;

// maximalni delka jmena mutexu
constexpr uint32_t Max_Mutex_Name_Length = 16;

class CProcess_Resource_Manager
{
    private:
        struct TMutex_Record
        {
            CMutex mtx;
            char name[Max_Mutex_Name_Length];
            unsigned int alloc_count;
        };

        TMutex_Record mMutexes[Mutex_Count];

    public:
        CProcess_Resource_Manager();
        ~CProcess_Resource_Manager();

        CMutex* Alloc_Mutex(const char* name);
        void Free_Mutex(CMutex* mtx);
};

extern CProcess_Resource_Manager sProcess_Resource_Manager;
