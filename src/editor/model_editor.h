#pragma once

#include "../math.h"
#include "../datafile.h"
#include "../util.h"
#include "../model.h"
#include "../debug_draw.h"
#include "editor_camera.h"

class ModelEditor
{
    Model* model;
    void loadBlenderFile(std::string const& filename);
    void processBlenderData();
    DataFile::Value blenderData;
    std::vector<u32> selectedObjects;
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
    void onUpdate(class Renderer* renderer, f32 deltaTime);
    void setModel(Model* model);
    Model* getCurrentModel() const { return model; }
};
