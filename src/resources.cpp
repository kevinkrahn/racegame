#include "resources.h"
#include "game.h"
#include <filesystem>
#include <stb_image.h>

void Resources::load()
{
    for (auto& p : fs::recursive_directory_iterator("."))
    {
        if (fs::is_regular_file(p))
        {
            std::string ext = p.path().extension().string();
            if (ext == ".dat")
            {
                print("Loading data file: ", p.path().string(), '\n');
                DataFile::Value val = DataFile::load(p.path().string().c_str());

                // meshes
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

                // scenes
                for (auto& val : val["scenes"].array())
                {
                    scenes[val["name"].string()] = std::move(val.dict());
                }
            }
            else if (ext == ".bmp" || ext == ".png")
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
        }
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
