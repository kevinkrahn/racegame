#pragma once

#include "../misc.h"
#include "../math.h"
#include "../entity.h"
#include "../texture.h"
#include "../renderer.h"
#include "editor_mode.h"
#include "resource_editor.h"

// TODO: Implement grid snapping in all edit modes
struct GridSettings
{
    bool show = false;
    bool snap = false;
    f32 cellSize = 4.f;
    f32 z = 2.f;
};

class TrackEditor : public ResourceEditor
{
    Resource* resource;
    SmallArray<OwnedPtr<EditorMode>> modes;
    u32 activeModeIndex = 0;
    GridSettings gridSettings;

public:
    ~TrackEditor();
    void init(class Resource* resource) override;
    void onUpdate(class Resource* r, class ResourceManager* rm, class Renderer* renderer, f32 deltaTime, u32 n) override;
    GridSettings& getGridSettings() { return gridSettings; }
    bool wantsExclusiveScreen() override;
};
