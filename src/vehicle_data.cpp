#include "vehicle_data.h"
#include "game.h"

void loadVehicleScene(const char* sceneName, VehicleData* vehicleData)
{
    DataFile::Value::Dict& scene = game.resources.getScene(sceneName);
    for (auto& e : scene["entities"].array())
    {
        std::string name = e["name"].string();
        glm::mat4 transform = e["matrix"].convertBytes<glm::mat4>();
        if (name.find("Chassis") != std::string::npos)
        {
            vehicleData->chassisMeshes.push_back({
                game.resources.getMesh(e["data_name"].string().c_str()).renderHandle,
                transform
            });
        }
        else if (name.find("FL") != std::string::npos)
        {
            vehicleData->wheelMeshFront = {
                game.resources.getMesh(e["data_name"].string().c_str()).renderHandle,
                glm::scale(glm::mat4(1.f), scaleOf(transform))
            };
            vehicleData->physics.wheelRadiusFront = e["bound_z"].real() * 0.5f;
            vehicleData->physics.wheelWidthFront = e["bound_y"].real();
            vehicleData->physics.wheelPositions[WHEEL_FRONT_RIGHT] = transform[3];
        }
        else if (name.find("RL") != std::string::npos)
        {
            vehicleData->wheelMeshRear = {
                game.resources.getMesh(e["data_name"].string().c_str()).renderHandle,
                glm::scale(glm::mat4(1.f), scaleOf(transform))
            };
            vehicleData->physics.wheelRadiusRear = e["bound_z"].real() * 0.5f;
            vehicleData->physics.wheelWidthRear = e["bound_y"].real();
            vehicleData->physics.wheelPositions[WHEEL_REAR_RIGHT] = transform[3];
        }
        else if (name.find("FR") != std::string::npos)
        {
            vehicleData->physics.wheelPositions[WHEEL_FRONT_LEFT] = transform[3];
        }
        else if (name.find("RR") != std::string::npos)
        {
            vehicleData->physics.wheelPositions[WHEEL_REAR_LEFT] = transform[3];
        }
        else if (name.find("debris") != std::string::npos)
        {
            std::string const& meshName = e["data_name"].string();
            PxConvexMesh* collisionMesh = game.resources.getConvexCollisionMesh(meshName);
            PxMaterial* material = game.physx.physics->createMaterial(0.3f, 0.3f, 0.1f);
            PxShape* shape = game.physx.physics->createShape(
                    PxConvexMeshGeometry(collisionMesh, PxMeshScale(convert(scaleOf(transform)))), *material);
            shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_GROUND,
                        COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS, 0, 0));
            material->release();
            vehicleData->debrisChunks.push_back({
                game.resources.getMesh(meshName.c_str()).renderHandle,
                transform,
                shape
            });
        }
        else if (name.find("Collision") != std::string::npos)
        {
            vehicleData->physics.collisionMeshes.push_back({
                game.resources.getConvexCollisionMesh(e["data_name"].string()),
                transform
            });
            vehicleData->collisionWidth =
                glm::max(vehicleData->collisionWidth, (f32)e["bound_y"].real());
        }
        else if (name.find("COM") != std::string::npos)
        {
            vehicleData->physics.centerOfMass = transform[3];
        }
    }
}
