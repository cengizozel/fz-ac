#include "../fz_ac_app_i.h"

static void fz_ac_scene_smart_off_view_callback(void* context, LearnViewEvent event) {
    FzAcApp* app = context;
    FzAcCustomEventType type;
    switch(event) {
    case LearnViewEventOk:
        type = FzAcCustomEventTypeLearnOk;
        break;
    case LearnViewEventRetry:
        type = FzAcCustomEventTypeLearnRetry;
        break;
    default:
        type = FzAcCustomEventTypeLearnSkip;
        break;
    }
    view_dispatcher_send_custom_event(app->view_dispatcher, FZ_AC_EVENT(type, 0));
}

void fz_ac_scene_smart_off_on_enter(void* context) {
    FzAcApp* app = context;

    learn_view_set_callback(app->learn_view, fz_ac_scene_smart_off_view_callback, app);
    learn_view_set_button(app->learn_view, "POWER OFF", 1, 1);
    learn_view_set_captured(app->learn_view, false, "");

    fz_ac_rx_alloc(app);
    fz_ac_rx_start(app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewLearn);
}

bool fz_ac_scene_smart_off_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        uint16_t type = FZ_AC_EVENT_TYPE(event.event);
        switch(type) {
        case FzAcCustomEventTypeIrCaptured:
            fz_ac_rx_stop(app);
            furi_mutex_acquire(app->signal_mutex, FuriWaitForever);
            ac_ir_signal_describe(&app->capture, app->str);
            furi_mutex_release(app->signal_mutex);
            learn_view_set_captured(app->learn_view, true, furi_string_get_cstr(app->str));
            consumed = true;
            break;
        case FzAcCustomEventTypeLearnOk:
            furi_mutex_acquire(app->signal_mutex, FuriWaitForever);
            ac_ir_signal_move(&app->off_capture, &app->capture);
            furi_mutex_release(app->signal_mutex);
            scene_manager_next_scene(app->scene_manager, FzAcScenePresetName);
            consumed = true;
            break;
        case FzAcCustomEventTypeLearnSkip:
            fz_ac_rx_stop(app);
            ac_ir_signal_reset(&app->off_capture);
            scene_manager_next_scene(app->scene_manager, FzAcScenePresetName);
            consumed = true;
            break;
        case FzAcCustomEventTypeLearnRetry:
            furi_mutex_acquire(app->signal_mutex, FuriWaitForever);
            ac_ir_signal_reset(&app->capture);
            furi_mutex_release(app->signal_mutex);
            learn_view_set_captured(app->learn_view, false, "");
            fz_ac_rx_start(app);
            consumed = true;
            break;
        default:
            break;
        }
    }
    return consumed;
}

void fz_ac_scene_smart_off_on_exit(void* context) {
    FzAcApp* app = context;
    fz_ac_rx_free(app);
}
