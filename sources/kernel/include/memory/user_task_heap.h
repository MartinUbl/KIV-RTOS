#pragma once
#include <hal/intdef.h>
#include <process/process.h>
#include <memory/mmu.h>

struct TTask_Struct;

struct TUser_Task_Heap_Chunk_Header {
    TUser_Task_Heap_Chunk_Header* prev;
    TUser_Task_Heap_Chunk_Header* next;
    uint32_t size;
    bool is_free;
};

class CUser_Task_Heap_Manager {
    private:
        TUser_Task_Heap_Chunk_Header* mFirst;

        TUser_Task_Heap_Chunk_Header* Alloc_Next_Page(TTask_Struct* task);
        
    public:
        CUser_Task_Heap_Manager();
        void* Alloc(uint32_t size, TTask_Struct* task);
};