#include "../fz_ac_app_i.h"

#include <string.h>

static void fz_ac_scene_delete_dialog_callback(DialogExResult result, void* context) {
    FzAcApp* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeDialogResult, result));
}

void fz_ac_scene_delete_on_enter(void* context) {
    FzAcApp* app = context;
    DialogEx* dialog = app->dialog_ex;

    dialog_ex_set_header(dialog, "Delete this AC?", 64, 4, AlignCenter, AlignTop);
    dialog_ex_set_text(
        dialog, app->ac_names[app->current_ac], 64, 24, AlignCenter, AlignTop);
    dialog_ex_set_left_button_text(dialog, "Cancel");
    dialog_ex_set_right_button_text(dialog, "Delete");
    dialog_ex_set_result_callback(dialog, fz_ac_scene_delete_dialog_callback);
    dialog_ex_set_context(dialog, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewDialogEx);
}

bool fz_ac_scene_delete_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       FZ_AC_EVENT_TYPE(event.event) == FzAcCustomEventTypeDialogResult) {
        if(FZ_AC_EVENT_VALUE(event.event) == DialogExResultRight) {
            const char* name = app->ac_names[app->current_ac];
            char path[FZ_AC_PATH_LEN];
            fz_ac_ac_path(name, path, sizeof(path));
            storage_common_remove(app->storage, path);

            for(int32_t i = app->alarms.count - 1; i >= 0; i--) {
                if(strcmp(app->alarms.items[i].ac_name, name) == 0) {
                    ac_alarms_remove(&app->alarms, i);
                }
            }
            ac_alarms_save(&app->alarms, app->storage, FZ_AC_ALARMS_PATH);

            fz_ac_refresh_ac_list(app);
            app->current_ac = -1;
            scene_manager_set_scene_state(app->scene_manager, FzAcSceneStart, 0);
            scene_manager_search_and_switch_to_previous_scene(
                app->scene_manager, FzAcSceneStart);
        } else {
            scene_manager_previous_scene(app->scene_manager);
        }
        consumed = true;
    }
    return consumed;
}

void fz_ac_scene_delete_on_exit(void* context) {
    FzAcApp* app = context;
    dialog_ex_reset(app->dialog_ex);
}
