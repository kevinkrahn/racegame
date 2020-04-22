#include "model_editor.h"
#include "../imgui.h"
#include "../renderer.h"

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
            model->sourceFilePath = path;
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
    ImGui::InputText("Name", &model->name);

    ImGui::End();
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
    if (model->sourceSceneName.empty() || blenderData.dict().val()["scenes"].array().val().size() > 1)
    {
        ImGui::OpenPopup("Blender Import");
    }
    else
    {
        processBlenderData();
    }
}

void ModelEditor::processBlenderData()
{
    for (auto& val : blenderData.dict().val()["meshes"].array().val())
    {
        auto& meshInfo = val.dict().val();
        u32 elementSize = (u32)meshInfo["element_size"].integer().val();
        if (elementSize == 3)
        {
            auto const& vertexBuffer = meshInfo["vertex_buffer"].bytearray().val();
            auto const& indexBuffer = meshInfo["index_buffer"].bytearray().val();
            std::vector<f32> vertices((f32*)vertexBuffer.data(), (f32*)(vertexBuffer.data() + vertexBuffer.size()));
            std::vector<u32> indices((u32*)indexBuffer.data(), (u32*)(indexBuffer.data() + indexBuffer.size()));
            u32 numVertices = (u32)meshInfo["num_vertices"].integer().val();
            u32 numIndices = (u32)meshInfo["num_indices"].integer().val();
            u32 numColors = (u32)meshInfo["num_colors"].integer().val();
            u32 numTexCoords = (u32)meshInfo["num_texcoords"].integer().val();
            u32 stride = (6 + numColors * 3 + numTexCoords * 2) * sizeof(f32);

            //print("Mesh: ", meshInfo["name"].string(), '\n');

            SmallVec<VertexAttribute> vertexFormat = {
                VertexAttribute::FLOAT3, // position
                VertexAttribute::FLOAT3, // normal
                VertexAttribute::FLOAT3, // color
                VertexAttribute::FLOAT2, // uv
            };
            Mesh mesh = {
                meshInfo["name"].string().val(),
                std::move(vertices),
                std::move(indices),
                numVertices,
                numIndices,
                numColors,
                numTexCoords,
                elementSize,
                stride,
                BoundingBox{
                    meshInfo["aabb_min"].convertBytes<glm::vec3>().val(),
                    meshInfo["aabb_max"].convertBytes<glm::vec3>().val(),
                },
                vertexFormat
            };
            //mesh.createVAO();
            //meshes[mesh.name] = std::move(mesh);
        }
    }
}
