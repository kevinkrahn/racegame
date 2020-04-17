#pragma once

#include "../math.h"
#include <string>

class EditorMode
{
    std::string name;

protected:
    class Editor* editor;

public:
    EditorMode(std::string name) : name(name) {}
    void setEditor(Editor* editor) { this->editor = editor; }
    virtual ~EditorMode() {}
    virtual void onUpdate(class Scene* scene, class Renderer* renderer, f32 deltaTime) = 0;
    virtual void onEditorTabGui(class Scene* scene, class Renderer* renderer, f32 deltaTime) = 0;
    virtual void onBeginTest(class Scene* scene) {}
    virtual void onEndTest(class Scene* scene) {}
    virtual void onSwitchTo(class Scene* scene) {}
    virtual void onSwitchFrom(class Scene* scene) {}
    std::string const& getName() const { return name; }
};
