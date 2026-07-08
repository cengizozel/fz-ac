#include "../fz_ac_app_i.h"

enum {
    StartIndexAdd = 1000,
    StartIndexManage,
    StartIndexAlarms,
};

static void fz_ac_scene_start_submenu_callback(void* context, uint32_t index) {
    FzAcApp* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeMenuSelected, index));
}

void fz_ac_scene_start_on_enter(void* context) {
    FzAcApp* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "AC Remote");
    for(uint32_t i = 0; i < app->ac_count; i++) {
        submenu_add_item(submenu, app->ac_names[i], i, fz_ac_scene_start_submenu_callback, app);
    }
    submenu_add_item(submenu, "+ Add AC", StartIndexAdd, fz_ac_scene_start_submenu_callback, app);
    if(app->ac_count > 0) {
        submenu_add_item(
            submenu, "Manage ACs", StartIndexManage, fz_ac_scene_start_submenu_callback, app);
    }
    submenu_add_item(submenu, "Alarms", StartIndexAlarms, fz_ac_scene_start_submenu_callback, app);

    submenu_set_selected_item(
        submenu, scene_manager_get_scene_state(app->scene_manager, FzAcSceneStart));
    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewSubmenu);
}

bool fz_ac_scene_start_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       FZ_AC_EVENT_TYPE(event.event) == FzAcCustomEventTypeMenuSelected) {
        uint32_t index = FZ_AC_EVENT_VALUE(event.event);
        scene_manager_set_scene_state(app->scene_manager, FzAcSceneStart, index);
        if(index < app->ac_count) {
            app->current_ac = index;
            fz_ac_load_current(app);
            scene_manager_next_scene(app->scene_manager, FzAcSceneRemote);
        } else if(index == StartIndexAdd) {
            if(app->ac_count >= FZ_AC_MAX_ACS) {
                notification_message(app->notifications, &sequence_error);
            } else {
                app->renaming = false;
                app->name_buf[0] = '\0';
                scene_manager_next_scene(app->scene_manager, FzAcSceneName);
            }
        } else if(index == StartIndexManage) {
            scene_manager_next_scene(app->scene_manager, FzAcSceneManage);
        } else if(index == StartIndexAlarms) {
            scene_manager_next_scene(app->scene_manager, FzAcSceneAlarms);
        }
        consumed = true;
    }
    return consumed;
}

void fz_ac_scene_start_on_exit(void* context) {
    FzAcApp* app = context;
    submenu_reset(app->submenu);
}
