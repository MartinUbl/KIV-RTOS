#include <hal/intdef.h>

struct TUser_Task_Heap_Chunk_Header {
    TUser_Task_Heap_Chunk_Header* prev;
    TUser_Task_Heap_Chunk_Header* next;
    uint32_t size;
    bool is_free;
};

class CUser_Task_Heap_Manager {
    private:
        TUser_Task_Heap_Chunk_Header* mFirst;

        TUser_Task_Heap_Chunk_Header* Alloc_Next_Page();
        
    public:
        CUser_Task_Heap_Manager();
        void* Alloc(uint32_t size);
};