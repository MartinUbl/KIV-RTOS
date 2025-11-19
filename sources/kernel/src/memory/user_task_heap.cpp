#include <memory/user_task_heap.h>
#include <memory/pages.h>
#include <memory/memmap.h>
#include <process/process.h>
#include <drivers/uart.h>
#include <stdstring.h>

CUser_Task_Heap_Manager::CUser_Task_Heap_Manager()
    : current_page{nullptr}
{
    //
}

TCurrent_Page* CUser_Task_Heap_Manager::Allocate_New_Page(TTask_Struct* task) {
    TCurrent_Page* new_page = reinterpret_cast<TCurrent_Page*>(sPage_Manager.Alloc_Page());
    new_page->used_space = sizeof(TCurrent_Page);
    new_page->virt_user_addr = task->heap_next;
    task->heap_next += mem::PageSize;

    unsigned long ttbr0_phys_address = task->cpu_context.ttbr0 & ~0x00003FFF;
    unsigned long ttbr0_address = ttbr0_phys_address + mem::MemoryVirtualBase;
    uint32_t phys_addr_page_start = reinterpret_cast<uint32_t>(new_page) - mem::MemoryVirtualBase;
    map_memory(reinterpret_cast<uint32_t*>(ttbr0_address), phys_addr_page_start, new_page->virt_user_addr);

    return new_page;
}

void* CUser_Task_Heap_Manager::Alloc(uint32_t size, TTask_Struct* task) {
    if (size > mem::PageSize - sizeof(TCurrent_Page)) {
        return nullptr;
    }
    

    if (current_page == nullptr) {
        current_page = Allocate_New_Page(task);
    }

    if (mem::PageSize - current_page->used_space < size) {
        current_page = Allocate_New_Page(task);
    }
    
    uint32_t alloc_addr = current_page->virt_user_addr + current_page->used_space;
    current_page->used_space += size;
    return reinterpret_cast<void*>(alloc_addr);
}
