#pragma once

#include <stdint.h>

typedef enum {
    FzAcCustomEventTypeMenuSelected,
    FzAcCustomEventTypeButtonSelected,
    FzAcCustomEventTypeIrCaptured,
    FzAcCustomEventTypeLearnOk,
    FzAcCustomEventTypeLearnRetry,
    FzAcCustomEventTypeLearnSkip,
    FzAcCustomEventTypeTextDone,
    FzAcCustomEventTypeDialogResult,
} FzAcCustomEventType;

#define FZ_AC_EVENT(type, value) (((uint32_t)(type) << 16) | (uint16_t)(value))
#define FZ_AC_EVENT_TYPE(event)  ((uint16_t)((event) >> 16))
#define FZ_AC_EVENT_VALUE(event) ((uint16_t)((event) & 0xFFFF))
