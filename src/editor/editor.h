#pragma once

#include "../misc.h"
#include "../math.h"
#include "../entity.h"
#include "../texture.h"
#include "../renderer.h"
#include "editor_mode.h"

// TODO: Implement grid snapping in all edit modes
struct GridSettings
{
    bool show = false;
    bool snap = false;
    f32 cellSize = 4.f;
    f32 z = 2.f;
};

class Editor
{
    SmallArray<OwnedPtr<EditorMode>> modes;
    u32 activeModeIndex = 0;
    GridSettings gridSettings;

public:
    Editor();
    void onUpdate(class Scene* scene, class Renderer* renderer, f32 deltaTime);
    GridSettings& getGridSettings() { return gridSettings; }
    void onEndTestDrive(Scene* scene);
    void reset();
};
