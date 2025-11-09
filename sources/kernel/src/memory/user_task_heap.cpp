#include <memory/user_task_heap.h>
#include <memory/pages.h>

CUser_Task_Heap_Manager::CUser_Task_Heap_Manager()
    : mFirst{nullptr}
{
    //
}

TUser_Task_Heap_Chunk_Header* CUser_Task_Heap_Manager::Alloc_Next_Page() {
    TUser_Task_Heap_Chunk_Header* chunk = reinterpret_cast<TUser_Task_Heap_Chunk_Header*>(sPage_Manager.Alloc_Page());
    chunk->prev = nullptr;
    chunk->next = nullptr;
    chunk->size = mem::PageSize - sizeof(TUser_Task_Heap_Chunk_Header);
    chunk->is_free = true;

    return chunk;
}

void* CUser_Task_Heap_Manager::Alloc(uint32_t size) {
    if (size > mem::PageSize - sizeof(TUser_Task_Heap_Chunk_Header)) {
        return nullptr;
    }
    
    if (!mFirst) {
        mFirst = Alloc_Next_Page();
    }

    TUser_Task_Heap_Chunk_Header* last = mFirst;
    TUser_Task_Heap_Chunk_Header* chunk = mFirst;

    while (chunk != nullptr && (!chunk->is_free || chunk->size < size)) {
        last = chunk;
        chunk = chunk->next;
    }

    if (!chunk) {
        last->next = Alloc_Next_Page();
        chunk = last->next;
    }

    if (chunk->size >= size && chunk->size <= size + sizeof(TUser_Task_Heap_Chunk_Header) + 1) {
        chunk->is_free = false;
        return reinterpret_cast<uint8_t*>(chunk) + sizeof(TUser_Task_Heap_Chunk_Header);
    }

    TUser_Task_Heap_Chunk_Header* hdr2 = reinterpret_cast<TUser_Task_Heap_Chunk_Header*>(reinterpret_cast<uint8_t*>(chunk) + sizeof(TUser_Task_Heap_Chunk_Header) + size);

    hdr2->size = chunk->size - size - sizeof(TUser_Task_Heap_Chunk_Header);
    hdr2->is_free = true;
    hdr2->prev = chunk;
    hdr2->next = chunk->next;

    chunk->size = size;
    chunk->is_free = false;
    chunk->next = hdr2;

    return reinterpret_cast<uint8_t*>(chunk) + sizeof(TUser_Task_Heap_Chunk_Header);
}
