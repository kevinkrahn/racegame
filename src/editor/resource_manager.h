#pragma once

#include "../math.h"
#include "../imgui.h"
#include "../renderer.h"
#include "../input.h"
#include "../util.h"
#include "editor.h"

class ResourceManager
{
    bool isResourceWindowOpen = true;
    bool isTextureWindowOpen = false;
    Texture* selectedTexture;
    Editor editor;

    std::string editName;

    std::vector<Texture*> textures;
    bool texturesStale = true;

    enum struct EditorType
    {
        NONE,
        SCENE,
    } activeEditor = EditorType::NONE;

    void newTexture(std::string const& filename="");
    void showTextureWindow(Renderer* renderer, f32 deltaTime);

    void drawFile(FileItem& file);

public:
    ResourceManager();
    void onUpdate(Renderer* renderer, f32 deltaTime);
};
