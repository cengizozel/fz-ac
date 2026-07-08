#pragma once

#include <furi.h>
#include <storage/storage.h>
#include <infrared.h>
#include <infrared_worker.h>

#define AC_BUTTON_COUNT     6
#define AC_MAX_RAW_TIMINGS  1024

typedef enum {
    AcButtonOff,
    AcButtonMode,
    AcButtonTempUp,
    AcButtonTempDown,
    AcButtonFan,
    AcButtonVane,
} AcButton;

extern const char* const ac_button_names[AC_BUTTON_COUNT];
extern const char* const ac_button_labels[AC_BUTTON_COUNT];

typedef struct {
    bool present;
    bool is_raw;
    InfraredMessage message;
    uint32_t* timings;
    size_t timings_size;
    uint32_t frequency;
    float duty_cycle;
} AcIrSignal;

typedef struct {
    AcIrSignal signals[AC_BUTTON_COUNT];
} AcRemote;

void ac_ir_signal_reset(AcIrSignal* signal);
void ac_ir_signal_from_worker(AcIrSignal* signal, const InfraredWorkerSignal* worker_signal);
void ac_ir_signal_move(AcIrSignal* dst, AcIrSignal* src);
void ac_ir_signal_describe(const AcIrSignal* signal, FuriString* out);
void ac_ir_signal_send(const AcIrSignal* signal);

void ac_remote_reset(AcRemote* remote);
bool ac_remote_load(AcRemote* remote, Storage* storage, const char* path);
bool ac_remote_save(const AcRemote* remote, Storage* storage, const char* path);
