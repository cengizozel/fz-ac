#include "../fz_ac_app_i.h"

enum {
    TypeIndexSmart,
    TypeIndexSimple,
};

static void fz_ac_scene_type_submenu_callback(void* context, uint32_t index) {
    FzAcApp* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeMenuSelected, index));
}

void fz_ac_scene_type_on_enter(void* context) {
    FzAcApp* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "AC type");
    submenu_add_item(
        submenu, "Smart AC (temp presets)", TypeIndexSmart, fz_ac_scene_type_submenu_callback, app);
    submenu_add_item(
        submenu,
        "Simple remote (6 buttons)",
        TypeIndexSimple,
        fz_ac_scene_type_submenu_callback,
        app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewSubmenu);
}

bool fz_ac_scene_type_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       FZ_AC_EVENT_TYPE(event.event) == FzAcCustomEventTypeMenuSelected) {
        if(FZ_AC_EVENT_VALUE(event.event) == TypeIndexSmart) {
            app->smart_creating = true;
            ac_ir_signal_reset(&app->off_capture);
            scene_manager_next_scene(app->scene_manager, FzAcSceneSmartOff);
        } else {
            scene_manager_next_scene(app->scene_manager, FzAcSceneLearn);
        }
        consumed = true;
    }
    return consumed;
}

void fz_ac_scene_type_on_exit(void* context) {
    FzAcApp* app = context;
    submenu_reset(app->submenu);
}
