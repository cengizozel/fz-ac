#include "../fz_ac_app_i.h"

#include "fz_ac_icons.h"
#include <stdio.h>
#include <string.h>

enum {
    RemoteLabelTitle = 100,
    RemoteLabelTemperature,
    RemoteLabelPreset,
    RemoteLabelPresetPos,
};

typedef struct {
    const Icon* icon;
    const Icon* icon_hover;
    uint16_t matrix_x;
    uint16_t matrix_y;
    uint16_t x;
    uint16_t y;
} RemoteButtonLayout;

static const RemoteButtonLayout remote_layout[AC_BUTTON_COUNT] = {
    [AcButtonOff] = {&I_off_19x20, &I_off_hover_19x20, 0, 0, 6, 17},
    [AcButtonMode] = {&I_cold_19x20, &I_cold_hover_19x20, 1, 0, 39, 17},
    [AcButtonTempUp] = {&I_tempup_24x21, &I_tempup_hover_24x21, 0, 1, 3, 51},
    [AcButtonTempDown] = {&I_tempdown_24x21, &I_tempdown_hover_24x21, 0, 2, 3, 93},
    [AcButtonFan] = {&I_fan_silent_19x20, &I_fan_silent_hover_19x20, 1, 1, 39, 54},
    [AcButtonVane] = {&I_vane_h3_19x20, &I_vane_h3_hover_19x20, 1, 2, 39, 91},
};

static void fz_ac_scene_remote_button_callback(void* context, uint32_t index) {
    FzAcApp* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, FZ_AC_EVENT(FzAcCustomEventTypeButtonSelected, index));
}

static void fz_ac_scene_remote_add_button(FzAcApp* app, AcButton button) {
    const RemoteButtonLayout* layout = &remote_layout[button];
    ac_remote_panel_add_item(
        app->panel,
        button,
        layout->matrix_x,
        layout->matrix_y,
        layout->x,
        layout->y,
        layout->icon,
        layout->icon_hover,
        fz_ac_scene_remote_button_callback,
        app);
}

static void fz_ac_scene_remote_update_smart_labels(FzAcApp* app) {
    if(app->smart_index.preset_count > 0) {
        const AcPresetInfo* preset = &app->smart_index.presets[app->smart_preset];
        snprintf(app->preset_label, sizeof(app->preset_label), "%.6s", preset->name);
        snprintf(
            app->preset_pos,
            sizeof(app->preset_pos),
            "%u/%u",
            app->smart_preset + 1,
            app->smart_index.preset_count);
        snprintf(app->temp_str, sizeof(app->temp_str), "%u", app->smart_temp);
    } else {
        snprintf(app->preset_label, sizeof(app->preset_label), "none");
        app->preset_pos[0] = '\0';
        snprintf(app->temp_str, sizeof(app->temp_str), "--");
    }
}

void fz_ac_scene_remote_on_enter(void* context) {
    FzAcApp* app = context;
    ACRemotePanel* panel = app->panel;
    bool smart = app->current_type == AcTypeSmart;

    ac_remote_panel_reserve(panel, 2, 3);
    fz_ac_scene_remote_add_button(app, AcButtonOff);
    fz_ac_scene_remote_add_button(app, AcButtonMode);
    fz_ac_scene_remote_add_button(app, AcButtonTempUp);
    fz_ac_scene_remote_add_button(app, AcButtonTempDown);
    if(!smart) {
        fz_ac_scene_remote_add_button(app, AcButtonFan);
        fz_ac_scene_remote_add_button(app, AcButtonVane);
    }

    ac_remote_panel_add_icon(panel, 9, 39, &I_off_text_14x5);
    ac_remote_panel_add_icon(panel, 39, 39, &I_mode_text_20x5);
    ac_remote_panel_add_icon(panel, 0, 63, &I_frame_30x39);
    if(!smart) {
        ac_remote_panel_add_icon(panel, 41, 76, &I_fan_text_14x5);
        ac_remote_panel_add_icon(panel, 38, 113, &I_vane_text_20x5);
    }

    if(app->current_ac >= 0 && app->current_ac < (int32_t)app->ac_count) {
        snprintf(app->title_buf, sizeof(app->title_buf), "%s", app->ac_names[app->current_ac]);
    } else {
        snprintf(app->title_buf, sizeof(app->title_buf), "AC remote");
    }
    ac_remote_panel_add_label(panel, RemoteLabelTitle, 2, 11, FontPrimary, app->title_buf);

    if(smart) {
        fz_ac_scene_remote_update_smart_labels(app);
        // current mode name and position sit right under the MODE button
        ac_remote_panel_add_label(
            panel, RemoteLabelPreset, 33, 51, FontSecondary, app->preset_label);
        ac_remote_panel_add_label(
            panel, RemoteLabelPresetPos, 39, 62, FontSecondary, app->preset_pos);
    } else {
        snprintf(app->temp_str, sizeof(app->temp_str), "%lu", app->temp_display);
    }
    ac_remote_panel_add_label(panel, RemoteLabelTemperature, 4, 86, FontKeyboard, app->temp_str);

    view_dispatcher_switch_to_view(app->view_dispatcher, FzAcViewPanel);
}

static void fz_ac_scene_remote_simple_press(FzAcApp* app, uint32_t button) {
    AcIrSignal* signal = &app->remote.signals[button];
    if(signal->present) {
        notification_message(app->notifications, &sequence_blink_white_100);
        ac_ir_signal_send(signal);
        if(button == AcButtonTempUp && app->temp_display < 31) {
            app->temp_display++;
        } else if(button == AcButtonTempDown && app->temp_display > 16) {
            app->temp_display--;
        }
        snprintf(app->temp_str, sizeof(app->temp_str), "%lu", app->temp_display);
        ac_remote_panel_label_set_string(app->panel, RemoteLabelTemperature, app->temp_str);
    } else {
        notification_message(app->notifications, &sequence_blink_red_100);
    }
}

static void fz_ac_scene_remote_smart_press(FzAcApp* app, uint32_t button) {
    if(button == AcButtonOff) {
        fz_ac_smart_send(app, NULL, 0);
        return;
    }
    if(app->smart_index.preset_count == 0) {
        notification_message(app->notifications, &sequence_blink_red_100);
        return;
    }

    AcPresetInfo* preset = &app->smart_index.presets[app->smart_preset];
    if(button == AcButtonMode) {
        app->smart_preset = (app->smart_preset + 1) % app->smart_index.preset_count;
        preset = &app->smart_index.presets[app->smart_preset];
        int32_t temp = ac_temp_bits_nearest(preset->temp_bits, app->smart_temp);
        if(temp < 0) {
            notification_message(app->notifications, &sequence_blink_red_100);
            return;
        }
        app->smart_temp = temp;
    } else if(button == AcButtonTempUp || button == AcButtonTempDown) {
        int32_t temp =
            ac_temp_bits_next(preset->temp_bits, app->smart_temp, button == AcButtonTempUp);
        if(temp < 0) {
            notification_message(app->notifications, &sequence_blink_red_100);
            return;
        }
        app->smart_temp = temp;
    } else {
        return;
    }

    fz_ac_scene_remote_update_smart_labels(app);
    ac_remote_panel_label_set_string(app->panel, RemoteLabelPreset, app->preset_label);
    ac_remote_panel_label_set_string(app->panel, RemoteLabelPresetPos, app->preset_pos);
    ac_remote_panel_label_set_string(app->panel, RemoteLabelTemperature, app->temp_str);
    fz_ac_smart_send(app, preset->name, app->smart_temp);
}

bool fz_ac_scene_remote_on_event(void* context, SceneManagerEvent event) {
    FzAcApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       FZ_AC_EVENT_TYPE(event.event) == FzAcCustomEventTypeButtonSelected) {
        uint32_t button = FZ_AC_EVENT_VALUE(event.event);
        if(button < AC_BUTTON_COUNT) {
            if(app->current_type == AcTypeSmart) {
                fz_ac_scene_remote_smart_press(app, button);
            } else {
                fz_ac_scene_remote_simple_press(app, button);
            }
        }
        consumed = true;
    }
    return consumed;
}

void fz_ac_scene_remote_on_exit(void* context) {
    FzAcApp* app = context;
    ac_remote_panel_reset(app->panel);
}
