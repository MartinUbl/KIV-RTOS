#pragma once

#include <fs/filesystem.h>
#include <memory/kernel_heap.h>
#include <stdstring.h>
#include "telemetry.h"

// --- souborové rozhraní pro telemetrii ---
class CTelemetry_File final : public IFile
{
public:
    CTelemetry_File()
        : IFile(NFile_Type_Major::Character) // důležité: inicializace base class
    {
    }

    ~CTelemetry_File()
    {
        Close();
    }

    virtual uint32_t Read(char* buffer, uint32_t num) override
    {
        if (!buffer || num == 0)
            return 0;

        telemetry_snapshot snapshot;
        telemetry_get_snapshot(&snapshot);

        char tmp[16];
        char* out = buffer;
        uint32_t written = 0;

        auto write_str = [&](const char* s) {
            while (*s && written < num) {
                *out++ = *s++;
                written++;
            }
        };

        auto write_num = [&](uint32_t value) {
            itoa(value, tmp, 10);
            write_str(tmp);
        };
        //kontrolní výpis
        /*
        write_str("Ticks/sec: ");
        write_num(snapshot.tick_accumulator);
        write_str("\r\n");
        */
        write_str("Syscls/min: ");
        write_num(snapshot.syscalls_per_min);
        write_str("\r\nInt/min: ");
        write_num(snapshot.interrupts_per_min);
        write_str("\r\nMtxLck/min: ");
        write_num(snapshot.mutex_locks_per_min);
        write_str("\r\nScheduler: ");
        write_num(snapshot.scheduler_load_percent);
        write_str(" %\r\n");


        return written;
    }

    virtual uint32_t Write(const char*, uint32_t) override
    {
        // telemetrii nelze zapisovat
        return 0;
    }

    virtual bool Close() override
    {
        return IFile::Close();
    }

    virtual bool IOCtl(NIOCtl_Operation, void*) override
    {
        return false;
    }
};

// --- FS driver, který vytváří CTelemetry_File ---
class CTelemetry_FS_Driver : public IFilesystem_Driver
{
public:
    virtual void On_Register() override
    {
        // nic zvláštního
    }

    virtual IFile* Open_File(const char* path, NFile_Open_Mode mode) override
    {
        // telemetrie je pouze pro čtení
        if (mode != NFile_Open_Mode::Read_Only)
            return nullptr;

        // subcesta musí být prázdná
        if (path && strlen(path) > 0)
            return nullptr;

        return new CTelemetry_File();
    }
};

// globální instance driveru
CTelemetry_FS_Driver fsTelemetry_FS_Driver;
