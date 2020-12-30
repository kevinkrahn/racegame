#pragma once

#include "../math.h"
#include "../datafile.h"
#include "../util.h"
#include "../model.h"
#include "../debug_draw.h"
#include "editor_camera.h"
#include "resource_editor.h"

class ModelEditor : public ResourceEditor
{
    Model* model;
    void loadBlenderFile(const char* filename);
    void processBlenderData();
    DataFile::Value blenderData;
    Array<u32> selectedObjects;
    EditorCamera camera;
    bool showGrid = true;
    bool showFloor = false;
    bool showBoundingBox = false;
    bool showColliders = false;
    DebugDraw debugDraw;

#if 0
    PxScene* physicsScene = nullptr;
    PxRigidStatic* body = nullptr;
#endif

    bool selectionStateCtrl = false;
    bool selectionStateShift = false;

    void showSceneSelection();

public:
    ModelEditor();
    void init(class Resource* resource) override;
    void onUpdate(class Resource* r, class ResourceManager* rm, class Renderer* renderer, f32 deltaTime, u32 n) override;
    Model* getCurrentModel() const { return model; }
};
