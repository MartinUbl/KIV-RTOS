#include <memory/user_task_heap.h>
#include <memory/kernel_heap.h>
#include <memory/pages.h>
#include <memory/memmap.h>
#include <process/process.h>
#include <drivers/uart.h>
#include <stdstring.h>

CUser_Task_Heap_Manager::CUser_Task_Heap_Manager()
    : first_page{nullptr}
{
    //
}

void* CUser_Task_Heap_Manager::Allocate_New_Page(TTask_Struct* task) {
    void* page_data = reinterpret_cast<void*>(sPage_Manager.Alloc_Page());

    unsigned long ttbr0_phys_address = task->cpu_context.ttbr0 & ~0x00003FFF;
    unsigned long ttbr0_address = ttbr0_phys_address + mem::MemoryVirtualBase;
    uint32_t phys_addr_page_start = reinterpret_cast<uint32_t>(page_data) - mem::MemoryVirtualBase;
    map_memory(reinterpret_cast<uint32_t*>(ttbr0_address), phys_addr_page_start, task->heap_next);

    return page_data;
}

void* CUser_Task_Heap_Manager::Alloc(uint32_t size, TTask_Struct* task) {
    if (size > mem::PageSize) {
        return nullptr;
    }
    

    if (first_page == nullptr) {
        first_page = sKernelMem.Alloc<TPage_List_Node>();
        first_page->used_space = 0;
        first_page->virt_user_addr = task->heap_base;
        first_page->page_start = Allocate_New_Page(task);
        first_page->next = nullptr;
        task->heap_next += mem::PageSize;
    }

    TPage_List_Node* current_page = first_page;
    while (current_page->next != nullptr && mem::PageSize - current_page->used_space < size) {
        current_page = current_page->next;
    }

    if (mem::PageSize - current_page->used_space < size) {
        TPage_List_Node* new_page = sKernelMem.Alloc<TPage_List_Node>();
        new_page->used_space = 0;
        new_page->virt_user_addr = task->heap_next;
        new_page->page_start = Allocate_New_Page(task);
        new_page->next = nullptr;
        task->heap_next += mem::PageSize;

        current_page->next = new_page;
        current_page = new_page;
    }
    
    uint32_t alloc_addr = current_page->virt_user_addr + current_page->used_space;
    current_page->used_space += size;
    return reinterpret_cast<void*>(alloc_addr);
}
