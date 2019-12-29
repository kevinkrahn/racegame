#include "static_decal.h"
#include "../renderer.h"
#include "../scene.h"
#include "../game.h"
#include "../gui.h"
#include "../billboard.h"

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

StaticDecal::StaticDecal()
{
    scale = glm::vec3(16.f);
    rotation = glm::rotate(rotation, (f32)M_PI * 0.5f, glm::vec3(0, 1, 0));
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

void StaticDecal::onPreview(RenderWorld* rw)
{
    rw->setViewportCamera(0, glm::vec3(0.f, 0.1f, 20.f),
            glm::vec3(0.f), 1.f, 200.f, 50.f);
    rw->push(BillboardRenderable(tex, glm::vec3(0, 0, 2.f),
                glm::vec4(1.f), 8.f, 0.f, false));
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

DataFile::Value StaticDecal::serializeState()
{
    DataFile::Value dict = PlaceableEntity::serializeState();
    dict["texIndex"] = DataFile::makeInteger(texIndex);
    dict["decalFilter"] = DataFile::makeInteger(decalFilter);
    dict["beforeMarking"] = DataFile::makeBool(beforeMarking);
    return dict;
}

void StaticDecal::deserializeState(DataFile::Value& data)
{
    PlaceableEntity::deserializeState(data);
    scale = data["scale"].vec3();
    texIndex = (i32)data["texIndex"].integer(0);
    decalFilter = (u32)data["decalFilter"].integer(DECAL_TRACK);
    beforeMarking = data["beforeMarking"].boolean(false);

    this->setVariationIndex((u32)texIndex);
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

    if (g_gui.toggle("Before Marking", beforeMarking))
    {
        updateTransform(scene);
    }
}

u32 StaticDecal::getVariationCount() const
{
    return ARRAY_SIZE(textureNames);
}

void StaticDecal::setVariationIndex(u32 variationIndex)
{
    this->texIndex = variationIndex;
    Texture* textures[] = {
        &g_res.textures->decal_arrow,
        &g_res.textures->decal_arrow_left,
        &g_res.textures->decal_arrow_right,
        &g_res.textures->decal_crack,
        &g_res.textures->decal_patch1,
        &g_res.textures->decal_grunge1,
        &g_res.textures->decal_grunge2,
        &g_res.textures->decal_grunge3,
        &g_res.textures->decal_sand,
    };
    tex = textures[texIndex];
    decal.setTexture(tex);
}
