#include "alarms.h"
#include "ac_ir.h"

#include <flipper_format/flipper_format.h>
#include <stdio.h>
#include <string.h>

// v2 line: "hour minute daily enabled kind button temp preset|ac_name"
// v1 line: "hour minute daily enabled button ac_name" (kind = button press)

static bool ac_alarms_parse_v2(AcAlarm* alarm, const char* line) {
    unsigned hour, minute, daily, enabled, kind, button, temp;
    int rest_offset = 0;
    if(sscanf(
           line,
           "%u %u %u %u %u %u %u %n",
           &hour,
           &minute,
           &daily,
           &enabled,
           &kind,
           &button,
           &temp,
           &rest_offset) != 7)
        return false;
    const char* rest = line + rest_offset;
    const char* sep = strchr(rest, '|');
    if(!sep || sep[1] == '\0') return false;
    if(hour > 23 || minute > 59 || kind > AcAlarmKindPreset || button >= AC_BUTTON_COUNT)
        return false;

    alarm->hour = hour;
    alarm->minute = minute;
    alarm->daily = daily != 0;
    alarm->enabled = enabled != 0;
    alarm->kind = kind;
    alarm->button = button;
    alarm->temp = temp;
    size_t preset_len = sep - rest;
    if(preset_len >= sizeof(alarm->preset)) preset_len = sizeof(alarm->preset) - 1;
    memcpy(alarm->preset, rest, preset_len);
    alarm->preset[preset_len] = '\0';
    snprintf(alarm->ac_name, sizeof(alarm->ac_name), "%s", sep + 1);
    return true;
}

static bool ac_alarms_parse_v1(AcAlarm* alarm, const char* line) {
    unsigned hour, minute, daily, enabled, button;
    int name_offset = 0;
    if(sscanf(line, "%u %u %u %u %u %n", &hour, &minute, &daily, &enabled, &button, &name_offset) !=
       5)
        return false;
    const char* name = line + name_offset;
    if(hour > 23 || minute > 59 || button >= AC_BUTTON_COUNT || name[0] == '\0') return false;

    alarm->hour = hour;
    alarm->minute = minute;
    alarm->daily = daily != 0;
    alarm->enabled = enabled != 0;
    alarm->kind = AcAlarmKindButton;
    alarm->button = button;
    snprintf(alarm->ac_name, sizeof(alarm->ac_name), "%s", name);
    return true;
}

bool ac_alarms_load(AcAlarms* alarms, Storage* storage, const char* path) {
    memset(alarms, 0, sizeof(*alarms));
    FlipperFormat* ff = flipper_format_buffered_file_alloc(storage);
    FuriString* str = furi_string_alloc();
    FuriString* header = furi_string_alloc();
    bool success = false;

    do {
        if(!flipper_format_buffered_file_open_existing(ff, path)) break;
        uint32_t version = 0;
        if(!flipper_format_read_header(ff, header, &version)) break;
        if(!furi_string_equal(header, "FZ AC Alarms") || version < 1 || version > 2) break;

        while(alarms->count < AC_MAX_ALARMS && flipper_format_read_string(ff, "Alarm", str)) {
            AcAlarm alarm;
            memset(&alarm, 0, sizeof(alarm));
            bool ok = (version == 2) ? ac_alarms_parse_v2(&alarm, furi_string_get_cstr(str)) :
                                       ac_alarms_parse_v1(&alarm, furi_string_get_cstr(str));
            if(ok) alarms->items[alarms->count++] = alarm;
        }
        success = true;
    } while(false);

    furi_string_free(str);
    furi_string_free(header);
    flipper_format_free(ff);
    return success;
}

bool ac_alarms_save(const AcAlarms* alarms, Storage* storage, const char* path) {
    FlipperFormat* ff = flipper_format_file_alloc(storage);
    FuriString* str = furi_string_alloc();
    bool success = false;

    do {
        if(!flipper_format_file_open_always(ff, path)) break;
        if(!flipper_format_write_header_cstr(ff, "FZ AC Alarms", 2)) break;

        bool ok = true;
        for(uint32_t i = 0; i < alarms->count; i++) {
            const AcAlarm* alarm = &alarms->items[i];
            furi_string_printf(
                str,
                "%u %u %u %u %u %u %u %s|%s",
                alarm->hour,
                alarm->minute,
                alarm->daily ? 1 : 0,
                alarm->enabled ? 1 : 0,
                alarm->kind,
                alarm->button,
                alarm->temp,
                alarm->preset,
                alarm->ac_name);
            ok &= flipper_format_write_string(ff, "Alarm", str);
            if(!ok) break;
        }
        if(!ok) break;
        success = true;
    } while(false);

    furi_string_free(str);
    flipper_format_free(ff);
    return success;
}

void ac_alarms_remove(AcAlarms* alarms, uint32_t index) {
    if(index >= alarms->count) return;
    for(uint32_t i = index; i + 1 < alarms->count; i++) {
        alarms->items[i] = alarms->items[i + 1];
    }
    alarms->count--;
    memset(&alarms->items[alarms->count], 0, sizeof(AcAlarm));
}
