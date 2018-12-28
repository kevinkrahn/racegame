#include "resources.h"
#include "game.h"
#include <filesystem>

namespace fs = std::filesystem;

void Resources::load()
{
    for (auto& p : fs::recursive_directory_iterator("."))
    {
        if (fs::is_regular_file(p))
        {
            if (p.path().extension().string() == ".dat")
            {
                print("Loading data file: ", p.path().c_str(), '\n');
                DataFile::Value val = DataFile::load(p.path().c_str());

                // meshes
                //print(val.getDict().at("meshes"), '\n');
                for (auto& val : val["meshes"].array())
                {
                    auto& meshInfo = val.dict();
                    print("Loading mesh: ", meshInfo.at("name"), '\n');
                    u32 elementSize = (u32)meshInfo["element_size"].integer();
                    if (elementSize == 3)
                    {
                        auto const& vertexBuffer = meshInfo["vertex_buffer"].bytearray();
                        auto const& indexBuffer = meshInfo["index_buffer"].bytearray();
                        std::vector<f32> vertices((f32*)vertexBuffer.data(), (f32*)(vertexBuffer.data() + vertexBuffer.size()));
                        std::vector<u32> indices((u32*)indexBuffer.data(), (u32*)(indexBuffer.data() + indexBuffer.size()));
                        u32 numVertices = (u32)meshInfo["num_vertices"].integer();
                        u32 numIndices = (u32)meshInfo["num_indices"].integer();
                        u32 numColors = (u32)meshInfo["num_colors"].integer();
                        u32 numTexCoords = (u32)meshInfo["num_texcoords"].integer();
                        u32 stride = (6 + numColors * 3 + numTexCoords * 2) * sizeof(f32);

                        Mesh meshData = {
                            std::move(vertices),
                            std::move(indices),
                            numVertices,
                            numIndices,
                            numColors,
                            numTexCoords,
                            elementSize,
                            stride,
                        };
                        meshData.renderHandle = game.renderer.loadMesh(meshData);
                        meshes[meshInfo["name"].string()] = meshData;
                    }
                    else
                    {
                        Mesh meshData = { {}, {}, 0, 0, elementSize, 3 * sizeof(f32), 0 };
                        meshes[meshInfo["name"].string()] = meshData;
                    }
                }

                // scenes
                for (auto& val : val["scenes"].array())
                {
                    scenes[val["name"].string()] = std::move(val.dict());
                }
            }
        }
    }
}
