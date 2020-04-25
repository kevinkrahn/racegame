#pragma once

#include "../entity.h"
#include "../decal.h"
#include "../collision_flags.h"

class StaticDecal : public PlaceableEntity
{
    struct Texture* tex;
    Decal decal;
    u32 decalFilter = DECAL_TRACK;
    i32 texIndex = 0;
    bool beforeMarking = false;

public:
    StaticDecal();
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onRender(class RenderWorld* rw, class Scene* scene, f32 deltaTime) override;
    void onPreview(RenderWorld* rw) override;
    void onEditModeRender(class RenderWorld* rw, class Scene* scene, bool isSelected) override;
    void serializeState(Serializer& s) override
    {
        PlaceableEntity::serializeState(s);
        s.field(texIndex);
        s.field(decalFilter);
        s.field(beforeMarking);
        if (s.deserialize)
        {
            static Texture* textures[] = {
                g_res.getTexture("decal_arrow"),
                g_res.getTexture("decal_arrow_left"),
                g_res.getTexture("decal_arrow_right"),
                g_res.getTexture("decal_crack"),
                g_res.getTexture("decal_patch1"),
                g_res.getTexture("decal_grunge1"),
                g_res.getTexture("decal_grunge2"),
                g_res.getTexture("decal_grunge3"),
                g_res.getTexture("decal_sand"),
            };
            tex = textures[texIndex];
            decal.setTexture(tex);
        }
    }
    void showDetails(Scene* scene) override;
};
