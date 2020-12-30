#pragma once

#include "../math.h"

class EditorMode
{
    const char* name;

protected:
    class TrackEditor* editor;

public:
    EditorMode(const char* name) : name(name) {}
    void setEditor(TrackEditor* editor) { this->editor = editor; }
    virtual ~EditorMode() {}
    virtual void onUpdate(class Scene* scene, class Renderer* renderer, f32 deltaTime) = 0;
    virtual void onEditorTabGui(class Scene* scene, class Renderer* renderer, f32 deltaTime) = 0;
    virtual void onBeginTest(class Scene* scene) {}
    virtual void onEndTest(class Scene* scene) {}
    virtual void onSwitchTo(class Scene* scene) {}
    virtual void onSwitchFrom(class Scene* scene) {}
    const char* getName() const { return name; }
};
