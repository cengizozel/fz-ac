#pragma once

#include <gui/scene_manager.h>

// Generate scene id enum
#define ADD_SCENE(prefix, name, id) FzAcScene##id,
typedef enum {
#include "fz_ac_scene_config.h"
    FzAcSceneNum,
} FzAcScene;
#undef ADD_SCENE

extern const SceneManagerHandlers fz_ac_scene_handlers;

// Generate scene on_enter handler declarations
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_enter(void*);
#include "fz_ac_scene_config.h"
#undef ADD_SCENE

// Generate scene on_event handler declarations
#define ADD_SCENE(prefix, name, id) \
    bool prefix##_scene_##name##_on_event(void* context, SceneManagerEvent event);
#include "fz_ac_scene_config.h"
#undef ADD_SCENE

// Generate scene on_exit handler declarations
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_exit(void* context);
#include "fz_ac_scene_config.h"
#undef ADD_SCENE
