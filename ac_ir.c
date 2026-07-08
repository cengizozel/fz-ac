#include "ac_ir.h"

#include <flipper_format/flipper_format.h>
#include <infrared_transmit.h>
#include <string.h>

const char* const ac_button_names[AC_BUTTON_COUNT] = {
    "Off",
    "Mode",
    "Temp_up",
    "Temp_dn",
    "Fan",
    "Vane",
};

const char* const ac_button_labels[AC_BUTTON_COUNT] = {
    "OFF",
    "MODE",
    "TEMP +",
    "TEMP -",
    "FAN",
    "VANE",
};

void ac_ir_signal_reset(AcIrSignal* signal) {
    if(signal->timings) {
        free(signal->timings);
    }
    memset(signal, 0, sizeof(*signal));
}

void ac_ir_signal_from_worker(AcIrSignal* signal, const InfraredWorkerSignal* worker_signal) {
    ac_ir_signal_reset(signal);
    if(infrared_worker_signal_is_decoded(worker_signal)) {
        signal->is_raw = false;
        signal->message = *infrared_worker_get_decoded_signal(worker_signal);
    } else {
        const uint32_t* timings;
        size_t timings_size;
        infrared_worker_get_raw_signal(worker_signal, &timings, &timings_size);
        if(timings_size == 0 || timings_size > AC_MAX_RAW_TIMINGS) return;
        signal->is_raw = true;
        signal->timings = malloc(timings_size * sizeof(uint32_t));
        memcpy(signal->timings, timings, timings_size * sizeof(uint32_t));
        signal->timings_size = timings_size;
        signal->frequency = INFRARED_COMMON_CARRIER_FREQUENCY;
        signal->duty_cycle = INFRARED_COMMON_DUTY_CYCLE;
    }
    signal->present = true;
}

void ac_ir_signal_move(AcIrSignal* dst, AcIrSignal* src) {
    ac_ir_signal_reset(dst);
    *dst = *src;
    memset(src, 0, sizeof(*src));
}

void ac_ir_signal_describe(const AcIrSignal* signal, FuriString* out) {
    if(!signal->present) {
        furi_string_set(out, "none");
    } else if(signal->is_raw) {
        furi_string_printf(out, "RAW signal, %u samples", (unsigned)signal->timings_size);
    } else {
        furi_string_printf(
            out,
            "%s  A:%02lX C:%02lX",
            infrared_get_protocol_name(signal->message.protocol),
            signal->message.address,
            signal->message.command);
    }
}

void ac_ir_signal_send(const AcIrSignal* signal) {
    if(!signal->present) return;
    if(signal->is_raw) {
        infrared_send_raw_ext(
            signal->timings,
            signal->timings_size,
            true,
            signal->frequency,
            signal->duty_cycle);
    } else {
        infrared_send(&signal->message, 2);
    }
}

void ac_remote_reset(AcRemote* remote) {
    for(size_t i = 0; i < AC_BUTTON_COUNT; i++) {
        ac_ir_signal_reset(&remote->signals[i]);
    }
}

bool ac_remote_load(AcRemote* remote, Storage* storage, const char* path) {
    ac_remote_reset(remote);
    FlipperFormat* ff = flipper_format_buffered_file_alloc(storage);
    FuriString* str = furi_string_alloc();
    FuriString* header = furi_string_alloc();
    bool success = false;

    do {
        if(!flipper_format_buffered_file_open_existing(ff, path)) break;
        uint32_t version = 0;
        if(!flipper_format_read_header(ff, header, &version)) break;
        if(!furi_string_equal(header, "IR signals file") || (version != 1)) break;

        while(flipper_format_read_string(ff, "name", str)) {
            int32_t index = -1;
            for(size_t i = 0; i < AC_BUTTON_COUNT; i++) {
                if(furi_string_equal(str, ac_button_names[i])) {
                    index = i;
                    break;
                }
            }

            AcIrSignal tmp;
            memset(&tmp, 0, sizeof(tmp));
            if(!flipper_format_read_string(ff, "type", str)) break;

            bool ok = false;
            if(furi_string_equal(str, "parsed")) {
                do {
                    if(!flipper_format_read_string(ff, "protocol", str)) break;
                    tmp.message.protocol =
                        infrared_get_protocol_by_name(furi_string_get_cstr(str));
                    if(!infrared_is_protocol_valid(tmp.message.protocol)) break;
                    uint32_t address, command;
                    if(!flipper_format_read_hex(ff, "address", (uint8_t*)&address, 4)) break;
                    if(!flipper_format_read_hex(ff, "command", (uint8_t*)&command, 4)) break;
                    tmp.message.address = address;
                    tmp.message.command = command;
                    tmp.message.repeat = false;
                    tmp.is_raw = false;
                    ok = true;
                } while(false);
            } else if(furi_string_equal(str, "raw")) {
                do {
                    uint32_t count = 0;
                    if(!flipper_format_read_uint32(ff, "frequency", &tmp.frequency, 1)) break;
                    if(!flipper_format_read_float(ff, "duty_cycle", &tmp.duty_cycle, 1)) break;
                    if(!flipper_format_get_value_count(ff, "data", &count)) break;
                    if(count == 0 || count > AC_MAX_RAW_TIMINGS) break;
                    tmp.timings = malloc(count * sizeof(uint32_t));
                    if(!flipper_format_read_uint32(ff, "data", tmp.timings, count)) {
                        free(tmp.timings);
                        tmp.timings = NULL;
                        break;
                    }
                    tmp.timings_size = count;
                    tmp.is_raw = true;
                    ok = true;
                } while(false);
            }

            if(ok && index >= 0) {
                tmp.present = true;
                ac_ir_signal_reset(&remote->signals[index]);
                remote->signals[index] = tmp;
                success = true;
            } else {
                ac_ir_signal_reset(&tmp);
            }
        }
    } while(false);

    furi_string_free(str);
    furi_string_free(header);
    flipper_format_free(ff);
    return success;
}

bool ac_remote_save(const AcRemote* remote, Storage* storage, const char* path) {
    FlipperFormat* ff = flipper_format_file_alloc(storage);
    bool success = false;

    do {
        if(!flipper_format_file_open_always(ff, path)) break;
        if(!flipper_format_write_header_cstr(ff, "IR signals file", 1)) break;

        bool ok = true;
        for(size_t i = 0; i < AC_BUTTON_COUNT; i++) {
            const AcIrSignal* signal = &remote->signals[i];
            if(!signal->present) continue;
            ok &= flipper_format_write_comment_cstr(ff, "");
            ok &= flipper_format_write_string_cstr(ff, "name", ac_button_names[i]);
            if(signal->is_raw) {
                uint32_t frequency = signal->frequency;
                float duty_cycle = signal->duty_cycle;
                ok &= flipper_format_write_string_cstr(ff, "type", "raw");
                ok &= flipper_format_write_uint32(ff, "frequency", &frequency, 1);
                ok &= flipper_format_write_float(ff, "duty_cycle", &duty_cycle, 1);
                ok &= flipper_format_write_uint32(ff, "data", signal->timings, signal->timings_size);
            } else {
                uint32_t address = signal->message.address;
                uint32_t command = signal->message.command;
                ok &= flipper_format_write_string_cstr(ff, "type", "parsed");
                ok &= flipper_format_write_string_cstr(
                    ff, "protocol", infrared_get_protocol_name(signal->message.protocol));
                ok &= flipper_format_write_hex(ff, "address", (uint8_t*)&address, 4);
                ok &= flipper_format_write_hex(ff, "command", (uint8_t*)&command, 4);
            }
            if(!ok) break;
        }
        if(!ok) break;
        success = true;
    } while(false);

    flipper_format_free(ff);
    return success;
}
