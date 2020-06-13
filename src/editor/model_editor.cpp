#include "model_editor.h"
#include "../imgui.h"
#include "../renderer.h"
#include "../game.h"
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

#if 0
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
        body->attachShape(model->meshes[obj.mesh].getCollisionMesh());
    }

    physicsScene->addActor(*body);
#endif

    BoundingBox bb = model->getBoundingbox(glm::mat4(1.f));
    f32 height = bb.max.z - bb.min.z;
    camera.setCameraDistance(18 + height);
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
    bool dirty = false;
    if (ImGui::Begin("Model Editor"))
    {
        ImGui::PushItemWidth(150);
        if (ImGui::InputText("##Name", &model->name))
        {
            dirty = true;
            g_game.resourceManager->markDirty(model->guid);
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("Load File"))
        {
            std::string path =
                chooseFile( true, "Model Files", { "*.blend" }, str(ASSET_DIRECTORY, "/models"));
            if (!path.empty())
            {
                model->sourceFilePath = std::filesystem::relative(path).string();
                model->sourceSceneName = "";
                loadBlenderFile(model->sourceFilePath);
                dirty = true;
            }
        }
        showSceneSelection();
        ImGui::SameLine();
        if (ImGui::Button("Reimport"))
        {
            if (!model->sourceFilePath.empty())
            {
                loadBlenderFile(model->sourceFilePath);
                dirty = true;
            }
        }

        ImGui::Gap();
        if (!model->sourceFilePath.empty())
        {
            ImGui::Text(model->sourceFilePath.c_str());
            ImGui::Text(tstr("Scene: ", model->sourceSceneName.c_str()));
        }
        std::string guid = str("0x", std::hex, model->guid, std::dec);
        ImGui::TextDisabled("GUID: %s", guid.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Copy"))
        {
            ImGui::LogToClipboard();
            ImGui::LogText(guid.c_str());
            ImGui::LogFinish();
        }
        ImGui::Checkbox("Show Grid", &showGrid);
        ImGui::Checkbox("Show Floor", &showFloor);
        ImGui::Checkbox("Show Bounds", &showBoundingBox);
        ImGui::Checkbox("Show Colliders", &showColliders);

        ImGui::Gap();
        ImGui::Combo("Usage", (i32*)&model->modelUsage,
                "Internal\0Static Prop\0Dynamic Prop\0Spline\0Vehicle\0");
        if (model->modelUsage == ModelUsage::DYNAMIC_PROP)
        {
            dirty |= ImGui::InputFloat("Density", &model->density);
        }
        if (model->modelUsage == ModelUsage::STATIC_PROP || model->modelUsage == ModelUsage::DYNAMIC_PROP)
        {
            if (ImGui::BeginCombo("Category", propCategoryNames[(u32)model->category]))
            {
                for (u32 i=0; i<ARRAY_SIZE(propCategoryNames); ++i)
                {
                    if (ImGui::Selectable(propCategoryNames[i]))
                    {
                        model->category = (PropCategory)i;
                        dirty = true;
                    }
                }
                ImGui::EndCombo();
            }
        }

        ImGui::Gap();

        auto listObjects = [&](i32 filterCollectionIndex) {
            for (u32 i=0; i<(u32)model->objects.size(); ++i)
            {
                bool isInCollection = false;
                if (filterCollectionIndex == -1)
                {
                    if (model->objects[i].collectionIndexes.empty())
                    {
                        isInCollection = true;
                    }
                }
                else
                {
                    for (u32 collectionIndex : model->objects[i].collectionIndexes)
                    {
                        if ((i32)collectionIndex == filterCollectionIndex)
                        {
                            isInCollection = true;
                            break;
                        }
                    }
                }
                if (!isInCollection)
                {
                    continue;
                }

                auto it = selectedObjects.find(i);
                if (ImGui::Selectable(model->objects[i].name.c_str(), !!it))
                {
                    if (!g_input.isKeyDown(KEY_LCTRL) && !g_input.isKeyDown(KEY_LSHIFT))
                    {
                        selectedObjects.clear();
                        selectedObjects.push_back(i);
                    }
                    else
                    {
                        if (it && !g_input.isKeyDown(KEY_LSHIFT))
                        {
                            selectedObjects.push_back(i);
                        }
                        if (!it && !g_input.isKeyDown(KEY_LCTRL))
                        {
                            selectedObjects.erase(it);
                        }
                    }
                }
            }
        };

        if (ImGui::BeginChild("Objects", {0,0}, true))
        {
            if (model->collections.empty())
            {
                if (ImGui::CollapsingHeader("Default Collection", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    listObjects(-1);
                }
            }
            else
            {
                for (i32 i=0; i<(i32)model->collections.size(); ++i)
                {
                    if (ImGui::CollapsingHeader(model->collections[i].name.c_str()))
                    {
                        listObjects(i);
                    }
                }
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();

    if (selectedObjects.size() > 0)
    {
        ModelObject& obj = model->objects[selectedObjects[0]];
        if (ImGui::Begin("Object Properties"))
        {
            if (ImGui::Checkbox("Is Collider", &obj.isCollider))
            {
                for (u32 index : selectedObjects)
                {
                    model->objects[index].isCollider = obj.isCollider;
                }
                dirty = true;
            }

            if (ImGui::Checkbox("Is Visible", &obj.isVisible))
            {
                for (u32 index : selectedObjects)
                {
                    model->objects[index].isVisible = obj.isVisible;
                }
                dirty = true;
            }

            /*
            if (model->modelUsage == ModelUsage::VEHICLE)
            {
                if (ImGui::Checkbox("Is Paint", &obj.isPaint))
                {
                    for (u32 index : selectedObjects)
                    {
                        model->objects[index].isPaint = obj.isPaint;
                    }
                }
                dirty = true;
            }
            */

            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);
            ImVec2 size = { ImGui::GetWindowWidth() * 0.65f, 300.f };
            ImGui::SetNextWindowSizeConstraints(size, size);
            if (ImGui::BeginCombo("Material",
                    obj.materialGuid ? g_res.getMaterial(obj.materialGuid)->name.c_str() : "None"))
            {
                dirty = true;

                static std::string searchString;
                static Array<Material*> searchResults;
                searchResults.clear();

                if (ImGui::IsWindowAppearing())
                {
                    ImGui::SetKeyboardFocusHere();
                    searchString = "";
                }
                ImGui::PushItemWidth(ImGui::GetWindowWidth() - 32);
                bool enterPressed = ImGui::InputText("", &searchString, ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("!", ImVec2(16, 0)))
                {
                    if (g_game.resourceManager->getSelectedMaterial())
                    {
                        for (u32 index : selectedObjects)
                        {
                            model->objects[index].materialGuid =
                                g_game.resourceManager->getSelectedMaterial()->guid;
                        }
                        ImGui::CloseCurrentPopup();
                    }
                }

                g_res.iterateResourceType(ResourceType::MODEL, [&](Resource* res){
                    Material* mat = (Material*)res;
                    if (searchString.empty() || mat->name.find(searchString) != std::string::npos)
                    {
                        searchResults.push_back(mat);
                    }
                });
                searchResults.sort([](auto a, auto b) {
                    return a->name < b->name;
                });

                if (enterPressed)
                {
                    for (u32 index : selectedObjects)
                    {
                        model->objects[index].materialGuid = searchResults[0]->guid;
                    }
                    ImGui::CloseCurrentPopup();
                }

                if (ImGui::BeginChild("Search Results", { 0, 0 }))
                {
                    for (Material* mat : searchResults)
                    {
                        ImGui::PushID(mat->guid);
                        if (ImGui::Selectable(mat->name.c_str()))
                        {
                            for (u32 index : selectedObjects)
                            {
                                model->objects[index].materialGuid = mat->guid;
                            }
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndChild();
                }

                ImGui::EndCombo();
            }

            ImGui::End();
        }
    }

    if (dirty)
    {
        g_game.resourceManager->markDirty(model->guid);
    }

    // end gui

    camera.setNearFar(1.f, 140.f);
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

#if 0
    physicsScene->simulate(deltaTime);
    physicsScene->fetchResults(true);

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
#endif

    RenderWorld* rw = renderer->getRenderWorld();
    //glm::vec3 rayDir = camera.getMouseRay(rw);
    //Camera const& cam = camera.getCamera();
    rw->updateWorldTime(g_game.currentTime);
    //rw->setShadowBounds(model->getBoundingbox(glm::mat4(1.f)).expand(5.f));
    rw->setShadowBounds(BoundingBox{ glm::vec3(-50), glm::vec3(50) });

    if (const u32* pixelID = rw->getPickPixelResult())
    {
        if (!selectionStateCtrl && !selectionStateShift)
        {
            selectedObjects.clear();
        }

        for (u32 i=0; i<model->objects.size(); ++i)
        {
            if (i + 1 == *pixelID)
            {
                auto it = selectedObjects.find(i);
                if (!it)
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

    if (!ImGui::GetIO().WantCaptureMouse && g_input.isMouseButtonPressed(MOUSE_LEFT))
    {
        rw->pickPixel(g_input.getMousePosition()
                / glm::vec2(g_game.windowWidth, g_game.windowHeight));
        selectionStateCtrl = g_input.isKeyDown(KEY_LCTRL);
        selectionStateShift = g_input.isKeyDown(KEY_LSHIFT);
    }

    if (showFloor)
    {
        drawSimple(rw, g_res.getModel("misc")->getMeshByName("world.Quad"), &g_res.white,
                    glm::scale(glm::mat4(1.f), glm::vec3(40.f)), glm::vec3(0.1f));
    }

    for (u32 i=0; i<(u32)model->objects.size(); ++i)
    {
        auto& obj = model->objects[i];
        if (obj.isVisible)
        {
            g_res.getMaterial(obj.materialGuid)->draw(rw, obj.getTransform(), &model->meshes[obj.meshIndex]);
        }
        if (obj.isCollider && showColliders)
        {
            drawWireframe(rw, &model->meshes[obj.meshIndex], obj.getTransform(),
                        { 1.f, 1.f, 0.1f, 0.5f });
        }
    }
    for (u32 selectedIndex : selectedObjects)
    {
        // NOTE: first byte is reserved for hidden flag
        u8 selectIndexByte = (u8)((selectedIndex % 126) + 1) << 1;
        auto& obj = model->objects[selectedIndex];
        Material* material = g_res.getMaterial(obj.materialGuid);
        material->drawHighlight(rw, obj.getTransform(), &model->meshes[obj.meshIndex], selectIndexByte);
    }

    if (showBoundingBox)
    {
        BoundingBox bb = model->getBoundingbox(glm::mat4(1.f));

#if 0
        glm::vec3 points[] = {
            { bb.min.x, bb.min.y, bb.min.z },
            { bb.min.x, bb.max.y, bb.min.z },
            { bb.max.x, bb.min.y, bb.min.z },
            { bb.max.x, bb.max.y, bb.min.z },
            { bb.min.x, bb.min.y, bb.max.z },
            { bb.min.x, bb.max.y, bb.max.z },
            { bb.max.x, bb.min.y, bb.max.z },
            { bb.max.x, bb.max.y, bb.max.z },
        };

        bool allPointsVisible = true;
        for (auto& p : points)
        {
            glm::vec4 tp = rw->getCamera(0).viewProjection * glm::vec4(p, 1.f);
            tp.x = (((tp.x / tp.w) + 1.f) / 2.f);
            tp.y = ((-1.f * (tp.y / tp.w) + 1.f) / 2.f);
            if (tp.x < 0.f || tp.x > 1.f || tp.y < 0.f || tp.y > 1.f)
            {
                allPointsVisible = false;
                drawSimple(rw, g_res.getMesh("world.Sphere"), &g_res,white,
                            glm::translate(glm::mat4(1.f), p));
            }
            else
            {
                drawSimple(rw, g_res.getMesh("world.Sphere"), &g_res.white,
                            glm::translate(glm::mat4(1.f), p), glm::vec4(0, 1, 0, 1));
            }
        }

        debugDraw.boundingBox(bb, glm::mat4(1.f),
                allPointsVisible ? glm::vec4(0, 1, 0, 1) : glm::vec4(1));
#else
        debugDraw.boundingBox(bb, glm::mat4(1.f), glm::vec4(1));
#endif
    }

    debugDraw.draw(rw);
}

void ModelEditor::loadBlenderFile(std::string const& filename)
{
    // execute blender with a python script that will output the data in the datafile format
    const char* blenderPath =
#if _WIN32
    "\"C:/Program Files/Blender Foundation/Blender 2.81/blender.exe\"";
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
        showError("Failed to import blender file. See console for details.");
        return;
    }

    const char* outputFile = "blender_output.dat";
    DataFile::Value val = DataFile::load(outputFile);

    // meshes
    if (!val.dict().hasValue())
    {
        error("Failed to import blender file: Unexpected data structure.\n");
        showError("Failed to import blender file. See console for details.");
        return;
    }

    // scenes
    if (!val.dict().val()["scenes"].array().hasValue()
            || val.dict().val()["scenes"].array().val().empty())
    {
        error("Failed to import blender file: No scenes found in file.\n");
        showError("The chosen does not contain any scenes.");
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
    if (!scenePtr)
    {
        error("Scene \"", model->sourceSceneName, "\" not found\n");
        return;
    }

    auto& scene = *scenePtr;

    if (model->name.empty() || model->name.substr(0, strlen("Model")) == "Model")
    {
        model->name = scene["name"].string().val();
    }

    model->collections.clear();
    auto collections = scene["collections"].array(true).val();
    for (auto& collection : collections)
    {
        std::string collectionName = collection.string("");
        model->collections.push_back({
            collectionName,
            collectionName.find("Debris") != std::string::npos
        });
    }

    Map<std::string, u32> meshesToLoad;
    auto& objects = scene["objects"].array().val();
    for (auto& mesh : model->meshes)
    {
        mesh.destroy();
    }
    model->meshes.clear();
    Array<ModelObject> oldObjects = std::move(model->objects);
    model->objects.clear();
    auto& meshDict = dict["meshes"].dict().val();
    for (auto& object : objects)
    {
        auto& obj = object.dict().val();
        auto& meshName = obj["data_name"].string().val();
        auto meshIt = meshesToLoad.get(meshName);
        if (!meshIt)
        {
            meshesToLoad[meshName] = model->meshes.size();
            meshIt = meshesToLoad.get(meshName);
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

        modelObj->meshIndex = *meshIt;
        modelObj->name = std::move(obj["name"]).string().val();
        glm::mat4 matrix = obj["matrix"].convertBytes<glm::mat4>().val();
        modelObj->position = translationOf(matrix);
        modelObj->rotation = glm::quat_cast(glm::mat3(rotationOf(matrix)));
        modelObj->scale = scaleOf(matrix);
        modelObj->bounds = obj["bounds"].vec3().val();

        modelObj->collectionIndexes.clear();
        auto collections = obj["collection_indexes"].array(true).val();
        for (auto& collection : collections)
        {
            modelObj->collectionIndexes.push_back((u32)collection.integer().val());
        }

        // auto set certain propers based on the name of the object
        if (modelObj->name.find("Collision") != std::string::npos
            || modelObj->name.find("collision") != std::string::npos)
        {
            modelObj->isCollider = true;
            modelObj->isVisible = false;
        }
        else if (modelObj->name.find("Body") != std::string::npos)
        {
            //modelObj->isPaint = true;
            modelObj->materialGuid = g_res.getMaterial("paint_material")->guid;
        }

        model->objects.push_back(std::move(*modelObj));
    }
#if 0
    for (auto& obj : model->objects)
    {
        obj.createMousePickCollisionShape(model);
        body->attachShape(*obj.mousePickShape);
    }
#endif
}
