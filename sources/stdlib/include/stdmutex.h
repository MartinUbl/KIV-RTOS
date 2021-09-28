#pragma once

#include <hal/intdef.h>
#include <process/swi.h>
#include <fs/filesystem.h>

using mutex_t = uint32_t;

mutex_t mutex_create(const char* name);
bool mutex_lock(mutex_t mtx);
bool mutex_unlock(mutex_t mtx);
void mutex_destroy(mutex_t mtx);
