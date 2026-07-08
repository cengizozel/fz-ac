#include "../fz_ac_app_i.h"

#include <stdio.h>
#include <string.h>

enum {
    AlarmsIndexAdd = 1000,
};

static void fz_ac_scene_alarms_submenu_callback(void* context, uint32_t index) {
    FzAcApp* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeMenuSelected, index));
}

void fz_ac_scene_alarms_on_enter(void* context) {
    FzAcApp* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "Alarms");
    char label[64];
    char action[24];
    for(uint32_t i = 0; i < app->alarms.count; i++) {
        const AcAlarm* alarm = &app->alarms.items[i];
        if(alarm->kind == AcAlarmKindPreset) {
            snprintf(action, sizeof(action), "%s %u", alarm->preset, alarm->temp);
        } else if(alarm->kind == AcAlarmKindOff) {
            snprintf(action, sizeof(action), "OFF");
        } else {
            snprintf(action, sizeof(action), "%s", ac_button_labels[alarm->button]);
        }
        snprintf(
            label,
            sizeof(label),
            "%s%02u:%02u %s %s",
            alarm->enabled ? "" : "[off] ",
            alarm->hour,
            alarm->minute,
            action,
            alarm->ac_name);
        submenu_add_item(submenu, label, i, fz_ac_scene_alarms_submenu_callback, app);
    }
    submenu_add_item(
        submenu, "+ Add alarm", AlarmsIndexAdd, fz_ac_scene_alarms_submenu_callback, app);

    submenu_set_selected_item(
        submenu, scene_manager_get_scene_state(app->scene_manager, FzAcSceneAlarms));
    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewSubmenu);
}

bool fz_ac_scene_alarms_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       FZ_AC_EVENT_TYPE(event.event) == FzAcCustomEventTypeMenuSelected) {
        uint32_t index = FZ_AC_EVENT_VALUE(event.event);
        scene_manager_set_scene_state(app->scene_manager, FzAcSceneAlarms, index);
        if(index == AlarmsIndexAdd) {
            if(app->ac_count == 0 || app->alarms.count >= AC_MAX_ALARMS) {
                notification_message(app->notifications, &sequence_error);
            } else {
                app->editing_alarm = -1;
                memset(&app->edit_alarm, 0, sizeof(app->edit_alarm));
                app->edit_alarm.enabled = true;
                app->edit_alarm.daily = true;
                app->edit_alarm.hour = 7;
                app->edit_alarm.minute = 0;
                snprintf(
                    app->edit_alarm.ac_name,
                    sizeof(app->edit_alarm.ac_name),
                    "%s",
                    app->ac_names[0]);
                scene_manager_next_scene(app->scene_manager, FzAcSceneAlarmEdit);
            }
        } else if(index < app->alarms.count) {
            app->editing_alarm = index;
            app->edit_alarm = app->alarms.items[index];
            scene_manager_next_scene(app->scene_manager, FzAcSceneAlarmEdit);
        }
        consumed = true;
    }
    return consumed;
}

void fz_ac_scene_alarms_on_exit(void* context) {
    FzAcApp* app = context;
    submenu_reset(app->submenu);
}
