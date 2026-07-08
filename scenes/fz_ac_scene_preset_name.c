#include "../fz_ac_app_i.h"

#include <stdio.h>
#include <string.h>

static void fz_ac_scene_preset_name_input_callback(void* context) {
    FzAcApp* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeTextDone, 0));
}

void fz_ac_scene_preset_name_on_enter(void* context) {
    FzAcApp* app = context;
    TextInput* text_input = app->text_input;

    app->preset_buf[0] = '\0';
    text_input_set_header_text(text_input, "Mode name (Cool, Heat...)");
    text_input_set_minimum_length(text_input, 1);
    text_input_set_result_callback(
        text_input,
        fz_ac_scene_preset_name_input_callback,
        app,
        app->preset_buf,
        sizeof(app->preset_buf),
        true);

    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewTextInput);
}

bool fz_ac_scene_preset_name_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       FZ_AC_EVENT_TYPE(event.event) == FzAcCustomEventTypeTextDone) {
        fz_ac_sanitize_name(app->preset_buf);
        if(strcmp(app->preset_buf, AC_OFF_NAME) == 0) {
            snprintf(app->preset_buf, sizeof(app->preset_buf), "Preset");
        }
        scene_manager_next_scene(app->scene_manager, FzAcSceneSweep);
        consumed = true;
    }
    return consumed;
}

void fz_ac_scene_preset_name_on_exit(void* context) {
    FzAcApp* app = context;
    text_input_reset(app->text_input);
}
