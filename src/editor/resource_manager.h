#pragma once

#include "../math.h"
#include "../imgui.h"
#include "../renderer.h"
#include "../input.h"
#include "../util.h"
#include "editor.h"

class ResourceManager
{
    std::vector<FileItem> files;
    bool filesStale = true;

    bool isResourceWindowOpen = true;
    bool isTextureWindowOpen = false;
    std::vector<Texture> textures;
    u32 textureViewIndex = 0;
    Editor editor;

    enum struct EditorType
    {
        NONE,
        SCENE,
    } activeEditor = EditorType::NONE;

    void showTextureWindow(Renderer* renderer, f32 deltaTime);

    void drawFile(FileItem& file);

public:
    ResourceManager();
    void onUpdate(Renderer* renderer, f32 deltaTime);
};
