#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "driver.h"

void Resources::load()
{
    textures.reset(new Textures);
    sounds.reset(new Sounds);

    /*
    auto files = loadFiles("./assets", ".dat");
    for (auto& file : files)
    {
    }
    */

    for (const char* filename : dataFiles)
    {
        print("Loading data file: ", filename, '\n');
        DataFile::Value val = DataFile::load(filename);

        // meshes
        if (val.hasKey("meshes"))
        {
            for (auto& val : val["meshes"].array())
            {
                auto& meshInfo = val.dict();
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

                    //print("Mesh: ", meshInfo["name"].string(), '\n');

                    SmallVec<VertexAttribute> vertexFormat = {
                        VertexAttribute::FLOAT3, // position
                        VertexAttribute::FLOAT3, // normal
                        VertexAttribute::FLOAT3, // color
                        VertexAttribute::FLOAT2, // uv
                    };
                    Mesh mesh = {
                        meshInfo["name"].string(),
                        std::move(vertices),
                        std::move(indices),
                        numVertices,
                        numIndices,
                        numColors,
                        numTexCoords,
                        elementSize,
                        stride,
                        BoundingBox{
                            meshInfo["aabb_min"].convertBytes<glm::vec3>(),
                            meshInfo["aabb_max"].convertBytes<glm::vec3>(),
                        },
                        vertexFormat
                    };
                    mesh.createVAO();
                    meshes[meshInfo["name"].string()] = std::move(mesh);
                }
                else
                {
                    auto const& vertexBuffer = meshInfo["vertex_buffer"].bytearray();
                    auto const& indexBuffer = meshInfo["index_buffer"].bytearray();
                    std::vector<f32> vertices((f32*)vertexBuffer.data(), (f32*)(vertexBuffer.data() + vertexBuffer.size()));
                    std::vector<u32> indices((u32*)indexBuffer.data(), (u32*)(indexBuffer.data() + indexBuffer.size()));
                    u32 numVertices = (u32)meshInfo["num_vertices"].integer();
                    u32 numIndices = (u32)meshInfo["num_indices"].integer();

                    u32 stride = elementSize * sizeof(f32);
                    Mesh mesh = {
                        meshInfo["name"].string(),
                        std::move(vertices),
                        std::move(indices),
                        numVertices,
                        numIndices,
                        0,
                        0,
                        elementSize,
                        stride,
                        BoundingBox{
                            meshInfo["aabb_min"].convertBytes<glm::vec3>(),
                            meshInfo["aabb_max"].convertBytes<glm::vec3>(),
                        }
                    };
                    meshes[meshInfo["name"].string()] = std::move(mesh);
                }
            }
        }

        // scenes
        if (val.hasKey("scenes"))
        {
            for (auto& val : val["scenes"].array())
            {
                scenes[val["name"].string()] = std::move(val.dict());
            }
        }
    }
}

