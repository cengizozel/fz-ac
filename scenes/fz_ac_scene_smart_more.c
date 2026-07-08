#include "../fz_ac_app_i.h"

static void fz_ac_scene_smart_more_dialog_callback(DialogExResult result, void* context) {
    FzAcApp* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeDialogResult, result));
}

static void fz_ac_scene_smart_more_finish(FzAcApp* app) {
    fz_ac_refresh_ac_list(app);
    app->current_ac = fz_ac_find_ac(app, app->learn_target);
    fz_ac_load_current(app);
    scene_manager_search_and_switch_to_previous_scene(app->scene_manager, FzAcSceneStart);
    if(app->current_ac >= 0) {
        scene_manager_next_scene(app->scene_manager, FzAcSceneRemote);
    }
}

void fz_ac_scene_smart_more_on_enter(void* context) {
    FzAcApp* app = context;
    DialogEx* dialog = app->dialog_ex;

    dialog_ex_set_header(dialog, "Preset saved", 64, 4, AlignCenter, AlignTop);
    dialog_ex_set_text(dialog, "Add another preset?", 64, 26, AlignCenter, AlignTop);
    dialog_ex_set_left_button_text(dialog, "Done");
    dialog_ex_set_right_button_text(dialog, "Add more");
    dialog_ex_set_result_callback(dialog, fz_ac_scene_smart_more_dialog_callback);
    dialog_ex_set_context(dialog, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewDialogEx);
}

bool fz_ac_scene_smart_more_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       FZ_AC_EVENT_TYPE(event.event) == FzAcCustomEventTypeDialogResult) {
        if(FZ_AC_EVENT_VALUE(event.event) == DialogExResultRight) {
            scene_manager_search_and_switch_to_previous_scene(
                app->scene_manager, FzAcScenePresetName);
        } else {
            fz_ac_scene_smart_more_finish(app);
        }
        consumed = true;
    } else if(event.type == SceneManagerEventTypeBack) {
        fz_ac_scene_smart_more_finish(app);
        consumed = true;
    }
    return consumed;
}

void fz_ac_scene_smart_more_on_exit(void* context) {
    FzAcApp* app = context;
    dialog_ex_reset(app->dialog_ex);
}
