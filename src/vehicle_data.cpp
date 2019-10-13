#include "vehicle_data.h"
#include "game.h"
#include "resources.h"
#include "scene.h"
#include "mesh_renderables.h"

void VehicleData::loadSceneData(const char* sceneName, VehicleTuning& tuning)
{
    // TODO: should these be stored here?
    chassisMeshes.clear();
    debrisChunks.clear();

    DataFile::Value::Dict& scene = g_resources.getScene(sceneName);
    for (auto& e : scene["entities"].array())
    {
        std::string name = e["name"].string();
        glm::mat4 transform = e["matrix"].convertBytes<glm::mat4>();
        if (name.find("debris") != std::string::npos)
        {
            std::string const& meshName = e["data_name"].string();
            Mesh* mesh = g_resources.getMesh(meshName.c_str());
            PxConvexMesh* collisionMesh = mesh->getConvexCollisionMesh();
            PxMaterial* material = g_game.physx.physics->createMaterial(0.3f, 0.3f, 0.1f);
            PxShape* shape = g_game.physx.physics->createShape(
                    PxConvexMeshGeometry(collisionMesh, PxMeshScale(convert(scaleOf(transform)))), *material);
            shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_DEBRIS,
                        COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS, 0, 0));
            material->release();
            debrisChunks.push_back({
                mesh,
                transform,
                shape,
                name.find("Body") != std::string::npos
            });
        }
        else if (name.find("Chassis") != std::string::npos)
        {
            chassisMeshes.push_back({
                g_resources.getMesh(e["data_name"].string().c_str()),
                transform,
                nullptr,
                name.find("Body") != std::string::npos
            });
        }
        else if (name.find("FL") != std::string::npos)
        {
            wheelMeshFront = {
                g_resources.getMesh(e["data_name"].string().c_str()),
                glm::scale(glm::mat4(1.f), scaleOf(transform))
            };
            tuning.physics.wheelRadiusFront = e["bound_z"].real() * 0.5f;
            tuning.physics.wheelWidthFront = e["bound_y"].real();
            tuning.physics.wheelPositions[WHEEL_FRONT_RIGHT] = transform[3];
        }
        else if (name.find("RL") != std::string::npos)
        {
            wheelMeshRear = {
                g_resources.getMesh(e["data_name"].string().c_str()),
                glm::scale(glm::mat4(1.f), scaleOf(transform))
            };
            tuning.physics.wheelRadiusRear = e["bound_z"].real() * 0.5f;
            tuning.physics.wheelWidthRear = e["bound_y"].real();
            tuning.physics.wheelPositions[WHEEL_REAR_RIGHT] = transform[3];
        }
        else if (name.find("FR") != std::string::npos)
        {
            tuning.physics.wheelPositions[WHEEL_FRONT_LEFT] = transform[3];
        }
        else if (name.find("RR") != std::string::npos)
        {
            tuning.physics.wheelPositions[WHEEL_REAR_LEFT] = transform[3];
        }
        else if (name.find("Collision") != std::string::npos)
        {
            tuning.physics.collisionMeshes.push_back({
                g_resources.getMesh(e["data_name"].string().c_str())->getConvexCollisionMesh(),
                transform
            });
            tuning.collisionWidth =
                glm::max(tuning.collisionWidth, (f32)e["bound_y"].real());
        }
        else if (name.find("COM") != std::string::npos)
        {
            tuning.physics.centerOfMass = transform[3];
        }
    }
}

void VehicleData::render(Renderer* renderer, glm::mat4 const& transform,
        glm::mat4* wheelTransforms, Driver* driver)
{
    for (auto& m : chassisMeshes)
    {
        LitSettings s;
        s.mesh = m.mesh;
        s.worldTransform = transform * m.transform;
        s.texture = nullptr;
        s.color = m.isBody ? driver->vehicleColor : glm::vec3(1.f);
        s.specularStrength = m.isBody ? 0.15f : 0.3f;
        s.specularPower = m.isBody ? 20.f : 200.f;
        renderer->push(LitRenderable(s));
    }

    glm::mat4 defaultWheelTransforms[NUM_WHEELS];
    if (!wheelTransforms)
    {
        for (u32 i=0; i<NUM_WHEELS; ++i)
        {
            defaultWheelTransforms[i] = glm::translate(glm::mat4(1.f),
                    driver->vehicleTuning.physics.wheelPositions[i]);
        }
    }

    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        glm::mat4 wheelTransform = transform *
            (wheelTransforms ? wheelTransforms[i] : defaultWheelTransforms[i]);
        if ((i & 1) == 0)
        {
            wheelTransform = glm::rotate(wheelTransform, PI, glm::vec3(0, 0, 1));
        }
        auto& mesh = i < 2 ? wheelMeshFront : wheelMeshRear;
        renderer->push(LitRenderable(mesh.mesh, wheelTransform * mesh.transform, nullptr));
    }
}

void VehicleData::renderDebris(Renderer* renderer,
        std::vector<VehicleDebris> const& debris, Driver* driver)
{
    for (auto const& d : debris)
    {
        LitSettings s;
        s.mesh = d.meshInfo->mesh;
        s.worldTransform = convert(d.rigidBody->getGlobalPose());
        s.texture = nullptr;
        s.color = d.meshInfo->isBody ? driver->vehicleColor : glm::vec3(1.f);
        s.specularStrength = d.meshInfo->isBody ? 0.15f : 0.3f;
        s.specularPower = d.meshInfo->isBody ? 20.f : 200.f;
        renderer->push(LitRenderable(s));
    }
}

#include "weapons/blaster.h"
#include "weapons/explosive_mine.h"

#include "vehicles/mini.h"
#include "vehicles/sportscar.h"

void initializeVehicleData()
{
    g_weapons.push_back(std::make_unique<WBlaster>());
    g_weapons.push_back(std::make_unique<WExplosiveMine>());

    g_vehicles.push_back(std::make_unique<VMini>());
    g_vehicles.push_back(std::make_unique<VSportscar>());
}
