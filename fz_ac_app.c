#include "fz_ac_app_i.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fz_ac_ac_path(const char* name, char* out, size_t out_size) {
    snprintf(out, out_size, "%s/%s.ir", FZ_AC_DIR, name);
}

void fz_ac_sanitize_name(char* name) {
    char* out = name;
    for(const char* in = name; *in; in++) {
        char c = *in;
        if(isalnum((unsigned char)c) || c == ' ' || c == '-' || c == '_') {
            *out++ = c;
        }
    }
    *out = '\0';
    // trim leading and trailing spaces
    while(name[0] == ' ') memmove(name, name + 1, strlen(name));
    size_t len = strlen(name);
    while(len > 0 && name[len - 1] == ' ') name[--len] = '\0';
    if(name[0] == '\0') snprintf(name, 3, "AC");
}

void fz_ac_make_unique_name(FzAcApp* app, char* name, size_t size) {
    char path[FZ_AC_PATH_LEN];
    fz_ac_ac_path(name, path, sizeof(path));
    if(!storage_file_exists(app->storage, path)) return;

    char base[FZ_AC_NAME_LEN];
    snprintf(base, sizeof(base), "%s", name);
    size_t max_base = (size > 4) ? size - 4 : 0;
    if(strlen(base) > max_base) base[max_base] = '\0';

    for(int n = 2; n < 100; n++) {
        snprintf(name, size, "%s %d", base, n);
        fz_ac_ac_path(name, path, sizeof(path));
        if(!storage_file_exists(app->storage, path)) return;
    }
}

int32_t fz_ac_find_ac(FzAcApp* app, const char* name) {
    for(uint32_t i = 0; i < app->ac_count; i++) {
        if(strcmp(app->ac_names[i], name) == 0) return i;
    }
    return -1;
}

static void fz_ac_sort_names(FzAcApp* app) {
    char tmp[FZ_AC_NAME_LEN];
    for(uint32_t i = 1; i < app->ac_count; i++) {
        memcpy(tmp, app->ac_names[i], FZ_AC_NAME_LEN);
        int32_t j = i - 1;
        while(j >= 0 && strcasecmp(app->ac_names[j], tmp) > 0) {
            memcpy(app->ac_names[j + 1], app->ac_names[j], FZ_AC_NAME_LEN);
            j--;
        }
        memcpy(app->ac_names[j + 1], tmp, FZ_AC_NAME_LEN);
    }
}

void fz_ac_refresh_ac_list(FzAcApp* app) {
    app->ac_count = 0;
    File* dir = storage_file_alloc(app->storage);
    if(storage_dir_open(dir, FZ_AC_DIR)) {
        FileInfo info;
        char name[FZ_AC_PATH_LEN];
        while(app->ac_count < FZ_AC_MAX_ACS &&
              storage_dir_read(dir, &info, name, sizeof(name))) {
            if(info.flags & FSF_DIRECTORY) continue;
            size_t len = strlen(name);
            if(len < 4 || strcmp(&name[len - 3], ".ir") != 0) continue;
            name[len - 3] = '\0';
            if(strlen(name) >= FZ_AC_NAME_LEN) continue;
            snprintf(app->ac_names[app->ac_count], FZ_AC_NAME_LEN, "%.*s", FZ_AC_NAME_LEN - 1, name);
            app->ac_count++;
        }
        storage_dir_close(dir);
    }
    storage_file_free(dir);
    fz_ac_sort_names(app);
}

void fz_ac_load_current(FzAcApp* app) {
    ac_remote_reset(&app->remote);
    if(app->current_ac < 0 || app->current_ac >= (int32_t)app->ac_count) return;
    char path[FZ_AC_PATH_LEN];
    fz_ac_ac_path(app->ac_names[app->current_ac], path, sizeof(path));
    ac_remote_load(&app->remote, app->storage, path);
}

static void fz_ac_alarm_fire(FzAcApp* app, AcAlarm* alarm) {
    bool sent = false;
    char path[FZ_AC_PATH_LEN];
    fz_ac_ac_path(alarm->ac_name, path, sizeof(path));

    AcRemote remote;
    memset(&remote, 0, sizeof(remote));
    if(ac_remote_load(&remote, app->storage, path)) {
        AcIrSignal* signal = &remote.signals[alarm->button];
        if(signal->present) {
            ac_ir_signal_send(signal);
            sent = true;
        }
    }
    ac_remote_reset(&remote);

    notification_message(app->notifications, &sequence_double_vibro);
    notification_message(app->notifications, sent ? &sequence_success : &sequence_error);
}

static void fz_ac_check_alarms(FzAcApp* app) {
    DateTime dt;
    furi_hal_rtc_get_datetime(&dt);
    uint32_t stamp = (uint32_t)dt.day * 10000 + (uint32_t)dt.hour * 100 + dt.minute;

    bool dirty = false;
    for(uint32_t i = 0; i < app->alarms.count; i++) {
        AcAlarm* alarm = &app->alarms.items[i];
        if(!alarm->enabled) continue;
        if(alarm->hour != dt.hour || alarm->minute != dt.minute) continue;
        if(alarm->fired_stamp == stamp) continue;
        alarm->fired_stamp = stamp;
        fz_ac_alarm_fire(app, alarm);
        if(!alarm->daily) {
            alarm->enabled = false;
            dirty = true;
        }
    }
    if(dirty) ac_alarms_save(&app->alarms, app->storage, FZ_AC_ALARMS_PATH);
}

static bool fz_ac_custom_event_callback(void* context, uint32_t event) {
    FzAcApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool fz_ac_back_event_callback(void* context) {
    FzAcApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static void fz_ac_tick_event_callback(void* context) {
    FzAcApp* app = context;
    // IR receive and transmit share the hardware, do not fire while learning
    if(!app->rx_active) fz_ac_check_alarms(app);
    scene_manager_handle_tick_event(app->scene_manager);
}

static FzAcApp* fz_ac_app_alloc(void) {
    FzAcApp* app = malloc(sizeof(FzAcApp));
    memset(app, 0, sizeof(FzAcApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    app->storage = furi_record_open(RECORD_STORAGE);
    storage_simply_mkdir(app->storage, FZ_AC_DIR);

    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&fz_ac_scene_handlers, app);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, fz_ac_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, fz_ac_back_event_callback);
    view_dispatcher_set_tick_event_callback(app->view_dispatcher, fz_ac_tick_event_callback, 500);

    app->submenu = submenu_alloc();
    view_dispatcher_add_view(app->view_dispatcher, FzAcViewSubmenu, submenu_get_view(app->submenu));
    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, FzAcViewTextInput, text_input_get_view(app->text_input));
    app->dialog_ex = dialog_ex_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, FzAcViewDialogEx, dialog_ex_get_view(app->dialog_ex));
    app->var_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, FzAcViewVarItemList, variable_item_list_get_view(app->var_item_list));
    app->panel = ac_remote_panel_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, FzAcViewPanel, ac_remote_panel_get_view(app->panel));
    app->learn_view = learn_view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, FzAcViewLearn, learn_view_get_view(app->learn_view));

    app->signal_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->str = furi_string_alloc();
    app->current_ac = -1;
    app->editing_alarm = -1;
    app->temp_display = 20;

    fz_ac_refresh_ac_list(app);
    ac_alarms_load(&app->alarms, app->storage, FZ_AC_ALARMS_PATH);

    return app;
}

static void fz_ac_app_free(FzAcApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, FzAcViewSubmenu);
    submenu_free(app->submenu);
    view_dispatcher_remove_view(app->view_dispatcher, FzAcViewTextInput);
    text_input_free(app->text_input);
    view_dispatcher_remove_view(app->view_dispatcher, FzAcViewDialogEx);
    dialog_ex_free(app->dialog_ex);
    view_dispatcher_remove_view(app->view_dispatcher, FzAcViewVarItemList);
    variable_item_list_free(app->var_item_list);
    view_dispatcher_remove_view(app->view_dispatcher, FzAcViewPanel);
    ac_remote_panel_free(app->panel);
    view_dispatcher_remove_view(app->view_dispatcher, FzAcViewLearn);
    learn_view_free(app->learn_view);

    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);

    ac_remote_reset(&app->remote);
    ac_remote_reset(&app->staged);
    ac_ir_signal_reset(&app->capture);
    furi_mutex_free(app->signal_mutex);
    furi_string_free(app->str);

    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_GUI);
    free(app);
}

int32_t fz_ac_app(void* p) {
    UNUSED(p);
    FzAcApp* app = fz_ac_app_alloc();

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    scene_manager_next_scene(app->scene_manager, FzAcSceneStart);
    view_dispatcher_run(app->view_dispatcher);

    fz_ac_app_free(app);
    return 0;
}
