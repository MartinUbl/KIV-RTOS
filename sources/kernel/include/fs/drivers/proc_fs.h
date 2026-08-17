#pragma once

#include <drivers/gpio.h>
#include <hal/peripherals.h>
#include <memory/kernel_heap.h>
#include <fs/filesystem.h>
#include <stdstring.h>
#include <process/process_manager.h>
#include <memory/memmap.h>
#include <memory/mmu.h>

constexpr uint32_t Max_ProcFS_File_Len = 64;

// virtualni soubor pro proces s PIDem (marker)
class CProcFS_Task_File : public IFile {
    protected:
        int mPID;

        const char* Status_To_String(NTask_State state) {
            switch (state) {
                case NTask_State::New: return "new";
                case NTask_State::Runnable:
                case NTask_State::Running: return "runnable";
                case NTask_State::Interruptable_Sleep: return "sleep";
                case NTask_State::Blocked: return "blocked";
                case NTask_State::Zombie: return "zombie";
                default: return "unknown";
            }
        }

    public:
        CProcFS_Task_File(int pid) : IFile(NFile_Type_Major::Character), mPID(pid) {
            //
        }
};

// virtualni nePIDovy soubor (marker trida)
class CProcFS_Global_File : public IFile {
    public:
        CProcFS_Global_File() : IFile(NFile_Type_Major::Character) {
            //
        }
};

class CProcFS_Task_File__PID : public CProcFS_Task_File {
    public:
        CProcFS_Task_File__PID(int pid) : CProcFS_Task_File(pid) {}

        virtual uint32_t Read(char* buffer, uint32_t num) override {
            itoa(mPID, buffer, 10);
            return strlen(buffer);
        }

        static CProcFS_Task_File* Create(int pid) {
            return new CProcFS_Task_File__PID(pid);
        }
};

class CProcFS_Task_File__Status : public CProcFS_Task_File {
    public:
        CProcFS_Task_File__Status(int pid) : CProcFS_Task_File(pid) {}

        virtual uint32_t Read(char* buffer, uint32_t num) override {
            TTask_Struct *task = sProcessMgr.Get_Process_By_PID(mPID);
            if (!task) {
                return 0;
            }

            const char *state_str = Status_To_String(task->state);

            strncat(buffer, state_str, num);
            return strlen(buffer);
        }

        static CProcFS_Task_File* Create(int pid) {
            return new CProcFS_Task_File__Status(pid);
        }
};

class CProcFS_Task_File__FD_Count : public CProcFS_Task_File {
    public:
        CProcFS_Task_File__FD_Count(int pid) : CProcFS_Task_File(pid) {}

        virtual uint32_t Read(char* buffer, uint32_t num) override {
            TTask_Struct *task = sProcessMgr.Get_Process_By_PID(mPID);
            if (!task) {
                return 0;
            }

            uint8_t f = 0;
            for (int i = 0; i < Max_Process_Opened_Files; i++) {
                if (task->opened_files[i] != nullptr) {
                    f++;
                }
            }

            itoa(f, buffer, 10);
            return strlen(buffer);
        }

        static CProcFS_Task_File* Create(int pid) {
            return new CProcFS_Task_File__FD_Count(pid);
        }
};

class CProcFS_Task_File__FD : public CProcFS_Task_File {
    public:
        CProcFS_Task_File__FD(int pid) : CProcFS_Task_File(pid) {}

        virtual uint32_t Read(char* buffer, uint32_t num) override {
            TTask_Struct *task = sProcessMgr.Get_Process_By_PID(mPID);
            if (!task) {
                return 0;
            }

            char fd_buffer[Max_Process_Opened_Files * 3];
            bzero(fd_buffer, Max_Process_Opened_Files * 3);
            for (int i = 0; i < Max_Process_Opened_Files; i++) {
                if (task->opened_files[i] != nullptr) {
                    itoa(i, fd_buffer + strlen(fd_buffer), 10);
                    strcat(fd_buffer, " ");
                }
            }

            strncat(buffer, fd_buffer, num);
            return strlen(buffer);
        }

        static CProcFS_Task_File* Create(int pid) {
            return new CProcFS_Task_File__FD(pid);
        }
};

class CProcFS_Task_File__Summary : public CProcFS_Task_File {
    public:
        CProcFS_Task_File__Summary(int pid) : CProcFS_Task_File(pid) {}

        virtual uint32_t Read(char* buffer, uint32_t num) override {
            TTask_Struct *task = sProcessMgr.Get_Process_By_PID(mPID);
            if (!task) {
                return 0;
            }

            const char *state_str = Status_To_String(task->state);

            uint8_t f = 0;
            for (int i = 0; i < Max_Process_Opened_Files; i++) {
                if (task->opened_files[i] != nullptr) {
                    f++;
                }
            }

            strncat(buffer, "PID: ", num);
            itoa(mPID, buffer + strlen(buffer), 10);
            strncat(buffer, "\r\nstatus: ", num);
            strncat(buffer, state_str, num);
            strncat(buffer, "\r\nopened files: ", num);
            itoa(f, buffer + strlen(buffer), 10);
            strncat(buffer, "\r\npage count: ", num);
            itoa(task->page_count, buffer + strlen(buffer), 10);

            return strlen(buffer);
        }

        static CProcFS_Task_File* Create(int pid) {
            return new CProcFS_Task_File__Summary(pid);
        }
};

class CProcFS_Task_File__Page_Count : public CProcFS_Task_File {
    public:
        CProcFS_Task_File__Page_Count(int pid) : CProcFS_Task_File(pid) {}

        virtual uint32_t Read(char* buffer, uint32_t num) override {
            TTask_Struct *task = sProcessMgr.Get_Process_By_PID(mPID);
            if (!task) {
                return 0;
            }

            itoa(task->page_count, buffer, 10);
            return strlen(buffer);
        }

        static CProcFS_Task_File* Create(int pid) {
            return new CProcFS_Task_File__Page_Count(pid);
        }
};

class CProcFS_Global_File__Scheduler final : public CProcFS_Global_File {
    public:
        virtual uint32_t Read(char* buffer, uint32_t num) override {
            CProcess_Summary_Info info;
            sProcessMgr.Get_Scheduler_Info(NGet_Sched_Info_Type::Process_Summary, &info);

            strncat(buffer, "runnable: ", num);
            itoa(info.running, buffer + strlen(buffer), 10);
            strncat(buffer, "\r\nblocked: ", num);
            itoa(info.blocked, buffer + strlen(buffer), 10);
            strncat(buffer, "\r\nzombie: ", num);
            itoa(info.zombie, buffer + strlen(buffer), 10);

            return strlen(buffer);
        }

        static CProcFS_Global_File* Create() {
            return new CProcFS_Global_File__Scheduler();
        }
};

class CProcFS_Global_File__Tasks final : public CProcFS_Global_File {
    public:
        virtual uint32_t Read(char* buffer, uint32_t num) override {
            CProcess_Summary_Info info;
            sProcessMgr.Get_Scheduler_Info(NGet_Sched_Info_Type::Process_Summary, &info);

            itoa(info.total, buffer, 10);

            return strlen(buffer);
        }

        static CProcFS_Global_File* Create() {
            return new CProcFS_Global_File__Tasks();
        }
};

class CProcFS_Global_File__Ticks final : public CProcFS_Global_File {
    public:
        virtual uint32_t Read(char* buffer, uint32_t num) override {
            uint32_t ticks;
            sProcessMgr.Get_Scheduler_Info(NGet_Sched_Info_Type::Tick_Count, &ticks);
            itoa(ticks, buffer, 10);

            return strlen(buffer);
        }

        static CProcFS_Global_File* Create() {
            return new CProcFS_Global_File__Ticks();
        }
};

class CProcFS_Global_File__FD_Count final : public CProcFS_Global_File {
    public:
        virtual uint32_t Read(char* buffer, uint32_t num) override {
            itoa(sProcessMgr.Get_File_Count(), buffer, 10);
            return strlen(buffer);
        }

        static CProcFS_Global_File* Create() {
            return new CProcFS_Global_File__FD_Count();
        }
};

class CProcFS_Global_File__Page_Count final : public CProcFS_Global_File {
    public:
        virtual uint32_t Read(char* buffer, uint32_t num) override {
            itoa(sProcessMgr.Get_Page_Count(), buffer, 10);

            return strlen(buffer);
        }

        static CProcFS_Global_File* Create() {
            return new CProcFS_Global_File__Page_Count();
        }
};

struct TProcFS_Task_File_Entry {
    const char* name;
    CProcFS_Task_File* (*create_func)(int pid);
};

TProcFS_Task_File_Entry ProcFS_Task_Files[] = {
    { "pid", CProcFS_Task_File__PID::Create },
    { "status", CProcFS_Task_File__Status::Create },
    { "fd_count", CProcFS_Task_File__FD_Count::Create },
    { "fd", CProcFS_Task_File__FD::Create },
    { "summary", CProcFS_Task_File__Summary::Create },
    { "page", CProcFS_Task_File__Page_Count::Create }
};

struct TProcFS_Global_File_Entry {
    const char* name;
    CProcFS_Global_File* (*create_func)();
};

TProcFS_Global_File_Entry ProcFS_Global_Files[] = {
    { "scheduler", CProcFS_Global_File__Scheduler::Create },
    { "tasks", CProcFS_Global_File__Tasks::Create },
    { "ticks", CProcFS_Global_File__Ticks::Create },
    { "fd_count", CProcFS_Global_File__FD_Count::Create },
    { "page_count", CProcFS_Global_File__Page_Count::Create }
};

// driver Proc FS (PROC:)
class CProc_FS_Driver : public IFilesystem_Driver {
    public:
        virtual void On_Register() override {
            //
        };

        virtual IFile* Open_File(const char* path, NFile_Open_Mode mode) override
        {
            if (mode != NFile_Open_Mode::Read_Only) {
                return nullptr;
            }

            // validace PIDu (cislo)
            bool is_pid = true;
            const char *s = path;
            while (*s && (*s) != '/') {
                if (*s < '0' || *s > '9') is_pid = false;
                s++;
            }

            if (*s == '/') {
                s++; // preskocime lomitko, pokud je
            }

            bool self = strncmp(path, "self", 4) == 0; // self je taky PID, jen aktualniho procesu
            if (is_pid || self) {
                // resolve pid + task
                uint32_t pid = 0;
                if (self) {
                    TTask_Struct *task = sProcessMgr.Get_Current_Process();
                    if (!task) {
                        return nullptr;
                    }
                    pid = task->pid;
                }
                else {
                    pid = atoi(path);
                    // validace PIDu
                    if (!sProcessMgr.Get_Process_By_PID(pid)) {
                        return nullptr;
                    }
                }

                for (int i = 0; i < sizeof(ProcFS_Task_Files) / sizeof(ProcFS_Task_Files[0]); i++) {
                    if (strncmp(s, ProcFS_Task_Files[i].name, MaxFilenameLength) == 0) {
                        return ProcFS_Task_Files[i].create_func(pid);
                    }
                }
            }
            else
            {
                for (int i = 0; i < sizeof(ProcFS_Global_Files) / sizeof(ProcFS_Global_Files[0]); i++) {
                    if (strncmp(path, ProcFS_Global_Files[i].name, MaxFilenameLength) == 0) {
                        return ProcFS_Global_Files[i].create_func();
                    }
                }
            }

            return nullptr;
        }
};

CProc_FS_Driver fsProc_FS_Driver;
