#include "../fz_ac_app_i.h"

#include <stdio.h>
#include <string.h>

static const char* const repeat_texts[] = {"Once", "Daily"};
static const char* const enabled_texts[] = {"Off", "On"};

static uint8_t fz_ac_temp_bits_count(uint32_t bits) {
    uint8_t count = 0;
    for(uint8_t i = 0; i < AC_TEMP_SLOTS; i++) {
        if(bits & (1UL << i)) count++;
    }
    return count;
}

static int32_t fz_ac_temp_bits_nth(uint32_t bits, uint8_t n) {
    uint8_t seen = 0;
    for(uint8_t i = 0; i < AC_TEMP_SLOTS; i++) {
        if(!(bits & (1UL << i))) continue;
        if(seen == n) return AC_TEMP_BASE + i;
        seen++;
    }
    return -1;
}

static uint8_t fz_ac_temp_bits_index_of(uint32_t bits, uint8_t temp) {
    uint8_t index = 0;
    for(uint8_t i = 0; i < AC_TEMP_SLOTS; i++) {
        if(!(bits & (1UL << i))) continue;
        if(AC_TEMP_BASE + i == temp) return index;
        index++;
    }
    return 0;
}

static uint32_t fz_ac_alarm_edit_preset_bits(FzAcApp* app) {
    if(app->edit_type != AcTypeSmart || app->edit_action == 0) return 0;
    if(app->edit_action > app->edit_index.preset_count) return 0;
    return app->edit_index.presets[app->edit_action - 1].temp_bits;
}

// scan the file of the AC selected in the editor and derive the action index
static void fz_ac_alarm_edit_scan(FzAcApp* app, bool keep_action) {
    char path[FZ_AC_PATH_LEN];
    fz_ac_ac_path(app->edit_alarm.ac_name, path, sizeof(path));
    app->edit_type = ac_file_scan(app->storage, path, &app->edit_index);

    if(!keep_action) {
        app->edit_action = 0;
    } else if(app->edit_type == AcTypeSimple) {
        app->edit_action = (app->edit_alarm.kind == AcAlarmKindButton) ? app->edit_alarm.button : 0;
    } else {
        app->edit_action = 0;
        if(app->edit_alarm.kind == AcAlarmKindPreset) {
            for(uint8_t i = 0; i < app->edit_index.preset_count; i++) {
                if(strcmp(app->edit_index.presets[i].name, app->edit_alarm.preset) == 0) {
                    app->edit_action = i + 1;
                    break;
                }
            }
        }
    }

    uint32_t bits = fz_ac_alarm_edit_preset_bits(app);
    if(bits && !(bits & (1UL << (app->edit_alarm.temp - AC_TEMP_BASE)))) {
        int32_t lowest = ac_temp_bits_lowest(bits);
        if(lowest > 0) app->edit_alarm.temp = lowest;
    }
}

static void fz_ac_scene_alarm_edit_ac_changed(VariableItem* item) {
    FzAcApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    snprintf(app->edit_alarm.ac_name, sizeof(app->edit_alarm.ac_name), "%s", app->ac_names[index]);
    variable_item_set_current_value_text(item, app->ac_names[index]);
    fz_ac_alarm_edit_scan(app, false);
    view_dispatcher_send_custom_event(
        app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeAlarmEditRebuild, 0));
}

static void fz_ac_scene_alarm_edit_action_changed(VariableItem* item) {
    FzAcApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->edit_action = index;
    if(app->edit_type == AcTypeSimple) {
        variable_item_set_current_value_text(item, ac_button_labels[index]);
    } else {
        variable_item_set_current_value_text(
            item, index == 0 ? "OFF" : app->edit_index.presets[index - 1].name);
        uint32_t bits = fz_ac_alarm_edit_preset_bits(app);
        if(bits) {
            int32_t lowest = ac_temp_bits_lowest(bits);
            if(lowest > 0) app->edit_alarm.temp = lowest;
        }
        view_dispatcher_send_custom_event(
            app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeAlarmEditRebuild, 1));
    }
}

static void fz_ac_scene_alarm_edit_temp_changed(VariableItem* item) {
    FzAcApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    int32_t temp = fz_ac_temp_bits_nth(fz_ac_alarm_edit_preset_bits(app), index);
    if(temp > 0) app->edit_alarm.temp = temp;
    char buf[8];
    snprintf(buf, sizeof(buf), "%ld", temp);
    variable_item_set_current_value_text(item, buf);
}

static void fz_ac_scene_alarm_edit_hour_changed(VariableItem* item) {
    FzAcApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->edit_alarm.hour = index;
    char buf[4];
    snprintf(buf, sizeof(buf), "%02u", index);
    variable_item_set_current_value_text(item, buf);
}

static void fz_ac_scene_alarm_edit_minute_changed(VariableItem* item) {
    FzAcApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->edit_alarm.minute = index;
    char buf[4];
    snprintf(buf, sizeof(buf), "%02u", index);
    variable_item_set_current_value_text(item, buf);
}

static void fz_ac_scene_alarm_edit_repeat_changed(VariableItem* item) {
    FzAcApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->edit_alarm.daily = index != 0;
    variable_item_set_current_value_text(item, repeat_texts[index]);
}

static void fz_ac_scene_alarm_edit_enabled_changed(VariableItem* item) {
    FzAcApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->edit_alarm.enabled = index != 0;
    variable_item_set_current_value_text(item, enabled_texts[index]);
}

static void fz_ac_scene_alarm_edit_enter_callback(void* context, uint32_t index) {
    FzAcApp* app = context;
    if(index == app->alarm_save_index || index == app->alarm_delete_index) {
        view_dispatcher_send_custom_event(
            app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeMenuSelected, index));
    }
}

static void fz_ac_scene_alarm_edit_build(FzAcApp* app) {
    VariableItemList* list = app->var_item_list;
    VariableItem* item;
    char buf[8];
    uint8_t item_index = 0;

    variable_item_list_reset(list);

    // AC
    item = variable_item_list_add(list, "AC", app->ac_count, fz_ac_scene_alarm_edit_ac_changed, app);
    int32_t ac_index = fz_ac_find_ac(app, app->edit_alarm.ac_name);
    if(ac_index < 0) ac_index = 0;
    variable_item_set_current_value_index(item, ac_index);
    variable_item_set_current_value_text(item, app->ac_names[ac_index]);
    item_index++;

    // Action
    if(app->edit_type == AcTypeSimple) {
        item = variable_item_list_add(
            list, "Action", AC_BUTTON_COUNT, fz_ac_scene_alarm_edit_action_changed, app);
        variable_item_set_current_value_index(item, app->edit_action);
        variable_item_set_current_value_text(item, ac_button_labels[app->edit_action]);
    } else {
        item = variable_item_list_add(
            list,
            "Action",
            1 + app->edit_index.preset_count,
            fz_ac_scene_alarm_edit_action_changed,
            app);
        variable_item_set_current_value_index(item, app->edit_action);
        variable_item_set_current_value_text(
            item,
            app->edit_action == 0 ? "OFF" : app->edit_index.presets[app->edit_action - 1].name);
    }
    item_index++;

    // Temp (smart preset action only)
    uint32_t bits = fz_ac_alarm_edit_preset_bits(app);
    if(bits) {
        item = variable_item_list_add(
            list, "Temp", fz_ac_temp_bits_count(bits), fz_ac_scene_alarm_edit_temp_changed, app);
        uint8_t temp_index = fz_ac_temp_bits_index_of(bits, app->edit_alarm.temp);
        variable_item_set_current_value_index(item, temp_index);
        snprintf(buf, sizeof(buf), "%u", app->edit_alarm.temp);
        variable_item_set_current_value_text(item, buf);
        item_index++;
    }

    item = variable_item_list_add(list, "Hour", 24, fz_ac_scene_alarm_edit_hour_changed, app);
    variable_item_set_current_value_index(item, app->edit_alarm.hour);
    snprintf(buf, sizeof(buf), "%02u", app->edit_alarm.hour);
    variable_item_set_current_value_text(item, buf);
    item_index++;

    item = variable_item_list_add(list, "Minute", 60, fz_ac_scene_alarm_edit_minute_changed, app);
    variable_item_set_current_value_index(item, app->edit_alarm.minute);
    snprintf(buf, sizeof(buf), "%02u", app->edit_alarm.minute);
    variable_item_set_current_value_text(item, buf);
    item_index++;

    item = variable_item_list_add(list, "Repeat", 2, fz_ac_scene_alarm_edit_repeat_changed, app);
    variable_item_set_current_value_index(item, app->edit_alarm.daily ? 1 : 0);
    variable_item_set_current_value_text(item, repeat_texts[app->edit_alarm.daily ? 1 : 0]);
    item_index++;

    item = variable_item_list_add(list, "Enabled", 2, fz_ac_scene_alarm_edit_enabled_changed, app);
    variable_item_set_current_value_index(item, app->edit_alarm.enabled ? 1 : 0);
    variable_item_set_current_value_text(item, enabled_texts[app->edit_alarm.enabled ? 1 : 0]);
    item_index++;

    app->alarm_save_index = item_index;
    variable_item_list_add(list, "Save", 0, NULL, NULL);
    item_index++;

    app->alarm_delete_index = 0xFF;
    if(app->editing_alarm >= 0) {
        app->alarm_delete_index = item_index;
        variable_item_list_add(list, "Delete alarm", 0, NULL, NULL);
    }

    variable_item_list_set_enter_callback(list, fz_ac_scene_alarm_edit_enter_callback, app);
}

void fz_ac_scene_alarm_edit_on_enter(void* context) {
    FzAcApp* app = context;

    if(fz_ac_find_ac(app, app->edit_alarm.ac_name) < 0 && app->ac_count > 0) {
        snprintf(app->edit_alarm.ac_name, sizeof(app->edit_alarm.ac_name), "%s", app->ac_names[0]);
    }
    fz_ac_alarm_edit_scan(app, true);
    fz_ac_scene_alarm_edit_build(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewVarItemList);
}

bool fz_ac_scene_alarm_edit_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type != SceneManagerEventTypeCustom) return false;
    uint16_t type = FZ_AC_EVENT_TYPE(event.event);

    if(type == FzAcCustomEventTypeAlarmEditRebuild) {
        fz_ac_scene_alarm_edit_build(app);
        variable_item_list_set_selected_item(app->var_item_list, FZ_AC_EVENT_VALUE(event.event));
        consumed = true;
    } else if(type == FzAcCustomEventTypeMenuSelected) {
        uint32_t index = FZ_AC_EVENT_VALUE(event.event);
        if(index == app->alarm_save_index) {
            if(app->edit_type == AcTypeSimple) {
                app->edit_alarm.kind = AcAlarmKindButton;
                app->edit_alarm.button = app->edit_action;
                app->edit_alarm.preset[0] = '\0';
            } else if(app->edit_action == 0) {
                app->edit_alarm.kind = AcAlarmKindOff;
                app->edit_alarm.preset[0] = '\0';
            } else {
                app->edit_alarm.kind = AcAlarmKindPreset;
                snprintf(
                    app->edit_alarm.preset,
                    sizeof(app->edit_alarm.preset),
                    "%s",
                    app->edit_index.presets[app->edit_action - 1].name);
            }
            app->edit_alarm.fired_stamp = 0;
            if(app->editing_alarm < 0) {
                if(app->alarms.count < AC_MAX_ALARMS) {
                    app->alarms.items[app->alarms.count++] = app->edit_alarm;
                }
            } else {
                app->alarms.items[app->editing_alarm] = app->edit_alarm;
            }
            ac_alarms_save(&app->alarms, app->storage, FZ_AC_ALARMS_PATH);
            scene_manager_previous_scene(app->scene_manager);
        } else if(index == app->alarm_delete_index) {
            if(app->editing_alarm >= 0) {
                ac_alarms_remove(&app->alarms, app->editing_alarm);
                ac_alarms_save(&app->alarms, app->storage, FZ_AC_ALARMS_PATH);
            }
            scene_manager_set_scene_state(app->scene_manager, FzAcSceneAlarms, 0);
            scene_manager_previous_scene(app->scene_manager);
        }
        consumed = true;
    }
    return consumed;
}

void fz_ac_scene_alarm_edit_on_exit(void* context) {
    FzAcApp* app = context;
    variable_item_list_reset(app->var_item_list);
}
