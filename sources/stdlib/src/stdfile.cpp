#include <stdfile.h>
#include <stdstring.h>

uint32_t getpid()
{
    uint32_t pid;

    asm volatile("swi 0");
    asm volatile("mov %0, r0" : "=r" (pid));

    return pid;
}

void terminate(int exitcode)
{
    asm volatile("mov r0, %0" : : "r" (exitcode));
    asm volatile("swi 1");
}

void sched_yield()
{
    asm volatile("swi 2");
}

uint32_t open(const char* filename, NFile_Open_Mode mode)
{
    uint32_t file;

    asm volatile("mov r0, %0" : : "r" (filename));
    asm volatile("mov r1, %0" : : "r" (mode));
    asm volatile("swi 64");
    asm volatile("mov %0, r0" : "=r" (file));

    return file;
}

uint32_t read(uint32_t file, char* const buffer, uint32_t size)
{
    uint32_t rdnum;

    asm volatile("mov r0, %0" : : "r" (file));
    asm volatile("mov r1, %0" : : "r" (buffer));
    asm volatile("mov r2, %0" : : "r" (size));
    asm volatile("swi 65");
    asm volatile("mov %0, r0" : "=r" (rdnum));

    return rdnum;
}

uint32_t write(uint32_t file, const char* buffer, uint32_t size)
{
    uint32_t wrnum;

    asm volatile("mov r0, %0" : : "r" (file));
    asm volatile("mov r1, %0" : : "r" (buffer));
    asm volatile("mov r2, %0" : : "r" (size));
    asm volatile("swi 66");
    asm volatile("mov %0, r0" : "=r" (wrnum));

    return wrnum;
}

void close(uint32_t file)
{
    asm volatile("mov r0, %0" : : "r" (file));
    asm volatile("swi 67");
}

uint32_t ioctl(uint32_t file, NIOCtl_Operation operation, void* param)
{
    uint32_t retcode;

    asm volatile("mov r0, %0" : : "r" (file));
    asm volatile("mov r1, %0" : : "r" (operation));
    asm volatile("mov r2, %0" : : "r" (param));
    asm volatile("swi 68");
    asm volatile("mov %0, r0" : "=r" (retcode));

    return retcode;
}

uint32_t notify(uint32_t file, uint32_t count)
{
    uint32_t retcnt;

    asm volatile("mov r0, %0" : : "r" (file));
    asm volatile("mov r1, %0" : : "r" (count));
    asm volatile("swi 69");
    asm volatile("mov %0, r0" : "=r" (retcnt));

    return retcnt;
}

NSWI_Result_Code wait(uint32_t file, uint32_t count, uint32_t notified_deadline)
{
    NSWI_Result_Code retcode;

    asm volatile("mov r0, %0" : : "r" (file));
    asm volatile("mov r1, %0" : : "r" (count));
    asm volatile("mov r2, %0" : : "r" (notified_deadline));
    asm volatile("swi 70");
    asm volatile("mov %0, r0" : "=r" (retcode));

    return retcode;
}

bool sleep(uint32_t ticks, uint32_t notified_deadline)
{
    uint32_t retcode;

    asm volatile("mov r0, %0" : : "r" (ticks));
    asm volatile("mov r1, %0" : : "r" (notified_deadline));
    asm volatile("swi 3");
    asm volatile("mov %0, r0" : "=r" (retcode));

    return retcode;
}

uint32_t get_active_process_count()
{
    const NGet_Sched_Info_Type req = NGet_Sched_Info_Type::Active_Process_Count;
    uint32_t retval;

    asm volatile("mov r0, %0" : : "r" (req));
    asm volatile("mov r1, %0" : : "r" (&retval));
    asm volatile("swi 4");

    return retval;
}

uint32_t get_tick_count()
{
    const NGet_Sched_Info_Type req = NGet_Sched_Info_Type::Tick_Count;
    uint32_t retval;

    asm volatile("mov r0, %0" : : "r" (req));
    asm volatile("mov r1, %0" : : "r" (&retval));
    asm volatile("swi 4");

    return retval;
}

void set_task_deadline(uint32_t tick_count_required)
{
    const NDeadline_Subservice req = NDeadline_Subservice::Set_Relative;

    asm volatile("mov r0, %0" : : "r" (req));
    asm volatile("mov r1, %0" : : "r" (&tick_count_required));
    asm volatile("swi 5");
}

uint32_t get_task_ticks_to_deadline()
{
    const NDeadline_Subservice req = NDeadline_Subservice::Get_Remaining;
    uint32_t ticks;

    asm volatile("mov r0, %0" : : "r" (req));
    asm volatile("mov r1, %0" : : "r" (&ticks));
    asm volatile("swi 5");

    return ticks;
}

const uint32_t min_alloc_size = 128;
uint32_t used = 0;
uint32_t allocated = 0;
uint32_t data = 0;

// ...existing code...
void* malloc(uint32_t size, uint32_t uart) {
    char buf[64];

    /* large single allocation path - use single extended asm to avoid reordering */
    if (allocated < size && min_alloc_size < size) {
        write(uart, "Allocating large block of memory!\n", 34);
        void* ptr;
        asm volatile(
            "mov r0, %1\n"
            "swi 6\n"
            "mov %0, r0\n"
            : "=r"(ptr)
            : "r"(size)
            : "memory"
        );

        /* print ptr and return */
        itoa((uint32_t)ptr, buf, 16);
        write(uart, "DEBUG large ptr = ", 18);
        write(uart, buf, strlen(buf));
        write(uart, "\n", 1);
        return ptr;
    }

    if (used + size > allocated) {
        write(uart, "Allocating additional block of memory!\n", 39);
        void* ptr;

        /* request new block deterministically */
        asm volatile(
            "mov r0, %1\n"
            "swi 6\n"
            "mov %0, r0\n"
            : "=r"(ptr)
            : "r"(min_alloc_size)
            : "memory"
        );
        if (ptr == nullptr) {
            return nullptr;
        }

        /* ensure compiler doesn't keep stale cached values */
        asm volatile("" ::: "memory");

        /* check extend vs new block */
        if ((uint32_t)ptr - min_alloc_size == data) {
            write(uart, "Extending existing memory block!\n", 33);
            allocated += min_alloc_size;
        } else {
            /* store new base and barrier, then print stored value */
            data = (uint32_t)ptr;
            asm volatile("" ::: "memory"); /* ensure store completes before reads */

            itoa((uint32_t)ptr, buf, 16);
            write(uart, "DEBUG: ptr (stored)    = ", 25);
            write(uart, buf, strlen(buf));
            write(uart, "\n", 1);

            itoa(data, buf, 16);
            write(uart, "DEBUG: data (after)    = ", 25);
            write(uart, buf, strlen(buf));
            write(uart, "\n", 1);

            allocated = min_alloc_size;
            used = 0;
        }
    }

    itoa(used, buf, 16);
    write(uart, "Used = ", 7);
    write(uart, buf, strlen(buf));
    write(uart, "\n", 1);

    uint32_t addr = data + used;
    itoa(addr, buf, 16);
    write(uart, "Returning address: ", 19);
    write(uart, buf, strlen(buf));
    write(uart, "\n", 1);
    used += size;
    return (void*)addr;
}

const char Pipe_File_Prefix[] = "SYS:pipe/";

uint32_t pipe(const char* name, uint32_t buf_size)
{
    char fname[64];
    int name_len = strlen(name);
    strncpy(fname, Pipe_File_Prefix, sizeof(Pipe_File_Prefix));
    strncpy(fname + sizeof(Pipe_File_Prefix) - 1, name, name_len);
    
    int ncur = sizeof(Pipe_File_Prefix) + name_len - 1;

    fname[ncur++] = '#';

    itoa(buf_size, &fname[ncur], 10);

    return open(fname, NFile_Open_Mode::Read_Write);
}
