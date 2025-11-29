#pragma once

#include <hal/intdef.h>
#include <drivers/uart.h>               // pro tisk reportu - případně uprav podle API
#include <process/process_manager.h>    // pro aktuální proces / stav
//#include <cstdio>                       // sprintf (pokud není k dispozici, uprav)
//#include <cstring>
#include <stdstring.h>

#ifdef ENABLE_TELEMETRY

inline uint32_t sTelemetryStateMinuteChanged = 0;
//uint32_t tick_accumulator;   // ← NOVÉ


// Inicializace telemetry, zavolat z main/startup
void telemetry_init();

// Tento callback volíme z timer IRQ (musí být volán z timer IRQ handleru)
// Telemetry počítá ticky a přepočítává sekundy a minuty dle TELEMETRY_TICKS_PER_SEC
void telemetry_on_timer_tick();

struct telemetry_snapshot {
    uint32_t syscalls_per_min;
    uint32_t interrupts_per_min;
    uint32_t mutex_locks_per_min;
    uint32_t scheduler_load_percent;
    uint32_t tick_accumulator;
};


void telemetry_get_snapshot(telemetry_snapshot* out);
// Inkrementace událostí (volat tam, kde k nim dochází)
void telemetry_increment_syscall();
void telemetry_increment_interrupt();
void telemetry_increment_mutex();
char* telemetry_generate_message();
void telemetry_generate_message(char* buffer, unsigned int buffer_size,
                                unsigned int sum_syscalls_last_min,
                                unsigned int sum_interrupts_last_min,
                                unsigned int sum_mutex_last_min,
                                float load_percent);


// vynulovat statistiky
void telemetry_reset();
uint32_t telemetry_get_tick_accumulator();

#else

static inline void telemetry_init() {}
static inline void telemetry_on_timer_tick() {}
static inline void telemetry_increment_syscall() {}
static inline void telemetry_increment_interrupt() {}
static inline void telemetry_increment_mutex() {}
static inline void telemetry_reset() {}
static inline void telemetry_get_snapshot() { (void)0; }


#endif // ENABLE_TELEMETRY
