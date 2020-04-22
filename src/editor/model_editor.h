#pragma once

#include "../math.h"
#include "../datafile.h"
#include "../util.h"
#include "../model.h"

class ModelEditor
{
    Model* model;
    void loadBlenderFile(std::string const& filename);
    void processBlenderData();
    std::vector<std::string> sceneChoices;
    DataFile::Value blenderData;

public:
    ModelEditor();
    void onUpdate(class Renderer* renderer, f32 deltaTime);
    void setModel(Model* model) { this->model = model; }
    Model* getCurrentModel() const { return model; }
};
