#pragma once
#include <hal/intdef.h>
#include <process/process.h>
#include <memory/mmu.h>

struct TTask_Struct;

struct TCurrent_Page {
    uint32_t used_space;
    uint32_t virt_user_addr;
};

class CUser_Task_Heap_Manager {
    private:
        TCurrent_Page* current_page;
        TCurrent_Page* Allocate_New_Page(TTask_Struct* task);
        
    public:
        CUser_Task_Heap_Manager();
        void* Alloc(uint32_t size, TTask_Struct* task);
};