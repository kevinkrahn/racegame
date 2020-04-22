#include "model_editor.h"
#include "../imgui.h"
#include "../renderer.h"
#include "../game.h"
#include "../mesh_renderables.h"
#include <filesystem>

ModelEditor::ModelEditor()
{
}

void ModelEditor::onUpdate(Renderer* renderer, f32 deltaTime)
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
    }

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

    ImGui::Gap();

    if (ImGui::BeginChild("Objects", {0,0}, true))
    {
        for (u32 i=0; i<model->objects.size(); ++i)
        {
            auto it = std::find_if(selectedObjects.begin(), selectedObjects.end(),
                    [&](u32 index){ return index == i; });
            if (ImGui::Selectable(model->objects[i].name.c_str(), it != selectedObjects.end()))
            {
                if (it == selectedObjects.end())
                {
                    selectedObjects.push_back(i);
                }
                else
                {
                    selectedObjects.erase(it);
                }
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();

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

    RenderWorld* rw = renderer->getRenderWorld();
    rw->add(&debugDraw);

    for (u32 i=0; i<(u32)model->objects.size(); ++i)
    {
        auto& obj = model->objects[i];
        glm::mat4 transform = glm::translate(glm::mat4(1.f), obj.position)
            * glm::mat4_cast(obj.rotation)
            * glm::scale(glm::mat4(1.f), obj.scale);
        rw->push(LitRenderable(&model->meshes[obj.meshIndex], transform));
    }
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

        model->objects.push_back(std::move(*modelObj));
    }
}
