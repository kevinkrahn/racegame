#pragma once

#include "../entity.h"
#include "../decal.h"
#include "../collision_flags.h"
#include "../game.h"

class StaticDecal : public PlaceableEntity
{
    Texture* tex;
    Decal decal;
    u32 decalFilter = DECAL_TRACK;
    i64 textureGuid = 0;
    bool beforeMarking = false;
    bool isDust = false;

public:
    StaticDecal();
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onRender(class RenderWorld* rw, class Scene* scene, f32 deltaTime) override;
    void onPreview(RenderWorld* rw) override;
    void onEditModeRender(class RenderWorld* rw, class Scene* scene, bool isSelected, u8 selectIndex) override;
    void serializeState(Serializer& s) override
    {
        PlaceableEntity::serializeState(s);
        s.field(decalFilter);
        s.field(beforeMarking);
        s.field(textureGuid);
        s.field(isDust);

        if (s.deserialize)
        {
            setTexture(textureGuid);
            if (textureGuid == 0)
            {
                // TODO: remove when scenes are updated
                u32 texIndex = 0;
                s.field(texIndex);
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
                setTexture(textures[texIndex]->guid);
                if (tex->name.find("dust") != std::string::npos || tex->name.find("sand") != std::string::npos)
                {
                    isDust = true;
                }
            }
        }
    }
    void showDetails(Scene* scene) override;
    void setTexture(i64 guid)
    {
        textureGuid = guid;
        tex = g_res.getTexture(guid);
        decal.setTexture(tex);
    }
    Array<PropPrefabData> generatePrefabProps() override;
    const char* getName() const override { return tstr("Decal ", tex->name.c_str()); }
};
