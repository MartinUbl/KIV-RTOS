

#ifdef ENABLE_TELEMETRY
#include "telemetry.h"


// konfigurovatelně z CMake (definováno jako -DTELEMETRY_TICKS_PER_SEC=...)
//10000
//1

#ifndef TELEMETRY_TICKS_PER_SEC
#define TELEMETRY_TICKS_PER_SEC 1000
#endif


// interní stavy 7250
//static volatile uint32_t telemetry_ticks_per_sec = 10000;
static volatile uint32_t g_syscall_tick = 0;
static volatile uint32_t g_interrupt_tick = 0;
static volatile uint32_t g_mutex_tick = 0;
static volatile uint32_t g_last_sec_ticks = 0;

// kruh pro poslednich 60 sekund
static uint32_t g_syscalls_per_sec[60];
static uint32_t g_interrupts_per_sec[60];
static uint32_t g_mutex_per_sec[60];

static uint32_t g_current_sec_index = 0;
static uint32_t g_tick_accumulator = 0; // kolik timer IRQ v tomto sekundě

// scheduler load - počítám ticky kdy je scheduler aktivní (běží proces) vs. kdy je "spánek"
static uint32_t g_active_ticks_history[60];
static uint32_t g_sleep_ticks_history[60];
static uint32_t g_active_ticks_acc = 0;
static uint32_t g_sleep_ticks_acc = 0;

// pomocné: pro minutu agregace
static uint32_t sum_syscalls_last_min = 0;
static uint32_t sum_interrupts_last_min = 0;
static uint32_t sum_mutex_last_min = 0;
static uint32_t sum_active_ticks_last_min = 0;
static uint32_t sum_sleep_ticks_last_min = 0;


// kvůli bezpečnosti inicializace
static bool g_initialized = false;

void telemetry_init()
{
    if (g_initialized) return;
    g_initialized = true;

    telemetry_reset();

}



// Vrátí aktuální snapshot pro FS driver
void telemetry_get_snapshot(telemetry_snapshot* out)
{
    if (!out) return;

    out->syscalls_per_min = sum_syscalls_last_min;
    out->interrupts_per_min = sum_interrupts_last_min;
    out->mutex_locks_per_min = sum_mutex_last_min;
    out->tick_accumulator = g_last_sec_ticks;

    uint32_t total = sum_active_ticks_last_min + sum_sleep_ticks_last_min;
    if (total == 0)
        out->scheduler_load_percent = 0;
    else
        out->scheduler_load_percent =
            (sum_active_ticks_last_min * 100) / total;
}

// Tento callback musí být volán z timer IRQ (např. v CTimer::IRQ_Callback())
void telemetry_on_timer_tick()
{




    if (!g_initialized) return;

    // zvyš tick akumulátoru (kolik timer IRQ v této sekundě)
    g_tick_accumulator++;

    // zjistíme, zda je aktuálně naplánovaný proces v Running stavu
    // pokud ano, považujeme tick za "active", jinak jako "sleep"
    auto* cur = sProcessMgr.Get_Current_Process();
    if (cur && cur->state == NTask_State::Running) {
        g_active_ticks_acc++;
    } else {
        g_sleep_ticks_acc++;
    }

    // Předpoklad: TELEMETRY_TICKS_PER_SEC je počet timer IRQ za jednu sekundu
    if (g_tick_accumulator >= TELEMETRY_TICKS_PER_SEC) {
        // uložit hodnoty do aktuální sekundové buňky
        // nejprve vyjmeme hodnoty, které tam byly (když rotujeme, musíme odečíst staré z miny)
        uint32_t old_sys = g_syscalls_per_sec[g_current_sec_index];
        uint32_t old_int = g_interrupts_per_sec[g_current_sec_index];
        uint32_t old_mutex = g_mutex_per_sec[g_current_sec_index];
        uint32_t old_active = g_active_ticks_history[g_current_sec_index];
        uint32_t old_sleep = g_sleep_ticks_history[g_current_sec_index];

        // uložíme nové hodnoty (na sekundu)
        g_syscalls_per_sec[g_current_sec_index] = g_syscall_tick;
        g_interrupts_per_sec[g_current_sec_index] = g_interrupt_tick;
        g_mutex_per_sec[g_current_sec_index] = g_mutex_tick;
        g_active_ticks_history[g_current_sec_index] = g_active_ticks_acc;
        g_sleep_ticks_history[g_current_sec_index] = g_sleep_ticks_acc;

        // upravíme součty poslední minuty
        sum_syscalls_last_min += g_syscall_tick;
        sum_syscalls_last_min -= old_sys;

        sum_interrupts_last_min += g_interrupt_tick;
        sum_interrupts_last_min -= old_int;

        sum_mutex_last_min += g_mutex_tick;
        sum_mutex_last_min -= old_mutex;

        sum_active_ticks_last_min += g_active_ticks_acc;
        sum_active_ticks_last_min -= old_active;

        sum_sleep_ticks_last_min += g_sleep_ticks_acc;
        sum_sleep_ticks_last_min -= old_sleep;

        // resetujeme per-second akumulátory
        g_syscall_tick = 0;
        g_interrupt_tick = 0;
        g_mutex_tick = 0;
        g_active_ticks_acc = 0;
        g_sleep_ticks_acc = 0;

        // přesun indexu
        g_current_sec_index++;
        if (g_current_sec_index >= 60)
        {
            g_current_sec_index = 0;

        //g_tick_accumulator = 0;

        //if (g_current_sec_index == 0)
        //{
            //telemetry_reset();

            sTelemetryStateMinuteChanged = 1;
        }
        g_last_sec_ticks = g_tick_accumulator;
        g_tick_accumulator = 0;


    }
}



void telemetry_increment_syscall()
{
    if (!g_initialized) return;
    g_syscall_tick++;
}

void telemetry_increment_interrupt()
{
    if (!g_initialized) return;
    g_interrupt_tick++;
}

void telemetry_increment_mutex()
{
    if (!g_initialized) return;
    g_mutex_tick++;

}



void telemetry_reset()
{

    memset(g_syscalls_per_sec, 0, sizeof(g_syscalls_per_sec));
    memset(g_interrupts_per_sec, 0, sizeof(g_interrupts_per_sec));
    memset(g_mutex_per_sec, 0, sizeof(g_mutex_per_sec));
    memset(g_active_ticks_history, 0, sizeof(g_active_ticks_history));
    memset(g_sleep_ticks_history, 0, sizeof(g_sleep_ticks_history));

    g_syscall_tick = g_interrupt_tick = g_mutex_tick = 0;
    g_tick_accumulator = 0;
    g_current_sec_index = 0;

    sum_syscalls_last_min = sum_interrupts_last_min = sum_mutex_last_min = 0;
    sum_active_ticks_last_min = sum_sleep_ticks_last_min = 0;

}

uint32_t telemetry_get_tick_accumulator()
{
    return g_tick_accumulator;
}





#endif // ENABLE_TELEMETRY
