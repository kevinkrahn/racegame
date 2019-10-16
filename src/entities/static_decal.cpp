#include "static_decal.h"
#include "../renderer.h"
#include "../scene.h"
#include "../game.h"
#include "../gui.h"

const char* textures[] = {
    "arrow",
    "arrow_left",
    "arrow_right",
    "crack",
    "patch1",
    "grunge1",
    "grunge2",
    "grunge3",
    "sand",
};

const char* textureNames[] = {
    "Arrow Forward",
    "Arrow Left",
    "Arrow Right",
    "Crack",
    "Patch 1",
    "Grunge 1",
    "Grunge 2",
    "Grunge 3",
    "Dust",
};

StaticDecal::StaticDecal(i32 texIndex, glm::vec3 const& pos, u32 decalFilter)
{
    position = pos;
    scale = glm::vec3(16.f);
    rotation = glm::rotate(rotation, (f32)M_PI * 0.5f, glm::vec3(0, 1, 0));
    this->texIndex = texIndex;
    this->decalFilter = decalFilter;
}

void StaticDecal::onCreateEnd(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.placeableEntity = this;
    actor->userData = &physicsUserData;

    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxBoxGeometry(convert(scale * 0.5f)), *scene->genericMaterial);
    collisionShape->setQueryFilterData(PxFilterData(COLLISION_FLAG_SELECTABLE, 0, 0, 0));
    collisionShape->setSimulationFilterData(PxFilterData(0, 0, 0, 0));
    collisionShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);

    scene->getPhysicsScene()->addActor(*actor);

    updateTransform(scene);
}

void StaticDecal::updateTransform(Scene* scene)
{
    if (actor)
    {
        PlaceableEntity::updateTransform(scene);

        PxShape* shape = nullptr;
        actor->getShapes(&shape, 1);
        shape->setGeometry(PxBoxGeometry(convert(
                        glm::abs(glm::max(glm::vec3(0.01f), scale) * 0.5f))));

        if (texIndex == 8)
        {
            shape->setQueryFilterData(PxFilterData(
                        COLLISION_FLAG_SELECTABLE | COLLISION_FLAG_DUST, 0, 0, 0));
        }
        else
        {
            shape->setQueryFilterData(PxFilterData(COLLISION_FLAG_SELECTABLE, 0, 0, 0));
        }
    }

    tex = g_resources.getTexture(textures[texIndex]);
    decal.setTexture(tex);
    decal.setPriority(beforeMarking ? 14 : 8000);

    const u32 bufferSize = 32;
    PxOverlapHit hitBuffer[bufferSize];
    PxOverlapBuffer hit(hitBuffer, bufferSize);
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    //filter.flags |= PxQueryFlag::eDYNAMIC;
    filter.data = PxFilterData(0, decalFilter, 0, 0);
    decal.begin(transform);
    if (scene->getPhysicsScene()->overlap(PxBoxGeometry(convert(glm::abs(scale * 0.5f))),
                PxTransform(convert(position), convert(rotation)), hit, filter))
    {
        for (u32 i=0; i<hit.getNbTouches(); ++i)
        {
            PxActor* actor = hit.getTouch(i).actor;
            ActorUserData* userData = (ActorUserData*)actor->userData;
            if (userData && (userData->entityType == ActorUserData::ENTITY
                        || userData->entityType == ActorUserData::SELECTABLE_ENTITY))
            {
                Entity* entity = (Entity*)userData->entity;
                entity->applyDecal(decal);
            }
        }
    }
    decal.end();
}

void StaticDecal::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    rw->add(&decal);
}

void StaticDecal::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected)
{
    BoundingBox decalBoundingBox{ glm::vec3(-0.5f), glm::vec3(0.5f) };
    if (isSelected)
    {
        scene->debugDraw.boundingBox(decalBoundingBox, transform, glm::vec4(1, 1, 1, 1));
    }
    else
    {
        scene->debugDraw.boundingBox(decalBoundingBox, transform, glm::vec4(1, 0.5f, 0, 1));
    }
}

DataFile::Value StaticDecal::serialize()
{
    DataFile::Value dict = DataFile::makeDict();
    dict["entityID"] = DataFile::makeInteger((i64)SerializedEntityID::STATIC_DECAL);
    dict["position"] = DataFile::makeVec3(position);
    dict["rotation"] = DataFile::makeVec4({ rotation.x, rotation.y, rotation.z, rotation.w });
    dict["scale"] = DataFile::makeVec3(scale);
    dict["texIndex"] = DataFile::makeInteger(texIndex);
    dict["decalFilter"] = DataFile::makeInteger(decalFilter);
    dict["beforeMarking"] = DataFile::makeBool(beforeMarking);
    return dict;
}

void StaticDecal::deserialize(DataFile::Value& data)
{
    position = data["position"].vec3();
    glm::vec4 r = data["rotation"].vec4();
    rotation = glm::quat(r.w, r.x, r.y, r.z);
    scale = data["scale"].vec3();
    texIndex = (i32)data["texIndex"].integer(0);
    decalFilter = (u32)data["decalFilter"].integer(DECAL_TRACK);
    beforeMarking = data["beforeMarking"].boolean(false);
}

void StaticDecal::showDetails(Scene* scene)
{
    u32 prevDecalFilter = decalFilter;

    bool affectsTrack = decalFilter & DECAL_TRACK;
    g_gui.toggle("Affects Track", affectsTrack);
    bool affectsTerrain = decalFilter & DECAL_TERRAIN;
    g_gui.toggle("Affects Terrain", affectsTerrain);
    bool affectsSigns = decalFilter & DECAL_SIGN;
    g_gui.toggle("Affects Signs", affectsSigns);
    bool affectsRailings = decalFilter & DECAL_RAILING;
    g_gui.toggle("Affects Railings", affectsRailings);
    bool affectsGround = decalFilter & DECAL_GROUND;
    g_gui.toggle("Affects Ground", affectsGround);

    decalFilter = DECAL_PLACEHOLDER;
    if (affectsTrack)    decalFilter |= DECAL_TRACK;
    if (affectsTerrain)  decalFilter |= DECAL_TERRAIN;
    if (affectsSigns)    decalFilter |= DECAL_SIGN;
    if (affectsRailings) decalFilter |= DECAL_RAILING;
    if (affectsGround)   decalFilter |= DECAL_GROUND;

    if (prevDecalFilter != decalFilter)
    {
        updateTransform(scene);
    }

    g_gui.beginSelect("Texture", &texIndex, true);
    for (i32 i=0; i<(i32)ARRAY_SIZE(textureNames); ++i)
    {
        if (g_gui.option(textureNames[i], i, nullptr))
        {
            updateTransform(scene);
        }
    }
    g_gui.end();

    if (g_gui.toggle("Before Marking", beforeMarking))
    {
        updateTransform(scene);
    }
}
