#include "resources.h"
#include "datafile.h"
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
                for (auto& val : val.getDict().at("meshes").getArray())
                {
                    auto const& meshInfo = val.getDict();
                    u32 elementSize = (u32)meshInfo.at("element_size").getInt();
                    if (elementSize == 3)
                    {
                        auto const& vertexBuffer = meshInfo.at("vertex_buffer").getByteArray();
                        auto const& indexBuffer = meshInfo.at("index_buffer").getByteArray();
                        std::vector<f32> vertices((f32*)vertexBuffer.data(), (f32*)vertexBuffer.data() + vertexBuffer.size());
                        std::vector<u32> indices((u32*)indexBuffer.data(), (u32*)indexBuffer.data() + indexBuffer.size());
                        u32 numColors = (u32)meshInfo.at("num_colors").getInt();
                        u32 numTexCoords = (u32)meshInfo.at("num_texcoords").getInt();
                        u32 stride = 6 + numColors * 3 + numTexCoords * 2 * sizeof(f32);
                        Mesh meshData = {
                            std::move(vertices),
                            std::move(indices),
                            numColors,
                            numTexCoords,
                            stride,
                            elementSize,
                        };
                        meshData.renderHandle = game.renderer.loadMesh(meshData);
                        meshes[meshInfo.at("name").getString()] = meshData;
                    }
                    else
                    {
                        Mesh meshData = { {}, {}, 0, 0, elementSize, 3 * sizeof(f32), 0 };
                        meshes[meshInfo.at("name").getString()] = meshData;
                    }
                }
            }
        }
    }
}
