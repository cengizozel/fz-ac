#include "../fz_ac_app_i.h"

#include <stdio.h>
#include <string.h>

static void fz_ac_scene_name_input_callback(void* context) {
    FzAcApp* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeTextDone, 0));
}

void fz_ac_scene_name_on_enter(void* context) {
    FzAcApp* app = context;
    TextInput* text_input = app->text_input;

    text_input_set_header_text(text_input, app->renaming ? "Rename AC" : "Name the AC");
    text_input_set_minimum_length(text_input, 1);
    text_input_set_result_callback(
        text_input,
        fz_ac_scene_name_input_callback,
        app,
        app->name_buf,
        FZ_AC_NAME_LEN,
        !app->renaming);

    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewTextInput);
}

bool fz_ac_scene_name_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       FZ_AC_EVENT_TYPE(event.event) == FzAcCustomEventTypeTextDone) {
        fz_ac_sanitize_name(app->name_buf);
        if(app->renaming) {
            const char* old_name = app->ac_names[app->current_ac];
            if(strcmp(old_name, app->name_buf) != 0) {
                fz_ac_make_unique_name(app, app->name_buf, sizeof(app->name_buf));
                char old_path[FZ_AC_PATH_LEN];
                char new_path[FZ_AC_PATH_LEN];
                fz_ac_ac_path(old_name, old_path, sizeof(old_path));
                fz_ac_ac_path(app->name_buf, new_path, sizeof(new_path));
                if(storage_common_rename(app->storage, old_path, new_path) == FSE_OK) {
                    bool dirty = false;
                    for(uint32_t i = 0; i < app->alarms.count; i++) {
                        if(strcmp(app->alarms.items[i].ac_name, old_name) == 0) {
                            snprintf(
                                app->alarms.items[i].ac_name,
                                sizeof(app->alarms.items[i].ac_name),
                                "%s",
                                app->name_buf);
                            dirty = true;
                        }
                    }
                    if(dirty) ac_alarms_save(&app->alarms, app->storage, FZ_AC_ALARMS_PATH);
                    fz_ac_refresh_ac_list(app);
                    app->current_ac = fz_ac_find_ac(app, app->name_buf);
                } else {
                    notification_message(app->notifications, &sequence_error);
                }
            }
            app->renaming = false;
            scene_manager_previous_scene(app->scene_manager);
        } else {
            fz_ac_make_unique_name(app, app->name_buf, sizeof(app->name_buf));
            snprintf(app->learn_target, sizeof(app->learn_target), "%s", app->name_buf);
            scene_manager_next_scene(app->scene_manager, FzAcSceneLearn);
        }
        consumed = true;
    }
    return consumed;
}

void fz_ac_scene_name_on_exit(void* context) {
    FzAcApp* app = context;
    text_input_reset(app->text_input);
}
