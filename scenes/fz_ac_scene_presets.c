#include "../fz_ac_app_i.h"

#include <stdio.h>

static void fz_ac_scene_presets_submenu_callback(void* context, uint32_t index) {
    FzAcApp* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeMenuSelected, index));
}

void fz_ac_scene_presets_on_enter(void* context) {
    FzAcApp* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "Delete preset");
    for(uint8_t i = 0; i < app->smart_index.preset_count; i++) {
        submenu_add_item(
            submenu, app->smart_index.presets[i].name, i, fz_ac_scene_presets_submenu_callback, app);
    }
    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewSubmenu);
}

bool fz_ac_scene_presets_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       FZ_AC_EVENT_TYPE(event.event) == FzAcCustomEventTypeMenuSelected) {
        uint32_t index = FZ_AC_EVENT_VALUE(event.event);
        if(index < app->smart_index.preset_count) {
            snprintf(
                app->preset_buf,
                sizeof(app->preset_buf),
                "%s",
                app->smart_index.presets[index].name);
            scene_manager_next_scene(app->scene_manager, FzAcScenePresetDelete);
        }
        consumed = true;
    }
    return consumed;
}

void fz_ac_scene_presets_on_exit(void* context) {
    FzAcApp* app = context;
    submenu_reset(app->submenu);
}
