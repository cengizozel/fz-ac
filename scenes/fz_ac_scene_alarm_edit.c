#include "../fz_ac_app_i.h"

#include <stdio.h>
#include <string.h>

enum {
    AlarmEditItemAc,
    AlarmEditItemButton,
    AlarmEditItemHour,
    AlarmEditItemMinute,
    AlarmEditItemRepeat,
    AlarmEditItemEnabled,
    AlarmEditItemSave,
    AlarmEditItemDelete,
};

static const char* const repeat_texts[] = {"Once", "Daily"};
static const char* const enabled_texts[] = {"Off", "On"};

static void fz_ac_scene_alarm_edit_ac_changed(VariableItem* item) {
    FzAcApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    snprintf(app->edit_alarm.ac_name, sizeof(app->edit_alarm.ac_name), "%s", app->ac_names[index]);
    variable_item_set_current_value_text(item, app->ac_names[index]);
}

static void fz_ac_scene_alarm_edit_button_changed(VariableItem* item) {
    FzAcApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->edit_alarm.button = index;
    variable_item_set_current_value_text(item, ac_button_labels[index]);
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
    if(index == AlarmEditItemSave || index == AlarmEditItemDelete) {
        view_dispatcher_send_custom_event(
            app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeMenuSelected, index));
    }
}

void fz_ac_scene_alarm_edit_on_enter(void* context) {
    FzAcApp* app = context;
    VariableItemList* list = app->var_item_list;
    VariableItem* item;
    char buf[4];

    variable_item_list_reset(list);

    item = variable_item_list_add(
        list, "AC", app->ac_count, fz_ac_scene_alarm_edit_ac_changed, app);
    int32_t ac_index = fz_ac_find_ac(app, app->edit_alarm.ac_name);
    if(ac_index < 0) {
        ac_index = 0;
        snprintf(app->edit_alarm.ac_name, sizeof(app->edit_alarm.ac_name), "%s", app->ac_names[0]);
    }
    variable_item_set_current_value_index(item, ac_index);
    variable_item_set_current_value_text(item, app->ac_names[ac_index]);

    item = variable_item_list_add(
        list, "Button", AC_BUTTON_COUNT, fz_ac_scene_alarm_edit_button_changed, app);
    variable_item_set_current_value_index(item, app->edit_alarm.button);
    variable_item_set_current_value_text(item, ac_button_labels[app->edit_alarm.button]);

    item = variable_item_list_add(list, "Hour", 24, fz_ac_scene_alarm_edit_hour_changed, app);
    variable_item_set_current_value_index(item, app->edit_alarm.hour);
    snprintf(buf, sizeof(buf), "%02u", app->edit_alarm.hour);
    variable_item_set_current_value_text(item, buf);

    item = variable_item_list_add(list, "Minute", 60, fz_ac_scene_alarm_edit_minute_changed, app);
    variable_item_set_current_value_index(item, app->edit_alarm.minute);
    snprintf(buf, sizeof(buf), "%02u", app->edit_alarm.minute);
    variable_item_set_current_value_text(item, buf);

    item = variable_item_list_add(list, "Repeat", 2, fz_ac_scene_alarm_edit_repeat_changed, app);
    variable_item_set_current_value_index(item, app->edit_alarm.daily ? 1 : 0);
    variable_item_set_current_value_text(item, repeat_texts[app->edit_alarm.daily ? 1 : 0]);

    item = variable_item_list_add(list, "Enabled", 2, fz_ac_scene_alarm_edit_enabled_changed, app);
    variable_item_set_current_value_index(item, app->edit_alarm.enabled ? 1 : 0);
    variable_item_set_current_value_text(item, enabled_texts[app->edit_alarm.enabled ? 1 : 0]);

    variable_item_list_add(list, "Save", 0, NULL, NULL);
    if(app->editing_alarm >= 0) {
        variable_item_list_add(list, "Delete alarm", 0, NULL, NULL);
    }

    variable_item_list_set_enter_callback(list, fz_ac_scene_alarm_edit_enter_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewVarItemList);
}

bool fz_ac_scene_alarm_edit_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       FZ_AC_EVENT_TYPE(event.event) == FzAcCustomEventTypeMenuSelected) {
        uint32_t index = FZ_AC_EVENT_VALUE(event.event);
        if(index == AlarmEditItemSave) {
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
        } else if(index == AlarmEditItemDelete) {
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
