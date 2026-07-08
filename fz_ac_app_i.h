#pragma once

#include "fz_ac_app.h"
#include "fz_ac_custom_event.h"
#include "ac_ir.h"
#include "alarms.h"
#include "views/ac_remote_panel.h"
#include "views/learn_view.h"
#include "scenes/fz_ac_scene.h"

#include <furi.h>
#include <furi_hal_rtc.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/variable_item_list.h>
#include <notification/notification_messages.h>
#include <storage/storage.h>
#include <infrared_worker.h>
#include <infrared_transmit.h>

#define FZ_AC_DIR         EXT_PATH("apps_data/fz_ac")
#define FZ_AC_ALARMS_PATH FZ_AC_DIR "/alarms.settings"
#define FZ_AC_MAX_ACS     32
#define FZ_AC_NAME_LEN    AC_ALARM_NAME_LEN
#define FZ_AC_PATH_LEN    128

typedef enum {
    FzAcViewSubmenu,
    FzAcViewTextInput,
    FzAcViewDialogEx,
    FzAcViewVarItemList,
    FzAcViewPanel,
    FzAcViewLearn,
} FzAcView;

struct FzAcApp {
    Gui* gui;
    SceneManager* scene_manager;
    ViewDispatcher* view_dispatcher;
    NotificationApp* notifications;
    Storage* storage;

    Submenu* submenu;
    TextInput* text_input;
    DialogEx* dialog_ex;
    VariableItemList* var_item_list;
    ACRemotePanel* panel;
    LearnView* learn_view;

    char ac_names[FZ_AC_MAX_ACS][FZ_AC_NAME_LEN];
    uint32_t ac_count;
    int32_t current_ac;
    AcRemote remote;
    uint32_t temp_display;
    char temp_str[8];
    char title_buf[12];

    char name_buf[FZ_AC_NAME_LEN];
    bool renaming;

    char learn_target[FZ_AC_NAME_LEN];
    uint8_t learn_index;
    InfraredWorker* rx_worker;
    bool rx_active;
    FuriMutex* signal_mutex;
    AcIrSignal capture;
    AcRemote staged;

    AcAlarms alarms;
    int32_t editing_alarm;
    AcAlarm edit_alarm;

    FuriString* str;
};

void fz_ac_refresh_ac_list(FzAcApp* app);
void fz_ac_ac_path(const char* name, char* out, size_t out_size);
void fz_ac_sanitize_name(char* name);
void fz_ac_make_unique_name(FzAcApp* app, char* name, size_t size);
int32_t fz_ac_find_ac(FzAcApp* app, const char* name);
void fz_ac_load_current(FzAcApp* app);
