#pragma once

#include "../math.h"
#include "../imgui.h"
#include "../renderer.h"
#include "../input.h"
#include "editor.h"

class ResourceManager
{
    std::vector<std::string> tracks;
    Editor editor;

    enum struct EditorType
    {
        NONE,
        SCENE,
    } activeEditor = EditorType::NONE;

public:
    ResourceManager();
    void onUpdate(Renderer* renderer,f32 deltaTime);
};
