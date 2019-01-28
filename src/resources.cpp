#include "resources.h"
#include "game.h"
#include "driver.h"
#include <filesystem>
#include <stb_image.h>

void Resources::load()
{
    std::vector<DataFile::Value> vehicleDataValues;
    std::vector<std::pair<DataFile::Value, Material*>> pendingMaterials;

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

                            Mesh meshData = {
                                std::move(vertices),
                                std::move(indices),
                                numVertices,
                                numIndices,
                                numColors,
                                numTexCoords,
                                elementSize,
                                stride,
                                0,
                                meshInfo["aabb_min"].convertBytes<glm::vec3>(),
                                meshInfo["aabb_max"].convertBytes<glm::vec3>(),
                            };
                            meshData.renderHandle = game.renderer.loadMesh(meshData);
                            meshes[meshInfo["name"].string()] = meshData;
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
                            Mesh meshData = {
                                std::move(vertices),
                                std::move(indices),
                                numVertices,
                                numIndices,
                                0,
                                0,
                                elementSize,
                                stride,
                                u32(-1),
                                meshInfo["aabb_min"].convertBytes<glm::vec3>(),
                                meshInfo["aabb_max"].convertBytes<glm::vec3>(),
                            };
                            meshes[meshInfo["name"].string()] = meshData;
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

                // materials
                if (val.hasKey("materials"))
                {
                    for (auto& val : val["materials"].dict())
                    {
                        auto mat = std::make_unique<Material>();
                        pendingMaterials.emplace_back(std::make_pair(std::move(val.second), mat.get()));
                        materials[val.first] = std::move(mat);
                    }
                }

                // vehicle colors
                if (val.hasKey("vehicle-colors"))
                {
                    for (auto& val : val["vehicle-colors"].array())
                    {
                        auto col = std::make_unique<VehicleColor>();
                        col->color = val["color"].vec3();
                        col->material.reset(new Material);
                        col->material->lighting.color = col->color;
                        pendingMaterials.emplace_back(std::make_pair(std::move(val["material"]), col->material.get()));
                        vehicleColors.emplace_back(std::move(col));
                    }
                }
            }
            else if (ext == ".bmp" || ext == ".png" || ext == ".jpg")
            {
                Texture tex;
                i32 width, height, channels;
                u8* data = (u8*)stbi_load(p.path().string().c_str(), &width, &height, &channels, 4);
                if (!data)
                {
                    error("Failed to load image: ", p.path().string(), " (", stbi_failure_reason(), ")\n");
                    continue;
                }
                size_t size = width * height * 4;
#if 0
                // premultied alpha
                for (u32 i=0; i<size; i+=4)
                {
                    //f32 a = f32(data[i+3]) / 255.f;
                    f32 a = powf(f32(data[i+3]) / 255.f, 1.f / 2.2f);
                    data[i]   = (u8)(data[i] / 255.f * a * 255.f);
                    data[i+1] = (u8)(data[i+1] / 255.f * a * 255.f);
                    data[i+2] = (u8)(data[i+2] / 255.f * a * 255.f);
                }
#endif
                tex.width = width;
                tex.height = height;
                tex.format = Texture::Format::RGBA8;
                tex.renderHandle = game.renderer.loadTexture(tex, data, size);
                textures[p.path().stem().string()] = tex;

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

    // finish loading materials now that textures have been loaded
    for (auto& m : pendingMaterials)
    {
        DataFile::Value& mat = m.first;
        Material* material = m.second;
        material->shader = game.renderer.getShader(mat["shader"].string().c_str());
        material->culling = mat["culling"].boolean(material->culling);
        material->castShadow = mat["cast-shadow"].boolean(material->castShadow);
        if (mat.hasKey("textures"))
        {
            for (auto& t : mat["textures"].array())
            {
                material->textures.push_back(getTexture(t.string().c_str()).renderHandle);
            }
        }
        material->depthWrite = mat["depth-write"].boolean(material->depthWrite);
        material->depthRead = mat["depth-read"].boolean(material->depthRead);
        material->depthOffset = mat["depth-offset"].real(material->depthOffset);
        if (mat.hasKey("lighting"))
        {
            auto& lighting = mat["lighting"];
            material->lighting.color = lighting["color"].vec3(material->lighting.color);
            material->lighting.emit = lighting["emit"].vec3(material->lighting.emit);
            material->lighting.specularPower = lighting["specular-power"].real(material->lighting.specularPower);
            material->lighting.specularStrength = lighting["specular-strength"].real(material->lighting.specularStrength);
            material->lighting.specularColor = lighting["specular-color"].vec3(material->lighting.specularColor);
            material->lighting.fresnelScale = lighting["fresnel-scale"].real(material->lighting.fresnelScale);
            material->lighting.fresnelPower = lighting["fresnel-power"].real(material->lighting.fresnelPower);
            material->lighting.fresnelBias = lighting["fresnel-bias"].real(material->lighting.fresnelBias);
        }
    }

    // finish loading vehicle data now that all the scenes have been loaded
    for (auto& val : vehicleDataValues)
    {
        vehicleData.push_back({});
        loadVehicleData(val, vehicleData.back());
    }
}

PxTriangleMesh* Resources::getCollisionMesh(std::string const& name)
{
    auto trimesh = collisionMeshCache.find(name);
    if (trimesh != collisionMeshCache.end())
    {
        return trimesh->second;
    }
    Mesh const& mesh = getMesh(name.c_str());

    PxTriangleMeshDesc desc;
    desc.points.count = mesh.numVertices;
    desc.points.stride = mesh.stride;
    desc.points.data = mesh.vertices.data();
    desc.triangles.count = mesh.numIndices / 3;
    desc.triangles.stride = 3 * sizeof(mesh.indices[0]);
    desc.triangles.data = mesh.indices.data();

    PxDefaultMemoryOutputStream writeBuffer;
    PxTriangleMeshCookingResult::Enum result;
    if (!game.physx.cooking->cookTriangleMesh(desc, writeBuffer, &result))
    {
        FATAL_ERROR("Failed to create collision mesh: ", name);
    }

    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    PxTriangleMesh* t = game.physx.physics->createTriangleMesh(readBuffer);
    collisionMeshCache[name] = t;

    return t;
}

PxConvexMesh* Resources::getConvexCollisionMesh(std::string const& name)
{
    auto convexMesh = convexCollisionMeshCache.find(name);
    if (convexMesh != convexCollisionMeshCache.end())
    {
        return convexMesh->second;
    }
    Mesh const& mesh = getMesh(name.c_str());

    PxConvexMeshDesc convexDesc;
    convexDesc.points.count  = mesh.numVertices;
    convexDesc.points.stride = mesh.stride;
    convexDesc.points.data   = mesh.vertices.data();
    convexDesc.flags         = PxConvexFlag::eCOMPUTE_CONVEX;

    PxDefaultMemoryOutputStream writeBuffer;
    if (!game.physx.cooking->cookConvexMesh(convexDesc, writeBuffer))
    {
        FATAL_ERROR("Failed to create convex collision mesh: ", name);
    }

    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    PxConvexMesh* t = game.physx.physics->createConvexMesh(readBuffer);
    convexCollisionMeshCache[name] = t;

    return t;
}
