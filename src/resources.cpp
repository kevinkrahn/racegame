#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "driver.h"
#include <stb_image.h>

void Resources::load()
{
    // load default texture
    u8 white[] = { 255, 255, 255, 255 };
    textures["white"] = Texture(1, 1, Texture::Format::RGBA8, white, sizeof(white));

    std::vector<DataFile::Value> vehicleDataValues;

    for (auto& p : fs::recursive_directory_iterator("."))
    {
        if (fs::is_regular_file(p))
        {
            std::string ext = p.path().extension().string();
            if (ext == ".dat" || ext == ".txt")
            {
                print("Loading data file: ", p.path().string(), '\n');
                DataFile::Value val = DataFile::load(p.path().string().c_str());

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

                // vehicles
                if (val.hasKey("vehicles"))
                {
                    for (auto& val : val["vehicles"].array())
                    {
                        vehicleDataValues.push_back(std::move(val));
                    }
                }
            }
            else if (ext == ".bmp" || ext == ".png" || ext == ".jpg")
            {
                i32 width, height, channels;
                u8* data = (u8*)stbi_load(p.path().string().c_str(), &width, &height, &channels, 4);
                if (!data)
                {
                    error("Failed to load image: ", p.path().string(), " (", stbi_failure_reason(), ")\n");
                    continue;
                }
                size_t size = width * height * 4;
                textures[p.path().stem().string()] = Texture(width, height, Texture::Format::SRGBA8, data, size);

                stbi_image_free(data);
            }
            else if (ext == ".wav")
            {
                SDL_AudioSpec spec;
                u32 size;
                u8* wavBuffer;
                if (SDL_LoadWAV(p.path().string().c_str(), &spec, &wavBuffer, &size) == nullptr)
                {
                    error("Failed to load wav file: ", p.path().string(), "(", SDL_GetError(), ")\n");
                    continue;
                }

                if (spec.freq != 44100)
                {
                    error("Failed to load wav file: ", p.path().string(), "(Unsupported frequency)\n");
                    continue;
                }

                if (spec.format != AUDIO_S16)
                {
                    error("Failed to load wav file: ", p.path().string(), "(Unsupported sample format)\n");
                    continue;
                }

                auto sound = std::make_unique<Sound>();
                sound->numChannels = spec.channels;
                sound->numSamples = (size / spec.channels) / sizeof(i16);
                sound->audioData.reset(new i16[size / sizeof(i16)]);
                memcpy(sound->audioData.get(), wavBuffer, size);
                sounds[p.path().stem().string()] = std::move(sound);

                SDL_FreeWAV(wavBuffer);
            }
            else if (ext == ".ogg")
            {
                auto sound = std::make_unique<Sound>();
                // TODO
                sounds[p.path().stem().string()] = std::move(sound);
            }
        }
    }

    // finish loading vehicle data now that all the scenes have been loaded
    for (auto& val : vehicleDataValues)
    {
        vehicleData.push_back({});
        loadVehicleData(val, vehicleData.back());
    }
}

