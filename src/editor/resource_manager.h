#pragma once

#include "../math.h"
#include "../imgui.h"
#include "../renderer.h"
#include "../input.h"
#include "../util.h"
#include "editor.h"
#include "model_editor.h"

class ResourceManager
{
    bool isResourceWindowOpen = true;
    std::string editName;
    ResourceType activeEditor;

    Editor trackEditor;
    ModelEditor modelEditor;

    std::vector<Texture*> textures;
    bool texturesStale = true;
    Texture* selectedTexture = nullptr;
    bool isTextureWindowOpen = false;

    std::vector<DataFile::Value*> tracks;
    bool tracksStale = true;

    std::vector<Model*> models;
    bool modelsStale = true;

    void newModel();
    void newTexture();
    void showTextureWindow(Renderer* renderer, f32 deltaTime);

public:
    ResourceManager();
    void onUpdate(Renderer* renderer, f32 deltaTime);
};
