#include "../fz_ac_app_i.h"

#include <stdio.h>

enum {
    AcMenuIndexOpen,
    AcMenuIndexRelearn,
    AcMenuIndexRename,
    AcMenuIndexDelete,
};

static void fz_ac_scene_ac_menu_submenu_callback(void* context, uint32_t index) {
    FzAcApp* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeMenuSelected, index));
}

void fz_ac_scene_ac_menu_on_enter(void* context) {
    FzAcApp* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, app->ac_names[app->current_ac]);
    submenu_add_item(
        submenu, "Open remote", AcMenuIndexOpen, fz_ac_scene_ac_menu_submenu_callback, app);
    submenu_add_item(
        submenu, "Re-learn buttons", AcMenuIndexRelearn, fz_ac_scene_ac_menu_submenu_callback, app);
    submenu_add_item(
        submenu, "Rename", AcMenuIndexRename, fz_ac_scene_ac_menu_submenu_callback, app);
    submenu_add_item(
        submenu, "Delete", AcMenuIndexDelete, fz_ac_scene_ac_menu_submenu_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewSubmenu);
}

bool fz_ac_scene_ac_menu_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       FZ_AC_EVENT_TYPE(event.event) == FzAcCustomEventTypeMenuSelected) {
        switch(FZ_AC_EVENT_VALUE(event.event)) {
        case AcMenuIndexOpen:
            fz_ac_load_current(app);
            scene_manager_next_scene(app->scene_manager, FzAcSceneRemote);
            break;
        case AcMenuIndexRelearn:
            snprintf(
                app->learn_target,
                sizeof(app->learn_target),
                "%s",
                app->ac_names[app->current_ac]);
            scene_manager_next_scene(app->scene_manager, FzAcSceneLearn);
            break;
        case AcMenuIndexRename:
            app->renaming = true;
            snprintf(
                app->name_buf, sizeof(app->name_buf), "%s", app->ac_names[app->current_ac]);
            scene_manager_next_scene(app->scene_manager, FzAcSceneName);
            break;
        case AcMenuIndexDelete:
            scene_manager_next_scene(app->scene_manager, FzAcSceneDelete);
            break;
        default:
            break;
        }
        consumed = true;
    }
    return consumed;
}

void fz_ac_scene_ac_menu_on_exit(void* context) {
    FzAcApp* app = context;
    submenu_reset(app->submenu);
}
