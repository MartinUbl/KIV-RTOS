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

enum class NProcFS_PID_Type
{
    PID,            // PID tasku
    STATE,          // stav tasku (runnable, blocked, ...)
    FD_N,           // pocet otevrenych souboru
    FD,             // file descriptory otevrenych souboru
    STATUS,         // souhrn vsech statistik
    PAGE,           // pocet alokovanych stranek
};

// virtualni soubor pro proces s PIDem
class CProcFS_PID_File final : public IFile
{
    public:
        CProcFS_PID_File(int pid, NProcFS_PID_Type type) : IFile(NFile_Type_Major::Character), _pid(pid), _type(type) 
        {
            //
        }

        ~CProcFS_PID_File()
        {
            Close();
        }

        virtual uint32_t Read(char* buffer, uint32_t num) override
        {
            TTask_Struct *task = sProcessMgr.Get_Process_By_PID(_pid);
            if (!task) return 0;

            // prevod statu do citelne formy
            const char *state_str;
            if (_type == NProcFS_PID_Type::STATE || _type == NProcFS_PID_Type::STATUS) 
            {
                switch (task->state)
                {
                    case NTask_State::New:
                        state_str = "new"; break;
                    case NTask_State::Runnable:
                    case NTask_State::Running:
                        state_str = "runnable"; break;
                    case NTask_State::Interruptable_Sleep:
                        state_str = "sleep"; break;
                    case NTask_State::Blocked:
                        state_str = "blocked"; break;
                    case NTask_State::Zombie:
                        state_str = "zombie"; break;
                    default:
                        state_str = "unknown"; break;
                }
            }

            // pocet otevrenych souboru
            uint8_t f = 0;
            char fd_buffer[Max_Process_Opened_Files * 3];
            bzero(fd_buffer, Max_Process_Opened_Files * 3);
            if (_type == NProcFS_PID_Type::STATUS || _type == NProcFS_PID_Type::FD_N || _type == NProcFS_PID_Type::FD)
            {
                for (int i = 0; i < Max_Process_Opened_Files; i++) {
                    if (task->opened_files[i] != nullptr) {
                        f++;
                        itoa(i, fd_buffer + strlen(fd_buffer), 10);
                        strcat(fd_buffer, " ");
                    }
                }
            }

            char buf[Max_ProcFS_File_Len];
            bzero(buf, Max_ProcFS_File_Len);

            switch (_type)
            {
                case NProcFS_PID_Type::PID:
                    itoa(_pid, buf, 10);
                    break;
                case NProcFS_PID_Type::STATUS:
                    strcat(buf, "PID: ");
                    itoa(_pid, buf + strlen(buf), 10);
                    strcat(buf, "\r\nstate: ");
                    strcat(buf, state_str);
                    strcat(buf, "\r\nopended files: ");
                    itoa(f, buf + strlen(buf), 10);
                    strcat(buf, "\r\npage count: ");
                    itoa(task->page_count, buf + strlen(buf), 10);
                    break;
                case NProcFS_PID_Type::STATE:
                    strcat(buf, state_str);
                    break;
                case NProcFS_PID_Type::FD:
                    strcat(buf, fd_buffer);
                    break;
                case NProcFS_PID_Type::FD_N:
                    itoa(f, buf, 10);
                    break;
                case NProcFS_PID_Type::PAGE:
                    itoa(task->page_count, buf, 10);
                    break;
            }

            int len_s = strlen(buf); // celkova delka dat
            int len = len_s < num ? len_s : num; // delka dat k zapsani do bufferu - min(len_s, num)
            bzero(buffer, num);
            strncpy(buffer, buf, len);

            return len;
        }

    private:
        int _pid;
        NProcFS_PID_Type _type;
};

enum class NProcFS_Status_Type
{
    SCHED = 0,                  // pocty procesu v ruznych stavech
    TASKS,                      // celkovy pocet tasku
    TICKS,                      // pocet tiku od startu
    FD_N,                       // celkovy pocet otevrenych souboru
    PAGE,                       // celkovy pocet alokovanych stranek
};

// virtualni nePIDový soubor
class CProcFS_Status_File final : public IFile
{
    public:

        CProcFS_Status_File(NProcFS_Status_Type type) : IFile(NFile_Type_Major::Character), _type(type) 
        {
            //
        }

        ~CProcFS_Status_File()
        {
            Close();
        }

        virtual uint32_t Read(char* buffer, uint32_t num) override
        {
            char buf[Max_ProcFS_File_Len];
            bzero(buf, Max_ProcFS_File_Len);

            // ziskani poctu bezicich, blokovanych atd. procesu
            CProcess_Summary_Info info;
            sProcessMgr.Get_Scheduler_Info(NGet_Sched_Info_Type::Process_Summary, &info);

            switch (_type)
            {
                case NProcFS_Status_Type::SCHED:
                    strcat(buf, "runnable: ");
                    itoa(info.running, buf + strlen(buf), 10);
                    strcat(buf, "\r\nblocked: ");
                    itoa(info.blocked, buf + strlen(buf), 10);
                    strcat(buf, "\r\nzombie: ");
                    itoa(info.zombie, buf + strlen(buf), 10);
                    break;
                case NProcFS_Status_Type::TASKS:
                    itoa(info.total, buf, 10);
                    break;
                case NProcFS_Status_Type::TICKS:
                    uint32_t ticks;
                    sProcessMgr.Get_Scheduler_Info(NGet_Sched_Info_Type::Tick_Count, &ticks);
                    itoa(ticks, buf, 10);
                    break;
                case NProcFS_Status_Type::FD_N:
                    itoa(sProcessMgr.Get_File_Count(), buf, 10);
                    break;
                case NProcFS_Status_Type::PAGE:
                    itoa(sProcessMgr.Get_Page_Count(), buf, 10);
                    break;
            };

            int len_s = strlen(buf); // celkova delka dat
            int len = len_s < num ? len_s : num; // delka dat k zapsani do bufferu - min(len_s, num)
            bzero(buffer, num);
            strncpy(buffer, buf, len);

            return len;
        }
    private:
        NProcFS_Status_Type _type;
};

// driver Proc FS (PROC:)
class CProc_FS_Driver : public IFilesystem_Driver
{
	public:
        virtual void On_Register() override 
        {
            //
        };

        virtual IFile* Open_File(const char* path, NFile_Open_Mode mode) override
        {
            if (mode != NFile_Open_Mode::Read_Only)
                return nullptr;

            // validace PIDu (cislo)
            bool is_pid = true;
            char *s = const_cast<char*>(path);
            while (*s && (*s) != '/')
            {
                if (*s < '0' || *s > '9') is_pid = false;
                s++;
            }
            
            bool self = strncmp(path, "self", 4) == 0; // self je taky PID, jen aktualniho procesu
            if (is_pid || self )
            {
                *s++ = '\0'; // ukonceni pid retezce (misto '/') pro atoi()

                // resolve pid + task
                uint32_t pid = 0;
                TTask_Struct *task = nullptr;
                if (self)
                {
                    task = sProcessMgr.Get_Current_Process();
                    if (!task) return nullptr;
                    pid = task->pid;
                }
                else
                {
                    pid = atoi(path);
                    // validace PIDu
                    task = sProcessMgr.Get_Process_By_PID(pid);
                    if (!task) return nullptr;
                }

                if (strncmp(s, "pid", MaxFilenameLength) == 0) return new CProcFS_PID_File(pid, NProcFS_PID_Type::PID);
                if (strncmp(s, "fd_n", MaxFilenameLength) == 0) return new CProcFS_PID_File(pid, NProcFS_PID_Type::FD_N);
                if (strncmp(s, "fd", MaxFilenameLength) == 0) return new CProcFS_PID_File(pid, NProcFS_PID_Type::FD);
                if (strncmp(s, "status", MaxFilenameLength) == 0) return new CProcFS_PID_File(pid, NProcFS_PID_Type::STATUS);
                if (strncmp(s, "state", MaxFilenameLength) == 0) return new CProcFS_PID_File(pid, NProcFS_PID_Type::STATE);
                if (strncmp(s, "page", MaxFilenameLength) == 0) return new CProcFS_PID_File(pid, NProcFS_PID_Type::PAGE);
            }
            else
            {
                if (strncmp(path, "sched", MaxFilenameLength) == 0) return new CProcFS_Status_File(NProcFS_Status_Type::SCHED);
                if (strncmp(path, "tasks", MaxFilenameLength) == 0) return new CProcFS_Status_File(NProcFS_Status_Type::TASKS);
                if (strncmp(path, "ticks", MaxFilenameLength) == 0) return new CProcFS_Status_File(NProcFS_Status_Type::TICKS);
                if (strncmp(path, "fd_n", MaxFilenameLength) == 0) return new CProcFS_Status_File(NProcFS_Status_Type::FD_N);
                if (strncmp(path, "page", MaxFilenameLength) == 0) return new CProcFS_Status_File(NProcFS_Status_Type::PAGE);
            }

            return nullptr;
        }
};

CProc_FS_Driver fsProc_FS_Driver;
