#pragma once

#include "editor_mode.h"
#include "../input.h"
#include "../renderer.h"
#include "../imgui.h"
#include "../scene.h"
#include "../game.h"
#include "../mesh_renderables.h"
#include "transform_gizmo.h"

class DecorationMode : public EditorMode, public TransformGizmoHandler
{
    struct PropPrefab
    {
        Texture icon;
        bool hasIcon;
        u32 entityIndex;
        PropPrefabData prefabData;
    };
    std::vector<PropPrefab> propPrefabs;

    TransformGizmo transformGizmo;
    Scene* scene;

    std::vector<PlaceableEntity*> selectedEntities;
    std::vector<DataFile::Value> serializedTransientEntities;
    std::vector<u32> selectedPropTypes;

    bool randomizeRotationX = false;
    bool randomizeRotationY = false;
    bool randomizeRotationZ = true;
    f32 randomizeScaleMin = 0.85f;
    f32 randomizeScaleMax = 1.15f;

    bool selectionStateCtrl = false;
    bool selectionStateShift = false;

    void showEntityIcons()
    {
        u32 count = (u32)propPrefabs.size();
        u32 iconSize = 48;

        static RenderWorld renderWorld;
        static i32 lastEntityIconRendered = -1;

        if (lastEntityIconRendered != -1)
        {
            propPrefabs[lastEntityIconRendered].icon = renderWorld.releaseTexture();
            propPrefabs[lastEntityIconRendered].hasIcon = true;
            lastEntityIconRendered = -1;
        }
        for (u32 categoryIndex=0; categoryIndex<ARRAY_SIZE(propCategoryNames); ++categoryIndex)
        {
            if (!ImGui::CollapsingHeader(propCategoryNames[categoryIndex]))
            {
                continue;
            }

            u32 buttonCount = 0;
            for (u32 i=0, itemIndex = 0; itemIndex<count; ++itemIndex)
            {
                if ((u32)propPrefabs[itemIndex].prefabData.category != categoryIndex)
                {
                    continue;
                }

                if (propPrefabs[itemIndex].hasIcon)
                {
                    if (buttonCount % 5 != 0)
                    {
                        ImGui::SameLine();
                    }
                    ImGui::PushID(itemIndex);
                    auto selectIt = std::find(selectedPropTypes.begin(), selectedPropTypes.end(), itemIndex);
                    bool isButtonSelected = selectIt != selectedPropTypes.end();
                    if (isButtonSelected)
                    {
                        const u32 selectedColor = 0x992299EE;
                        ImGui::PushStyleColor(ImGuiCol_Button, selectedColor);
                    }
                    if (ImGui::ImageButton((void*)(uintptr_t)propPrefabs[itemIndex].icon.handle,
                                ImVec2(iconSize, iconSize), {1,1}, {0,0}))
                    {
                        if (!isButtonSelected && g_input.isKeyDown(KEY_LCTRL))
                        {
                            selectedPropTypes.push_back(itemIndex);
                        }
                        else if (isButtonSelected && g_input.isKeyDown(KEY_LSHIFT))
                        {
                            selectedPropTypes.erase(selectIt);
                        }
                        else if (!g_input.isKeyDown(KEY_LCTRL) && !g_input.isKeyDown(KEY_LSHIFT))
                        {
                            selectedPropTypes.clear();
                            selectedPropTypes.push_back(itemIndex);
                        }
                    }
                    if (isButtonSelected)
                    {
                        ImGui::PopStyleColor();
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                        ImGui::TextUnformatted(propPrefabs[itemIndex].prefabData.name.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                    ImGui::PopID();
                    ++buttonCount;
                }
                else if (lastEntityIconRendered == -1)
                {
                    renderWorld.setName("Entity Icon");
                    renderWorld.setSize(iconSize*2, iconSize*2);
                    renderWorld.setClearColor(true, glm::vec4(0.15f, 0.15f, 0.15f, 1.f));
                    Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
                    renderWorld.push(LitRenderable(quadMesh,
                                glm::scale(glm::mat4(1.f), glm::vec3(200.f)), nullptr, glm::vec3(0.15f)));
                    renderWorld.addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.5f));
                    renderWorld.setViewportCount(1);
                    renderWorld.updateWorldTime(30.f);
                    renderWorld.setViewportCamera(0, glm::vec3(8.f, 8.f, 10.f),
                            glm::vec3(0.f, 0.f, 1.f), 1.f, 220.f, 39.f);
                    static std::unique_ptr<PlaceableEntity> ptr;
                    ptr.reset((PlaceableEntity*)
                            g_entities[propPrefabs[itemIndex].entityIndex].create());
                    propPrefabs[itemIndex].prefabData.doThing(ptr.get());
                    ptr->onPreview(&renderWorld);
                    g_game.renderer->addRenderWorld(&renderWorld);
                    lastEntityIconRendered = (i32)itemIndex;
                }
                ++i;
            }
        }
    }

    void showEntityListWindow()
    {
        if (ImGui::Begin("Entities"))
        {
            for (auto& entity : scene->getEntities())
            {
                if (!(entity->entityFlags & Entity::PROP))
                {
                    continue;
                }
                auto it = std::find_if(selectedEntities.begin(), selectedEntities.end(),
                        [&](PlaceableEntity* e){ return e == entity.get(); });
                ImGui::PushID((void*)entity.get());
                if (ImGui::Selectable(((PlaceableEntity*)entity.get())->getName(), it != selectedEntities.end()))
                {
                    if (!g_input.isKeyDown(KEY_LCTRL) && !g_input.isKeyDown(KEY_LSHIFT))
                    {
                        selectedEntities.clear();
                        selectedEntities.push_back((PlaceableEntity*)entity.get());
                    }
                    else
                    {
                        if (it == selectedEntities.end() && !g_input.isKeyDown(KEY_LSHIFT))
                        {
                            selectedEntities.push_back((PlaceableEntity*)entity.get());
                        }
                        if (it != selectedEntities.end() && !g_input.isKeyDown(KEY_LCTRL))
                        {
                            selectedEntities.erase(it);
                        }
                    }
                }
                ImGui::PopID();
            }
        }
        ImGui::End();
    }

public:
    DecorationMode() : EditorMode("Props")
    {
        for (u32 entityIndex = 0; entityIndex < (u32)g_entities.size(); ++entityIndex)
        {
            auto& e = g_entities[entityIndex];
            if (e.isPlaceableInEditor)
            {
                std::unique_ptr<PlaceableEntity> ptr((PlaceableEntity*)e.create());
                std::vector<PropPrefabData> data = ptr->generatePrefabProps();
                for (auto& d : data)
                {
                    propPrefabs.push_back({ {}, false, entityIndex, std::move(d) });
                }
            }
        }
    }
    ~DecorationMode()
    {
        for (auto& pp : propPrefabs)
        {
            pp.icon.destroy();
        }
    }

    void onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        this->scene = scene;
        bool isMouseClickHandled = ImGui::GetIO().WantCaptureMouse;

        RenderWorld* rw = renderer->getRenderWorld();
        glm::vec3 rayDir = scene->getEditorCamera().getMouseRay(rw);
        Camera const& cam = scene->getEditorCamera().getCamera();

        if (selectedEntities.size() > 0)
        {
            glm::vec3 minP(FLT_MAX);
            glm::vec3 maxP(-FLT_MAX);
            for (PlaceableEntity* e : selectedEntities)
            {
                minP = glm::min(minP, e->position);
                maxP = glm::max(maxP, e->position);
            }
            glm::vec3 p = minP + (maxP - minP) * 0.5f;
            glm::mat4 orientation(1.f);
            if (selectedEntities.size() == 1)
            {
                orientation = glm::mat4_cast(selectedEntities[0]->rotation);
            }
            isMouseClickHandled = transformGizmo.update(p, scene, rw, deltaTime, orientation, this);
        }

        if (selectedEntities.size() > 0)
        {
            ImGui::Begin("Entity Properties");
            ImGui::Text("%s", selectedEntities.front()->getName());
            ImGui::Gap();
            if (ImGui::InputFloat3("Position", (f32*)&selectedEntities[0]->position))
            {
                selectedEntities[0]->updateTransform(scene);
            }
            if (ImGui::InputFloat3("Scale", (f32*)&selectedEntities[0]->scale))
            {
                selectedEntities[0]->updateTransform(scene);
            }
            glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(selectedEntities[0]->rotation));
            if (ImGui::InputFloat3("Rotation", (f32*)&eulerAngles))
            {
                selectedEntities[0]->rotation = glm::quat(glm::radians(eulerAngles));
                selectedEntities[0]->updateTransform(scene);
            }
            ImGui::Gap();
            selectedEntities.front()->showDetails(scene);
            ImGui::End();
        }

        showEntityListWindow();

        if (const u32* pixelID = rw->getPickPixelResult())
        {
            if (!selectionStateCtrl && !selectionStateShift)
            {
                selectedEntities.clear();
            }

            if (*pixelID != 0)
            {
                for (auto& e : scene->getEntities())
                {
                    if (e->entityCounterID == *pixelID)
                    {
                        auto it = std::find(selectedEntities.begin(), selectedEntities.end(), e.get());
                        if (selectionStateShift)
                        {
                            if (it != selectedEntities.end())
                            {
                                selectedEntities.erase(it);
                            }
                        }
                        else
                        {
                            if (it == selectedEntities.end())
                            {
                                selectedEntities.push_back((PlaceableEntity*)e.get());
                            }
                        }
                    }
                }

            }
            else
            {
                PxRaycastBuffer hit;
                if (scene->raycastStatic(cam.position, rayDir, 10000.f,
                            &hit, COLLISION_FLAG_SELECTABLE))
                {
                    if (hit.block.actor)
                    {
                        ActorUserData* userData = (ActorUserData*)hit.block.actor->userData;
                        if (userData->entityType == ActorUserData::SELECTABLE_ENTITY)
                        {
                            auto it = std::find(selectedEntities.begin(),
                                selectedEntities.end(), userData->placeableEntity);
                            if (selectionStateShift)
                            {
                                if (it != selectedEntities.end())
                                {
                                    selectedEntities.erase(it);
                                }
                            }
                            else
                            {
                                if (it == selectedEntities.end())
                                {
                                    selectedEntities.push_back((PlaceableEntity*)userData->entity);
                                }
                            }
                        }
                    }
                }
            }
        }

        if (!isMouseClickHandled)
        {
            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                isMouseClickHandled = true;
                if (g_input.isKeyDown(KEY_LCTRL) && g_input.isKeyDown(KEY_LSHIFT) && !selectedPropTypes.empty())
                {
                    PxRaycastBuffer hit;
                    if (scene->raycastStatic(cam.position, rayDir, 10000.f, &hit))
                    {
                        u32 propTypeIndex = selectedPropTypes[
                            irandom(scene->randomSeries, 0, (u32)selectedPropTypes.size())];
                        glm::vec3 hitPoint = convert(hit.block.position);
                        PlaceableEntity* newEntity =
                            (PlaceableEntity*)g_entities[propPrefabs[propTypeIndex].entityIndex].create();
                        propPrefabs[propTypeIndex].prefabData.doThing(newEntity);
                        newEntity->position = hitPoint;
                        newEntity->scale *= random(scene->randomSeries, randomizeScaleMin, randomizeScaleMax);;
                        glm::vec3 eulerAngles =  {
                            randomizeRotationX ? random(scene->randomSeries, 0.f, PI2) : 0,
                            randomizeRotationY ? random(scene->randomSeries, 0.f, PI2) : 0,
                            randomizeRotationZ ? random(scene->randomSeries, 0.f, PI2) : 0,
                        };
                        newEntity->rotation = glm::quat(eulerAngles) * newEntity->rotation;
                        newEntity->updateTransform(scene);
                        newEntity->setPersistent(true);
                        scene->addEntity(newEntity);
                    }
                }
                else
                {
                    rw->pickPixel(g_input.getMousePosition()
                            / glm::vec2(g_game.windowWidth, g_game.windowHeight));
                    selectionStateCtrl = g_input.isKeyDown(KEY_LCTRL);
                    selectionStateShift = g_input.isKeyDown(KEY_LSHIFT);
                }
            }
        }

        for (u32 i=0; (u32)i<scene->getEntities().size(); ++i)
        {
            auto& e = scene->getEntities()[i];
            auto it = std::find(selectedEntities.begin(),
                    selectedEntities.end(), e.get());
            u8 selectIndexByte = 0;
            bool isSelected = it != selectedEntities.end();
            if (isSelected)
            {
                u32 selectIndex = it - selectedEntities.begin();
                // NOTE: first byte is reserved for hidden flag
                selectIndexByte = (u8)((selectIndex % 126) + 1) << 1;
            }
            e->onEditModeRender(rw, scene, isSelected, selectIndexByte);
        }
    }

    void onEditorTabGui(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        ImGui::Spacing();

        ImGui::ListBoxHeader("Transform Mode", {0,70});
        if (ImGui::Selectable("Translate [g]", transformGizmo.getTransformMode() == TransformMode::TRANSLATE))
        {
            transformGizmo.setTransformMode(TransformMode::TRANSLATE);
            transformGizmo.reset();
        }
        if (ImGui::Selectable("Rotate [r]", transformGizmo.getTransformMode() == TransformMode::ROTATE))
        {
            transformGizmo.setTransformMode(TransformMode::ROTATE);
            transformGizmo.reset();
        }
        if (ImGui::Selectable("Scale [f]", transformGizmo.getTransformMode() == TransformMode::SCALE))
        {
            transformGizmo.setTransformMode(TransformMode::SCALE);
            transformGizmo.reset();
        }
        ImGui::ListBoxFooter();

        ImGui::Gap();

        auto buttonSize = ImVec2(ImGui::GetWindowWidth() * 0.65f, 0);
        if (ImGui::Button("Duplicate [b]", buttonSize) > 0 || g_input.isKeyPressed(KEY_B))
        {
            std::vector<PlaceableEntity*> newEntities;
            for (auto e : selectedEntities)
            {
                auto data = DataFile::makeDict();
                Serializer s(data, false);
                e->serialize(s);
                PlaceableEntity* newEntity = (PlaceableEntity*)scene->deserializeEntity(data);
                newEntity->setPersistent(true);
                newEntities.push_back(newEntity);
            }
            selectedEntities.clear();
            for (auto e : newEntities)
            {
                selectedEntities.push_back(e);
            }
        }

        if (ImGui::Button("Delete [DELETE]", buttonSize) || g_input.isKeyPressed(KEY_DELETE))
        {
            for (PlaceableEntity* e : selectedEntities)
            {
                e->destroy();
            }
            selectedEntities.clear();
        }

        ImGui::Gap();

        ImGui::Checkbox("Randomize Rotation X", &randomizeRotationX);
        ImGui::Checkbox("Randomize Rotation Y", &randomizeRotationY);
        ImGui::Checkbox("Randomize Rotation Z", &randomizeRotationZ);
        ImGui::DragFloatRange2("Scale Range", &randomizeScaleMin, &randomizeScaleMax, 0.01f, 0.1f, 10.f);

        ImGui::Gap();

        ImGui::BeginChild("Entites");
        showEntityIcons();
        ImGui::EndChild();
    }

    void onBeginTest(Scene* scene) override
    {
        serializedTransientEntities = scene->serializeTransientEntities();
        transformGizmo.reset();
    }

    void onEndTest(Scene* scene) override
    {
        scene->deserializeTransientEntities(serializedTransientEntities);
    }

    void onSwitchFrom(Scene* scene) override
    {
        transformGizmo.reset();
    }

    void gizmoDrag(glm::vec3 const& dragCenter, glm::vec3 const& dragDest, glm::vec3 const& dragDestZ,
            glm::vec3 dragOffset, i32 dragAxis) override
    {
        for (PlaceableEntity* e : selectedEntities)
        {
            if (dragAxis & DragAxis::X)
            {
                f32 diff = dragCenter.x - e->position.x;
                e->position.x = dragDest.x + dragOffset.x - diff;
            }

            if (dragAxis & DragAxis::Y)
            {
                f32 diff = dragCenter.y - e->position.y;
                e->position.y = dragDest.y + dragOffset.y - diff;
            }

            if (dragAxis & DragAxis::Z)
            {
                f32 diff = dragCenter.z - e->position.z;
                e->position.z = dragDestZ.z + dragOffset.z - diff;
            }

            e->updateTransform(scene);
        }
    }

    void gizmoRotate(f32 angleDiff, glm::vec3 const& rotatePivot, i32 dragAxis) override
    {
        glm::vec3 rotationAxis(1, 0, 0);
        if (dragAxis & DragAxis::X)
        {
            rotationAxis = glm::vec3(1, 0, 0);
        }
        else if (dragAxis & DragAxis::Y)
        {
            rotationAxis = glm::vec3(0, 1, 0);
        }
        else if (dragAxis & DragAxis::Z)
        {
            rotationAxis = glm::vec3(0, 0, 1);
        }
        glm::mat4 transform = glm::rotate(glm::mat4(1.f), angleDiff, rotationAxis) *
            glm::translate(glm::mat4(1.f), -rotatePivot);
        for (PlaceableEntity* e : selectedEntities)
        {
            e->position = glm::vec3(transform * glm::vec4(e->position, 1.f)) + rotatePivot;
            e->rotation = glm::rotate(glm::identity<glm::quat>(),
                    angleDiff, rotationAxis) * e->rotation;
            e->updateTransform(scene);
        }
    }

    void gizmoScale(f32 scaleFactor, i32 entityDragAxis, glm::vec3 const& scaleCenter,
            glm::vec3 const& dragOffset, glm::vec3 const& hitPos, glm::vec3& hitPosZ) override
    {
        if (selectedEntities.size() > 1)
        {
            for (PlaceableEntity* e : selectedEntities)
            {
                glm::mat4 t = glm::translate(glm::mat4(1.f), e->position)
                    * glm::scale(glm::mat4(1.f), glm::vec3(e->scale));
                glm::mat4 transform = glm::scale(glm::mat4(1.f), glm::vec3(scaleFactor))
                    * glm::translate(glm::mat4(1.f), -scaleCenter) * t;
                e->position = translationOf(transform) + scaleCenter;
                e->scale *= scaleFactor;
            }
        }
        else if ((entityDragAxis & DragAxis::MIDDLE) == DragAxis::MIDDLE)
        {
            selectedEntities[0]->scale *= scaleFactor;
        }
        else if (entityDragAxis & DragAxis::X)
        {
            f32 startDistance = glm::dot(dragOffset, glm::vec3(1, 0, 0));
            selectedEntities[0]->scale.x *= glm::dot(scaleCenter - hitPos, glm::vec3(1, 0, 0)) / startDistance;
        }
        else if (entityDragAxis & DragAxis::Y)
        {
            f32 startDistance = glm::dot(dragOffset, glm::vec3(0, 1, 0));
            selectedEntities[0]->scale.y *= glm::dot(scaleCenter - hitPos, glm::vec3(0, 1, 0)) / startDistance;
        }
        else if (entityDragAxis & DragAxis::Z)
        {
            f32 startDistance = glm::dot(dragOffset, glm::vec3(0, 0, 1));
            selectedEntities[0]->scale.z *= glm::dot(scaleCenter - hitPosZ, glm::vec3(0, 0, 1)) / startDistance;
        }

        for (PlaceableEntity* e : selectedEntities)
        {
            e->updateTransform(scene);
        }
    }
};
