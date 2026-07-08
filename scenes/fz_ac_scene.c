#include "fz_ac_scene.h"

// Generate scene on_enter handlers array
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
static void (*const fz_ac_on_enter_handlers[])(void*) = {
#include "fz_ac_scene_config.h"
};
#undef ADD_SCENE

// Generate scene on_event handlers array
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
static bool (*const fz_ac_on_event_handlers[])(void* context, SceneManagerEvent event) = {
#include "fz_ac_scene_config.h"
};
#undef ADD_SCENE

// Generate scene on_exit handlers array
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
static void (*const fz_ac_on_exit_handlers[])(void* context) = {
#include "fz_ac_scene_config.h"
};
#undef ADD_SCENE

const SceneManagerHandlers fz_ac_scene_handlers = {
    .on_enter_handlers = fz_ac_on_enter_handlers,
    .on_event_handlers = fz_ac_on_event_handlers,
    .on_exit_handlers = fz_ac_on_exit_handlers,
    .scene_num = FzAcSceneNum,
};
