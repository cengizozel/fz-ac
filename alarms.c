#include "alarms.h"
#include "ac_ir.h"

#include <flipper_format/flipper_format.h>
#include <stdio.h>
#include <string.h>

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
        if(!furi_string_equal(header, "FZ AC Alarms") || (version != 1)) break;

        while(alarms->count < AC_MAX_ALARMS && flipper_format_read_string(ff, "Alarm", str)) {
            unsigned hour, minute, daily, enabled, button;
            int name_offset = 0;
            if(sscanf(
                   furi_string_get_cstr(str),
                   "%u %u %u %u %u %n",
                   &hour,
                   &minute,
                   &daily,
                   &enabled,
                   &button,
                   &name_offset) != 5)
                continue;
            const char* name = furi_string_get_cstr(str) + name_offset;
            if(hour > 23 || minute > 59 || button >= AC_BUTTON_COUNT || name[0] == '\0') continue;
            AcAlarm* alarm = &alarms->items[alarms->count++];
            alarm->hour = hour;
            alarm->minute = minute;
            alarm->daily = daily != 0;
            alarm->enabled = enabled != 0;
            alarm->button = button;
            snprintf(alarm->ac_name, sizeof(alarm->ac_name), "%s", name);
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
        if(!flipper_format_write_header_cstr(ff, "FZ AC Alarms", 1)) break;

        bool ok = true;
        for(uint32_t i = 0; i < alarms->count; i++) {
            const AcAlarm* alarm = &alarms->items[i];
            furi_string_printf(
                str,
                "%u %u %u %u %u %s",
                alarm->hour,
                alarm->minute,
                alarm->daily ? 1 : 0,
                alarm->enabled ? 1 : 0,
                alarm->button,
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
