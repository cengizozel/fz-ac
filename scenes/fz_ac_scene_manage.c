#include "../fz_ac_app_i.h"

static void fz_ac_scene_manage_submenu_callback(void* context, uint32_t index) {
    FzAcApp* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeMenuSelected, index));
}

void fz_ac_scene_manage_on_enter(void* context) {
    FzAcApp* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "Manage ACs");
    for(uint32_t i = 0; i < app->ac_count; i++) {
        submenu_add_item(submenu, app->ac_names[i], i, fz_ac_scene_manage_submenu_callback, app);
    }
    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewSubmenu);
}

bool fz_ac_scene_manage_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       FZ_AC_EVENT_TYPE(event.event) == FzAcCustomEventTypeMenuSelected) {
        uint32_t index = FZ_AC_EVENT_VALUE(event.event);
        if(index < app->ac_count) {
            app->current_ac = index;
            scene_manager_next_scene(app->scene_manager, FzAcSceneAcMenu);
        }
        consumed = true;
    }
    return consumed;
}

void fz_ac_scene_manage_on_exit(void* context) {
    FzAcApp* app = context;
    submenu_reset(app->submenu);
}
