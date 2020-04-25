#include "model_editor.h"
#include "../imgui.h"
#include "../renderer.h"
#include "../game.h"
#include "../mesh_renderables.h"
#include "../collision_flags.h"
#include <filesystem>

ModelEditor::ModelEditor()
{
}

PxFilterFlags physxFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{ return PxFilterFlags(0); }

void ModelEditor::setModel(Model* model)
{
    this->model = model;
    selectedObjects.clear();

    if (physicsScene)
    {
        physicsScene->release();
        body->release();
    }

    PxSceneDesc sceneDesc(g_game.physx.physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.f, 0.f, -15.f);
    sceneDesc.cpuDispatcher = g_game.physx.dispatcher;
    sceneDesc.filterShader = physxFilterShader;
    sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
    sceneDesc.solverType = PxSolverType::eTGS;
    physicsScene = g_game.physx.physics->createScene(sceneDesc);
    body = g_game.physx.physics->createRigidStatic(PxTransform(PxIdentity));

    for (auto& obj : model->objects)
    {
        body->attachShape(*obj.mousePickShape);
    }

    physicsScene->addActor(*body);
}

void ModelEditor::showSceneSelection()
{
    if (ImGui::BeginPopupModal("Blender Import", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("This blender file contains multiple scenes. Import which scene?");
        ImGui::Spacing();
        ImGui::Spacing();
        auto& scenes = blenderData.dict().val()["scenes"].array().val();
        for (auto& val : scenes)
        {
            if (ImGui::Selectable(val.dict().val()["name"].string().val().c_str()))
            {
                model->sourceSceneName = val.dict().val()["name"].string().val();
                processBlenderData();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::Spacing();
        ImGui::Spacing();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void ModelEditor::onUpdate(Renderer* renderer, f32 deltaTime)
{
    ImGui::Begin("Model Editor");
    // TODO: Add keyboard shortcut
    if (ImGui::Button("Save"))
    {
        // TODO: mark resource_manager modelsStale = true
        saveResource(*model);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load File"))
    {
        std::string path = chooseFile("", true, "Model Files", { "*.blend" });
        if (!path.empty())
        {
            model->sourceFilePath = std::filesystem::relative(path);
            model->sourceSceneName = "";
            loadBlenderFile(model->sourceFilePath);
        }
    }
    showSceneSelection();
    ImGui::SameLine();
    if (ImGui::Button("Reimport"))
    {
        if (!model->sourceFilePath.empty())
        {
            loadBlenderFile(model->sourceFilePath);
        }
    }

    ImGui::Gap();
    if (!model->sourceFilePath.empty())
    {
        ImGui::Text(model->sourceFilePath.c_str());
        ImGui::Text(tstr("Scene: ", model->sourceSceneName.c_str()));
    }
    ImGui::InputText("Name", &model->name);
    ImGui::Checkbox("Show Grid", &showGrid);
    ImGui::Checkbox("Show Floor", &showFloor);

    ImGui::Gap();
    ImGui::Combo("Usage", (i32*)&model->modelUsage,
            "Internal\0Static Prop\0Dynamic Prop\0Spline\0Vehicle\0");
    if (model->modelUsage == ModelUsage::DYNAMIC_PROP)
    {
        ImGui::InputFloat("Density", &model->density);
    }

    ImGui::Gap();

    if (ImGui::BeginChild("Objects", {0,0}, true))
    {
        for (u32 i=0; i<model->objects.size(); ++i)
        {
            auto it = std::find_if(selectedObjects.begin(), selectedObjects.end(),
                    [&](u32 index){ return index == i; });
            if (ImGui::Selectable(model->objects[i].name.c_str(), it != selectedObjects.end()))
            {
                if (!g_input.isKeyDown(KEY_LCTRL) && !g_input.isKeyDown(KEY_LSHIFT))
                {
                    selectedObjects.clear();
                    selectedObjects.push_back(i);
                }
                else
                {
                    if (it == selectedObjects.end() && !g_input.isKeyDown(KEY_LSHIFT))
                    {
                        selectedObjects.push_back(i);
                    }
                    if (it != selectedObjects.end() && !g_input.isKeyDown(KEY_LCTRL))
                    {
                        selectedObjects.erase(it);
                    }
                }
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();

    if (selectedObjects.size() > 0)
    {
        ModelObject& obj = model->objects[selectedObjects[0]];
        if (ImGui::Begin("Object Properties"))
        {
            ImGui::Checkbox("Is Collider", &obj.isCollider);
            ImGui::Checkbox("Is Visible", &obj.isVisible);

            if (ImGui::BeginCombo("Material",
                    obj.materialGuid? g_res.getMaterial(obj.materialGuid)->name.c_str() : "None"))
            {
                for (auto& mat : g_res.materials)
                {
                    if (ImGui::Selectable(mat.second->name.c_str()))
                    {
                        obj.materialGuid = mat.first;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::End();
        }
    }

    // end gui

    // this seems to be necessary for debug visualization
    physicsScene->simulate(deltaTime);
    physicsScene->fetchResults(true);

    camera.update(deltaTime, renderer->getRenderWorld());

    // draw grid
    if (showGrid)
    {
        f32 cellSize = 4.f;
        i32 count = (i32)(40.f / cellSize);
        f32 gridSize = count * cellSize;
        glm::vec4 color = { 1.f, 1.f, 1.f, 0.2f };
        glm::vec3 gridPos = { 0, 0, 0 };
        for (i32 i=-count; i<=count; i++)
        {
            f32 x = i * cellSize;
            f32 y = i * cellSize;
            debugDraw.line(
                { snap(gridPos.x + x, cellSize), snap(gridPos.y - gridSize, cellSize), gridPos.z },
                { snap(gridPos.x + x, cellSize), snap(gridPos.y + gridSize, cellSize), gridPos.z },
                color, color);
            debugDraw.line(
                { snap(gridPos.x - gridSize, cellSize), snap(gridPos.y + y, cellSize), gridPos.z },
                { snap(gridPos.x + gridSize, cellSize), snap(gridPos.y + y, cellSize), gridPos.z },
                color, color);
        }
    }

    if (g_game.isPhysicsDebugVisualizationEnabled)
    {
        physicsScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
        physicsScene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 2.0f);
        physicsScene->setVisualizationParameter(PxVisualizationParameter::eBODY_MASS_AXES, 1.0f);
        physicsScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 2.0f);
        physicsScene->setVisualizationCullingBox(PxBounds3({-1000, -1000, -1000}, {1000, 1000, 1000}));
        const PxRenderBuffer& rb = physicsScene->getRenderBuffer();
        for(PxU32 i=0; i < rb.getNbLines(); ++i)
        {
            const PxDebugLine& line = rb.getLines()[i];
            debugDraw.line(convert(line.pos0), convert(line.pos1), rgbaFromU32(line.color0), rgbaFromU32(line.color1));
        }
    }
    else
    {
        physicsScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 0.0f);
    }

    RenderWorld* rw = renderer->getRenderWorld();
    glm::vec3 rayDir = camera.getMouseRay(rw);
    Camera const& cam = camera.getCamera();
    rw->updateWorldTime(g_game.currentTime);

    if (!ImGui::GetIO().WantCaptureMouse && g_input.isMouseButtonPressed(MOUSE_LEFT))
    {
        if (!g_input.isKeyDown(KEY_LCTRL) && !g_input.isKeyDown(KEY_LSHIFT))
        {
            selectedObjects.clear();
        }

        PxRaycastBuffer hit;
        PxQueryFilterData filter;
        filter.data = PxFilterData(COLLISION_FLAG_SELECTABLE, 0, 0, 0);
        if (physicsScene->raycast(convert(cam.position), convert(rayDir), 10000.f, hit,
                PxHitFlags(PxHitFlag::eDEFAULT), filter))
        {
            assert(hit.block.actor == body);
            for (u32 i=0; i<model->objects.size(); ++i)
            {
                if (model->objects[i].mousePickShape == hit.block.shape)
                {
                    auto it = std::find_if(selectedObjects.begin(), selectedObjects.end(),
                            [&](u32 index){ return index == i; });
                    if (it == selectedObjects.end())
                    {
                        selectedObjects.push_back(i);
                    }
                    else if (g_input.isKeyDown(KEY_LSHIFT))
                    {
                        selectedObjects.erase(it);
                    }
                    break;
                }
            }
        }
    }

    if (showFloor)
    {
        rw->push(LitRenderable(g_res.getMesh("world.Quad"),
                    glm::scale(glm::mat4(1.f), glm::vec3(40.f)), nullptr, glm::vec3(0.1f)));
    }

    for (u32 i=0; i<(u32)model->objects.size(); ++i)
    {
        auto& obj = model->objects[i];
        if (obj.isVisible)
        {
            rw->push(LitMaterialRenderable(&model->meshes[obj.meshIndex], obj.getTransform(),
                        g_res.getMaterial(obj.materialGuid)));
        }
        if (obj.isCollider)
        {
            rw->push(WireframeRenderable(&model->meshes[obj.meshIndex], obj.getTransform(),
                        { 1.f, 1.f, 0.1f, 0.5f }));
        }
    }
    for (u32 selectedIndex : selectedObjects)
    {
        auto& obj = model->objects[selectedIndex];
        rw->push(WireframeRenderable(&model->meshes[obj.meshIndex], obj.getTransform()));
    }

    rw->add(&debugDraw);
}

void ModelEditor::loadBlenderFile(std::string const& filename)
{
    // execute blender with a python script that will output the data in the datafile format
    const char* blenderPath =
#if _WIN32
    "\"C:/Program Files/Blender Foundation/Blender 2.81/blender.exe\""
#else
    "blender";
#endif
    CommandResult r = runShellCommand(str(blenderPath,
            " -b ", filename,
            " -P ../blender_exporter.py --enable-autoexec"));

    if (r.output.find("Saved to file: ") == std::string::npos)
    {
        error("Failed to import blender file:\n");
        error(r.output);
        // TODO: Show error message box
        return;
    }

    const char* outputFile = "blender_output.dat";
    DataFile::Value val = DataFile::load(outputFile);

    // meshes
    if (!val.dict().hasValue())
    {
        error("Failed to import blender file: Unexpected data structure.\n");
        // TODO: Show error message box
        return;
    }

    // scenes
    if (!val.dict().val()["scenes"].array().hasValue()
            || val.dict().val()["scenes"].array().val().empty())
    {
        error("Failed to import blender file: No scenes found in file.\n");
        // TODO: Show error message box
        return;
    }

    blenderData = std::move(val);
    if (model->sourceSceneName.empty() && blenderData.dict().val()["scenes"].array().val().size() > 1)
    {
        print("Blender file contains multiple scenes. There are choices.\n");
        ImGui::OpenPopup("Blender Import");
    }
    else
    {
        if (model->sourceSceneName.empty())
        {
            model->sourceSceneName =
                blenderData.dict().val()["scenes"].array().val().front().dict().val()["name"].string().val();
        }
        processBlenderData();
    }
}

void ModelEditor::processBlenderData()
{
    auto& dict = blenderData.dict().val();
    auto& scenes = dict["scenes"].array().val();
    DataFile::Value::Dict* scenePtr = nullptr;
    for (auto& sceneData : scenes)
    {
        if (sceneData.dict().val()["name"].string().val() == model->sourceSceneName)
        {
            scenePtr = &sceneData.dict().val();
            break;
        }
    }
    auto& scene = *scenePtr;

    if (model->name.empty() || model->name.substr(0, strlen("Model")) == "Model")
    {
        model->name = scene["name"].string().val();
    }

    u32 meshCount = 0;
    std::map<std::string, u32> meshesToLoad;
    auto& objects = scene["objects"].array().val();
    for (auto& mesh : model->meshes)
    {
        mesh.destroy();
    }
    model->meshes.clear();
    std::vector<ModelObject> oldObjects = std::move(model->objects);
    model->objects.clear();
    auto& meshDict = dict["meshes"].dict().val();
    for (auto& object : objects)
    {
        auto& obj = object.dict().val();
        auto& meshName = obj["data_name"].string().val();
        auto meshIt = meshesToLoad.find(meshName);
        if (meshIt == meshesToLoad.end())
        {
            meshesToLoad[meshName] = meshCount++;
            meshIt = meshesToLoad.find(meshName);
            Mesh mesh;
            Serializer s(meshDict[meshName], true);
            mesh.serialize(s);
            model->meshes.push_back(std::move(mesh));
        }

        ModelObject newObj;
        // handle reimport
        ModelObject* modelObj = &newObj;
        for (auto& oldObj : oldObjects)
        {
            if (oldObj.name == obj["name"].string().val())
            {
                modelObj = &oldObj;
                break;
            }
        }

        modelObj->meshIndex = meshIt->second;
        modelObj->name = std::move(obj["name"]).string().val();
        glm::mat4 matrix = obj["matrix"].convertBytes<glm::mat4>().val();
        modelObj->position = translationOf(matrix);
        modelObj->rotation = glm::quat_cast(glm::mat3(rotationOf(matrix)));
        modelObj->scale = scaleOf(matrix);
        if (modelObj->name.find("Collision") != std::string::npos
            || modelObj->name.find("collision") != std::string::npos)
        {
            modelObj->isCollider = true;
            modelObj->isVisible = false;
        }

        model->objects.push_back(std::move(*modelObj));
    }
    for (auto& obj : model->objects)
    {
        obj.createMousePickCollisionShape(model);
        body->attachShape(*obj.mousePickShape);
    }
}
