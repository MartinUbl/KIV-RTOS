#pragma once
#include <hal/intdef.h>
#include <process/process.h>
#include <memory/mmu.h>

struct TTask_Struct;

struct TPage_List_Node {
    uint32_t used_space;
    uint32_t virt_user_addr;
    void* page_start;
    TPage_List_Node* next;
};

class CUser_Task_Heap_Manager {
    private:
        TPage_List_Node* first_page;
        void* Allocate_New_Page(TTask_Struct* task);
        
    public:
        CUser_Task_Heap_Manager();
        void* Alloc(uint32_t size, TTask_Struct* task);
};