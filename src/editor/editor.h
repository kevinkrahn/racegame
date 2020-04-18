#pragma once

#include "../misc.h"
#include "../math.h"
#include "../entity.h"
#include "../texture.h"
#include "../renderer.h"
#include "editor_mode.h"

struct GridSettings
{
    bool show = false;
    bool snap = false;
    f32 cellSize = 4.f;
    f32 z = 2.f;
};

class Editor
{
    std::vector<std::unique_ptr<EditorMode>> modes;
    EditorMode* activeMode = nullptr;
    GridSettings gridSettings;

public:
    Editor();
    void onUpdate(class Scene* scene, class Renderer* renderer, f32 deltaTime);
    GridSettings& getGridSettings() { return gridSettings; }
};

std::string chooseFile(const char* defaultSelection, bool open);
