#include <stdmutex.h>
#include <stdfile.h>
#include <stdstring.h>


int main(int argc, char** argv)
{
    // otevřeme mutex 0
    uint32_t m = mutex_create("test");

    while (true)
    {
        mutex_lock(m);          // zde proběhne volání telemetry_increment_mutex()
        //for (volatile int i = 0; i < 100000; i++);
        mutex_unlock(m);

        //sleep(0x10);
    }

    close(m);
    return 0;
}
