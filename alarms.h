#pragma once

#include <furi.h>
#include <storage/storage.h>

#define AC_MAX_ALARMS     16
#define AC_ALARM_NAME_LEN 25

typedef struct {
    bool enabled;
    bool daily;
    uint8_t hour;
    uint8_t minute;
    uint8_t button;
    char ac_name[AC_ALARM_NAME_LEN];
    uint32_t fired_stamp;
} AcAlarm;

typedef struct {
    AcAlarm items[AC_MAX_ALARMS];
    uint32_t count;
} AcAlarms;

bool ac_alarms_load(AcAlarms* alarms, Storage* storage, const char* path);
bool ac_alarms_save(const AcAlarms* alarms, Storage* storage, const char* path);
void ac_alarms_remove(AcAlarms* alarms, uint32_t index);
