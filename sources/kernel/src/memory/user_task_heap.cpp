#include <memory/user_task_heap.h>
#include <memory/pages.h>
#include <process/process.h>
#include <drivers/uart.h>
#include <stdstring.h>

CUser_Task_Heap_Manager::CUser_Task_Heap_Manager()
    : mFirst{nullptr}
{
    //
}

TUser_Task_Heap_Chunk_Header* CUser_Task_Heap_Manager::Alloc_Next_Page(TTask_Struct* task) {
    TUser_Task_Heap_Chunk_Header* chunk = reinterpret_cast<TUser_Task_Heap_Chunk_Header*>(sPage_Manager.Alloc_Page());

    chunk->prev = nullptr;
    chunk->next = nullptr;
    chunk->size = mem::PageSize - sizeof(TUser_Task_Heap_Chunk_Header);
    chunk->is_free = true;

    uint32_t user_va = task->heap_next;
    uint32_t phys = (uint32_t)chunk;

    map_memory((uint32_t*)task->cpu_context.ttbr0, phys, user_va);

    task->heap_next += mem::PageSize;

    char buf[64];
    itoa((unsigned int)chunk, buf, 16);
    sUART0.Write("Chunk address: 0x", 17);
    sUART0.Write(buf);
    sUART0.Write("\n", 1);
    // physical address
    itoa((unsigned int)phys, buf, 16);
    sUART0.Write("Phys: 0x", 8);
    sUART0.Write(buf);
    sUART0.Write(", ", 2);

    // virtual address
    itoa((unsigned int)user_va, buf, 16);
    sUART0.Write("VA: 0x", 6);
    sUART0.Write(buf);
    sUART0.Write(", ", 2);

    // chunk size
    itoa((unsigned int)chunk->size, buf, 10);
    sUART0.Write("size: ", 6);
    sUART0.Write(buf);
    sUART0.Write("\n", 1);
    return reinterpret_cast<TUser_Task_Heap_Chunk_Header*>(user_va);
}

void* CUser_Task_Heap_Manager::Alloc(uint32_t size, TTask_Struct* task) {
    if (size > mem::PageSize - sizeof(TUser_Task_Heap_Chunk_Header)) {
        return nullptr;
    }
    
    if (!mFirst) {
        mFirst = Alloc_Next_Page(task);
    }

    TUser_Task_Heap_Chunk_Header* last = mFirst;
    TUser_Task_Heap_Chunk_Header* chunk = mFirst;

    while (chunk != nullptr && (!chunk->is_free || chunk->size < size)) {
        last = chunk;
        chunk = chunk->next;
    }

    if (!chunk) {
        last->next = Alloc_Next_Page(task);
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
