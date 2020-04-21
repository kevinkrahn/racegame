#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "driver.h"
#include "util.h"
#include <filesystem>

void Resources::loadResource(DataFile::Value& data)
{
    if (data.dict().hasValue())
    {
        auto& dict = data.dict().val();
        auto resourceType = (u32)dict["type"].integer().val();
        auto guid = dict["guid"].integer().val();
        switch ((ResourceType)resourceType)
        {
            case ResourceType::TEXTURE:
            {
                auto tex = std::make_unique<Texture>();
                ::loadResource(data, *tex);
                textureNameMap[tex->name] = tex.get();
                textures[guid] = std::move(tex);
            } break;
            case ResourceType::MESH_SCENE:
            {
            } break;
            case ResourceType::SOUND:
            {
            } break;
            case ResourceType::FONT:
            {
            } break;
            case ResourceType::TRACK:
            {
            } break;
        }
    }
}

void Resources::load()
{
    sounds.reset(new Sounds);

    constexpr u8 whiteBytes[] = { 255, 255, 255, 255 };
    constexpr u8 identityNormalBytes[] = { 128, 128, 255, 255 };
    white = Texture("white", 1, 1, (u8*)whiteBytes, TextureType::COLOR);
    identityNormal = Texture("identityNormal", 1, 1, (u8*)identityNormalBytes, TextureType::NORMAL_MAP);

    std::vector<FileItem> resourceFiles = readDirectory(DATA_DIRECTORY);
    for (auto& file : resourceFiles)
    {
        if (std::filesystem::path(file.path).extension() == ".dat")
        {
            print("Loading data file: ", file.path, '\n');
            auto data = DataFile::load(str(DATA_DIRECTORY, "/", file.path));
            if (data.array().hasValue())
            {
                for (auto& el : data.array().val())
                {
                    loadResource(el);
                }
            }
            else
            {
                loadResource(data);
            }
        }
    }

    for (const char* filename : dataFiles)
    {
        DataFile::Value val = DataFile::load(filename);

        // meshes
        if (val.dict().hasValue())
        {
            for (auto& val : val.dict().val()["meshes"].array().val())
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
                    mesh.createVAO();
                    meshes[mesh.name] = std::move(mesh);
                }
                // TODO: This path is never used, should it be removed?
                else
                {
                    auto const& vertexBuffer = meshInfo["vertex_buffer"].bytearray().val();
                    auto const& indexBuffer = meshInfo["index_buffer"].bytearray().val();
                    std::vector<f32> vertices((f32*)vertexBuffer.data(), (f32*)(vertexBuffer.data() + vertexBuffer.size()));
                    std::vector<u32> indices((u32*)indexBuffer.data(), (u32*)(indexBuffer.data() + indexBuffer.size()));
                    u32 numVertices = (u32)meshInfo["num_vertices"].integer().val();
                    u32 numIndices = (u32)meshInfo["num_indices"].integer().val();

                    u32 stride = elementSize * sizeof(f32);
                    Mesh mesh = {
                        meshInfo["name"].string().val(),
                        std::move(vertices),
                        std::move(indices),
                        numVertices,
                        numIndices,
                        0,
                        0,
                        elementSize,
                        stride,
                        BoundingBox{
                            meshInfo["aabb_min"].convertBytes<glm::vec3>().val(),
                            meshInfo["aabb_max"].convertBytes<glm::vec3>().val(),
                        }
                    };
                    meshes[mesh.name] = std::move(mesh);
                }
            }
        }

        // scenes
        if (val.dict().hasValue())
        {
            // this really sucks
            for (auto& val : val.dict().val()["scenes"].array().val())
            {
                scenes[val.dict().val()["name"].string().val()] = std::move(val.dict().val());
            }
        }
    }
}

