#include "../fz_ac_app_i.h"

static void fz_ac_scene_sweep_view_callback(void* context, SweepViewEvent event) {
    FzAcApp* app = context;
    FzAcCustomEventType type;
    switch(event) {
    case SweepViewEventDone:
        type = FzAcCustomEventTypeLearnOk;
        break;
    case SweepViewEventUndo:
        type = FzAcCustomEventTypeLearnRetry;
        break;
    case SweepViewEventUp:
        type = FzAcCustomEventTypeSweepUp;
        break;
    default:
        type = FzAcCustomEventTypeSweepDown;
        break;
    }
    view_dispatcher_send_custom_event(app->view_dispatcher, FZ_AC_EVENT(type, 0));
}

static void fz_ac_scene_sweep_reset(FzAcApp* app) {
    for(size_t i = 0; i < AC_SWEEP_MAX; i++) ac_ir_signal_reset(&app->sweep[i]);
    app->sweep_count = 0;
}

static void fz_ac_scene_sweep_update(FzAcApp* app) {
    sweep_view_update(app->sweep_view, app->preset_buf, app->sweep_temp_start, app->sweep_count);
}

static void fz_ac_scene_sweep_done(FzAcApp* app) {
    if(app->sweep_count == 0) {
        notification_message(app->notifications, &sequence_blink_red_100);
        return;
    }
    fz_ac_rx_stop(app);

    char path[FZ_AC_PATH_LEN];
    fz_ac_ac_path(app->learn_target, path, sizeof(path));
    bool saved = ac_smart_write_preset(
        app->storage,
        path,
        app->smart_creating,
        &app->off_capture,
        app->preset_buf,
        app->sweep_temp_start,
        app->sweep,
        app->sweep_count);

    if(saved) {
        app->smart_creating = false;
        ac_ir_signal_reset(&app->off_capture);
        fz_ac_scene_sweep_reset(app);
        notification_message(app->notifications, &sequence_success);
        scene_manager_next_scene(app->scene_manager, FzAcSceneSmartMore);
    } else {
        notification_message(app->notifications, &sequence_error);
        fz_ac_rx_start(app);
    }
}

void fz_ac_scene_sweep_on_enter(void* context) {
    FzAcApp* app = context;

    fz_ac_scene_sweep_reset(app);
    sweep_view_set_callback(app->sweep_view, fz_ac_scene_sweep_view_callback, app);
    fz_ac_scene_sweep_update(app);

    fz_ac_rx_alloc(app);
    fz_ac_rx_start(app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewSweep);
}

bool fz_ac_scene_sweep_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        uint16_t type = FZ_AC_EVENT_TYPE(event.event);
        switch(type) {
        case FzAcCustomEventTypeIrCaptured:
            furi_mutex_acquire(app->signal_mutex, FuriWaitForever);
            if(app->sweep_count < AC_SWEEP_MAX &&
               app->sweep_temp_start + app->sweep_count < AC_TEMP_BASE + AC_TEMP_SLOTS) {
                ac_ir_signal_move(&app->sweep[app->sweep_count], &app->capture);
                app->sweep_count++;
            } else {
                ac_ir_signal_reset(&app->capture);
                notification_message(app->notifications, &sequence_blink_red_100);
            }
            furi_mutex_release(app->signal_mutex);
            fz_ac_scene_sweep_update(app);
            consumed = true;
            break;
        case FzAcCustomEventTypeLearnOk:
            fz_ac_scene_sweep_done(app);
            consumed = true;
            break;
        case FzAcCustomEventTypeLearnRetry:
            if(app->sweep_count > 0) {
                app->sweep_count--;
                ac_ir_signal_reset(&app->sweep[app->sweep_count]);
                fz_ac_scene_sweep_update(app);
            }
            consumed = true;
            break;
        case FzAcCustomEventTypeSweepUp:
            if(app->sweep_temp_start + app->sweep_count < AC_TEMP_BASE + AC_TEMP_SLOTS - 1) {
                app->sweep_temp_start++;
                fz_ac_scene_sweep_update(app);
            }
            consumed = true;
            break;
        case FzAcCustomEventTypeSweepDown:
            if(app->sweep_temp_start > AC_TEMP_BASE) {
                app->sweep_temp_start--;
                fz_ac_scene_sweep_update(app);
            }
            consumed = true;
            break;
        default:
            break;
        }
    }
    return consumed;
}

void fz_ac_scene_sweep_on_exit(void* context) {
    FzAcApp* app = context;
    fz_ac_rx_free(app);
    fz_ac_scene_sweep_reset(app);
}
