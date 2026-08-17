#pragma once

#include <fs/filesystem.h>
#include <process/resource_manager.h>
#include <stdstring.h>

class CBroadcast_FS_Driver : public IFilesystem_Driver {
    public:
        virtual void On_Register() override {
            //
        }

        virtual IFile *Open_File(const char *path, NFile_Open_Mode mode) override {
            char bcastname[Max_Broadcast_Channel_Name_Length];
            strncpy(bcastname, path, Max_Broadcast_Channel_Name_Length);

            const int len = strlen(bcastname);
            uint32_t bcastsize = Broadcast_Channel_Byte_Count_Unknown;
            for (int i = 1; i < len - 1; i++) {
                if (bcastname[i] == '#') {
                    bcastname[i] = '\0';
                    // explicitne receno, ze nevime, jak velky ma broadcast channel byt
                    if (bcastname[i + 1] == '?')
                        break;

                    bcastsize = atoi(&bcastname[i + 1]);
                    break;
                }
            }

            return sProcess_Resource_Manager.Alloc_Broadcast_Channel(bcastname, bcastsize);
        }
};

CBroadcast_FS_Driver fsBroadcast_FS_Driver;
