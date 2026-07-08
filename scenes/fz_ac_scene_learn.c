#include "../fz_ac_app_i.h"

static void fz_ac_scene_learn_rx_callback(void* context, InfraredWorkerSignal* signal) {
    FzAcApp* app = context;
    furi_mutex_acquire(app->signal_mutex, FuriWaitForever);
    ac_ir_signal_from_worker(&app->capture, signal);
    furi_mutex_release(app->signal_mutex);
    view_dispatcher_send_custom_event(
        app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeIrCaptured, 0));
}

static void fz_ac_scene_learn_view_callback(void* context, LearnViewEvent event) {
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

static void fz_ac_scene_learn_rx_start(FzAcApp* app) {
    if(!app->rx_active) {
        infrared_worker_rx_start(app->rx_worker);
        app->rx_active = true;
    }
}

static void fz_ac_scene_learn_rx_stop(FzAcApp* app) {
    if(app->rx_active) {
        infrared_worker_rx_stop(app->rx_worker);
        app->rx_active = false;
    }
}

static void fz_ac_scene_learn_show_current(FzAcApp* app) {
    learn_view_set_button(
        app->learn_view,
        ac_button_labels[app->learn_index],
        app->learn_index + 1,
        AC_BUTTON_COUNT);
    learn_view_set_captured(app->learn_view, false, "");
}

static void fz_ac_scene_learn_finish(FzAcApp* app) {
    fz_ac_scene_learn_rx_stop(app);

    bool any_present = false;
    for(size_t i = 0; i < AC_BUTTON_COUNT; i++) {
        if(app->staged.signals[i].present) {
            any_present = true;
            break;
        }
    }

    if(any_present) {
        char path[FZ_AC_PATH_LEN];
        fz_ac_ac_path(app->learn_target, path, sizeof(path));
        bool saved = ac_remote_save(&app->staged, app->storage, path);
        fz_ac_refresh_ac_list(app);
        notification_message(app->notifications, saved ? &sequence_success : &sequence_error);
        if(saved) {
            app->current_ac = fz_ac_find_ac(app, app->learn_target);
            fz_ac_load_current(app);
            scene_manager_search_and_switch_to_previous_scene(app->scene_manager, FzAcSceneStart);
            scene_manager_next_scene(app->scene_manager, FzAcSceneRemote);
            return;
        }
    } else {
        notification_message(app->notifications, &sequence_error);
    }
    scene_manager_search_and_switch_to_previous_scene(app->scene_manager, FzAcSceneStart);
}

static void fz_ac_scene_learn_advance(FzAcApp* app) {
    app->learn_index++;
    if(app->learn_index < AC_BUTTON_COUNT) {
        fz_ac_scene_learn_show_current(app);
        fz_ac_scene_learn_rx_start(app);
    } else {
        fz_ac_scene_learn_finish(app);
    }
}

void fz_ac_scene_learn_on_enter(void* context) {
    FzAcApp* app = context;

    app->learn_index = 0;
    ac_remote_reset(&app->staged);
    ac_ir_signal_reset(&app->capture);

    learn_view_set_callback(app->learn_view, fz_ac_scene_learn_view_callback, app);
    fz_ac_scene_learn_show_current(app);

    app->rx_worker = infrared_worker_alloc();
    infrared_worker_rx_set_received_signal_callback(
        app->rx_worker, fz_ac_scene_learn_rx_callback, app);
    infrared_worker_rx_enable_signal_decoding(app->rx_worker, true);
    infrared_worker_rx_enable_blink_on_receiving(app->rx_worker, true);
    fz_ac_scene_learn_rx_start(app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewLearn);
}

bool fz_ac_scene_learn_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        uint16_t type = FZ_AC_EVENT_TYPE(event.event);
        switch(type) {
        case FzAcCustomEventTypeIrCaptured:
            fz_ac_scene_learn_rx_stop(app);
            furi_mutex_acquire(app->signal_mutex, FuriWaitForever);
            ac_ir_signal_describe(&app->capture, app->str);
            furi_mutex_release(app->signal_mutex);
            learn_view_set_captured(app->learn_view, true, furi_string_get_cstr(app->str));
            consumed = true;
            break;
        case FzAcCustomEventTypeLearnOk:
            furi_mutex_acquire(app->signal_mutex, FuriWaitForever);
            ac_ir_signal_move(&app->staged.signals[app->learn_index], &app->capture);
            furi_mutex_release(app->signal_mutex);
            fz_ac_scene_learn_advance(app);
            consumed = true;
            break;
        case FzAcCustomEventTypeLearnSkip:
            fz_ac_scene_learn_rx_stop(app);
            ac_ir_signal_reset(&app->staged.signals[app->learn_index]);
            fz_ac_scene_learn_advance(app);
            consumed = true;
            break;
        case FzAcCustomEventTypeLearnRetry:
            furi_mutex_acquire(app->signal_mutex, FuriWaitForever);
            ac_ir_signal_reset(&app->capture);
            furi_mutex_release(app->signal_mutex);
            learn_view_set_captured(app->learn_view, false, "");
            fz_ac_scene_learn_rx_start(app);
            consumed = true;
            break;
        default:
            break;
        }
    }
    return consumed;
}

void fz_ac_scene_learn_on_exit(void* context) {
    FzAcApp* app = context;
    fz_ac_scene_learn_rx_stop(app);
    infrared_worker_free(app->rx_worker);
    app->rx_worker = NULL;
    furi_mutex_acquire(app->signal_mutex, FuriWaitForever);
    ac_ir_signal_reset(&app->capture);
    furi_mutex_release(app->signal_mutex);
    ac_remote_reset(&app->staged);
}
