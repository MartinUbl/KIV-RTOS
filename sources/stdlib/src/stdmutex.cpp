#include <stdmutex.h>

mutex_t mutex_create(const char* name)
{
    mutex_t mtx;
    const NMutex_Operation op = NMutex_Operation::Create;

    asm volatile("mov r0, %0" : : "r" (op));
    asm volatile("mov r1, %0" : : "r" (name));
    asm volatile("swi 69");
    asm volatile("mov %0, r0" : "=r" (mtx));

    return mtx;
}

bool mutex_lock(mutex_t mtx)
{
    NSWI_Result_Code res;
    const NMutex_Operation op = NMutex_Operation::Lock;

    asm volatile("mov r0, %0" : : "r" (op));
    asm volatile("mov r1, %0" : : "r" (mtx));
    asm volatile("swi 69");
    asm volatile("mov %0, r0" : "=r" (res));

    return (res == NSWI_Result_Code::OK);
}

bool mutex_unlock(mutex_t mtx)
{
    NSWI_Result_Code res;
    const NMutex_Operation op = NMutex_Operation::Unlock;

    asm volatile("mov r0, %0" : : "r" (op));
    asm volatile("mov r1, %0" : : "r" (mtx));
    asm volatile("swi 69");
    asm volatile("mov %0, r0" : "=r" (res));

    return (res == NSWI_Result_Code::OK);
}

void mutex_destroy(mutex_t mtx)
{
    NSWI_Result_Code res;
    const NMutex_Operation op = NMutex_Operation::Destroy;

    asm volatile("mov r0, %0" : : "r" (op));
    asm volatile("mov r1, %0" : : "r" (mtx));
    asm volatile("swi 69");
    asm volatile("mov %0, r0" : "=r" (res));

    //return (res == NSWI_Result_Code::OK);
}