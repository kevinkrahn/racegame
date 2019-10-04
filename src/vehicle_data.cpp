#include "vehicle_data.h"
#include "game.h"
#include "resources.h"
#include "scene.h"

void loadVehicleData(DataFile::Value& data, VehicleData& vehicle)
{
    vehicle.physics.chassisDensity = data["chassis-density"].real();
    vehicle.physics.wheelMassFront = data["wheel-mass-front"].real();
    vehicle.physics.wheelMassRear = data["wheel-mass-rear"].real();
    vehicle.physics.frontToeAngle = glm::radians(-data["wheel-front-toe-angle-degrees"].real());
    vehicle.physics.rearToeAngle = glm::radians(data["wheel-rear-toe-angle-degrees"].real());
    vehicle.physics.wheelDampingRate = data["wheel-damping"].real();
    vehicle.physics.offroadDampingRate = data["wheel-offroad-damping"].real();
    vehicle.physics.trackTireFriction = data["track-tire-friction"].real();
    vehicle.physics.offroadTireFriction = data["offroad-tire-friction"].real();
    vehicle.physics.topSpeed = data["top-speed"].real();
    vehicle.physics.constantDownforce = data["constant-downforce"].real();
    vehicle.physics.forwardDownforce = data["forward-downforce"].real();
    vehicle.physics.driftBoost = data["drift-boost"].real();
    vehicle.physics.rearTireGripPercent = data["rear-tire-grip-percent"].real();
    vehicle.physics.maxEngineOmega = data["max-engine-omega"].real();
    vehicle.physics.peekEngineTorque = data["peek-engine-torque"].real();
    vehicle.physics.engineDampingFullThrottle = data["engine-damping-full-throttle"].real();
    vehicle.physics.engineDampingZeroThrottleClutchEngaged = data["engine-damping-zero-throttle-clutch-engaged"].real();
    vehicle.physics.engineDampingZeroThrottleClutchDisengaged = data["engine-damping-zero-throttle-clutch-disengaged"].real();
    vehicle.physics.maxBrakeTorque = data["max-brake-torque"].real();
    vehicle.physics.maxSteerAngle = glm::radians(data["max-steer-angle-degrees"].real());
    vehicle.physics.clutchStrength = data["clutch-strength"].real();
    vehicle.physics.gearRatios.clear();
    for (auto& d : data["gear-ratios"].array())
    {
        vehicle.physics.gearRatios.push_back(d.real());
    }
    vehicle.physics.finalGearRatio = data["final-gear-ratio"].real();
    vehicle.physics.suspensionMaxCompression = data["suspension-max-compression"].real();
    vehicle.physics.suspensionMaxDroop = data["suspension-max-droop"].real();
    vehicle.physics.autoBoxSwitchTime = data["auto-box-switch-time"].real();
    vehicle.physics.gearSwitchTime = data["gear-switch-time"].real();
    vehicle.physics.suspensionSpringStrength = data["suspension-spring-strength"].real();
    vehicle.physics.suspensionSpringDamperRate = data["suspension-spring-damper-rate"].real();
    vehicle.physics.camberAngleAtRest = data["camber-angle-at-rest"].real();
    vehicle.physics.camberAngleAtMaxDroop = data["camber-angle-at-max-droop"].real();
    vehicle.physics.camberAngleAtMaxCompression = data["camber-angle-at-max-compression"].real();
    vehicle.physics.frontAntiRollbarStiffness = data["front-anti-rollbar-stiffness"].real();
    vehicle.physics.rearAntiRollbarStiffness = data["rear-anti-rollbar-stiffness"].real();
    vehicle.physics.ackermannAccuracy = data["ackermann-accuracy"].real();

    bool limitedSlipDiff = data["limited-slip-diff"].boolean();
    std::string drivenWheels = data["driven-wheels"].string();
    if (drivenWheels == "front")
    {
        vehicle.physics.differential = limitedSlipDiff ?
            PxVehicleDifferential4WData::eDIFF_TYPE_LS_FRONTWD : PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_FRONTWD;
    }
    else if (drivenWheels == "rear")
    {
        vehicle.physics.differential = limitedSlipDiff ?
            PxVehicleDifferential4WData::eDIFF_TYPE_LS_REARWD : PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_REARWD;
    }
    else if (drivenWheels == "all")
    {
        vehicle.physics.differential = limitedSlipDiff ?
            PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD : PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_4WD;
    }
    else
    {
        FATAL_ERROR("Invalid 'driven-wheels' setting for vehicle. Must be one of 'front', 'rear', or 'all'.");
    }

    vehicle.name = data["name"].string();
    vehicle.description = data["description"].string();
    vehicle.price = data["price"].integer();
    auto& specs = data["specs"];
    vehicle.specs.acceleration = specs["acceleration"].real();
    vehicle.specs.speed = specs["speed"].real();
    vehicle.specs.armor = specs["armor"].real();
    vehicle.specs.weight = specs["weight"].real();
    vehicle.specs.handling = specs["handling"].real();

    DataFile::Value::Dict& scene = g_resources.getScene(data["scene"].string().c_str());
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
            vehicle.debrisChunks.push_back({
                mesh,
                transform,
                shape,
                name.find("Body") != std::string::npos
            });
        }
        else if (name.find("Chassis") != std::string::npos)
        {
            vehicle.chassisMeshes.push_back({
                g_resources.getMesh(e["data_name"].string().c_str()),
                transform,
                nullptr,
                name.find("Body") != std::string::npos
            });
        }
        else if (name.find("FL") != std::string::npos)
        {
            vehicle.wheelMeshFront = {
                g_resources.getMesh(e["data_name"].string().c_str()),
                glm::scale(glm::mat4(1.f), scaleOf(transform))
            };
            vehicle.physics.wheelRadiusFront = e["bound_z"].real() * 0.5f;
            vehicle.physics.wheelWidthFront = e["bound_y"].real();
            vehicle.physics.wheelPositions[WHEEL_FRONT_RIGHT] = transform[3];
        }
        else if (name.find("RL") != std::string::npos)
        {
            vehicle.wheelMeshRear = {
                g_resources.getMesh(e["data_name"].string().c_str()),
                glm::scale(glm::mat4(1.f), scaleOf(transform))
            };
            vehicle.physics.wheelRadiusRear = e["bound_z"].real() * 0.5f;
            vehicle.physics.wheelWidthRear = e["bound_y"].real();
            vehicle.physics.wheelPositions[WHEEL_REAR_RIGHT] = transform[3];
        }
        else if (name.find("FR") != std::string::npos)
        {
            vehicle.physics.wheelPositions[WHEEL_FRONT_LEFT] = transform[3];
        }
        else if (name.find("RR") != std::string::npos)
        {
            vehicle.physics.wheelPositions[WHEEL_REAR_LEFT] = transform[3];
        }
        else if (name.find("Collision") != std::string::npos)
        {
            vehicle.physics.collisionMeshes.push_back({
                g_resources.getMesh(e["data_name"].string().c_str())->getConvexCollisionMesh(),
                transform
            });
            vehicle.collisionWidth =
                glm::max(vehicle.collisionWidth, (f32)e["bound_y"].real());
        }
        else if (name.find("COM") != std::string::npos)
        {
            vehicle.physics.centerOfMass = transform[3];
        }
    }

    if (data.hasKey("center-of-mass-offset"))
    {
        vehicle.physics.centerOfMass = data["center-of-mass-offset"].vec3();
    }
}
