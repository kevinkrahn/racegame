#include "../resources.h"
#include "../imgui.h"
#include "resource_editor.h"
#include "resource_manager.h"

class MaterialEditor : public ResourceEditor
{
    RenderWorld rw;
    i32 previewMeshIndex = 0;

public:
    ~MaterialEditor()
    {
        rw.destroy();
    }

    void onUpdate(Resource* r, ResourceManager* rm, class Renderer* renderer, f32 deltaTime, u32 n) override
    {
        Material& mat = *(Material*)r;
        mat.loadShaderHandles();

        rw.setName("Material Preview");
        rw.setSize(200, 200);
        rw.setBloomForceOff(true);
        // TODO: find out why normal maps don't look right in the preview
        const char* previewMeshes[] = { "world.Sphere", "world.UnitCube", "world.Quad" };
        Mesh* previewMesh = g_res.getModel("misc")->getMeshByName(previewMeshes[previewMeshIndex]);
        rw.addDirectionalLight(Vec3(-0.5f, 0.2f, -1.f), Vec3(1.5f));
        rw.setViewportCount(1);
        rw.updateWorldTime(30.f);
        rw.setClearColor(true, { 0.05f, 0.05f, 0.05f, 1.f });
        rw.setViewportCamera(0, Vec3(8.f, 8.f, 10.f),
                Vec3(0.f, 0.f, 1.f), 1.f, 200.f, 40.f);
        Mat4 transform = Mat4::scaling(Vec3(3.5f));
        mat.draw(&rw, transform, previewMesh);

        renderer->addRenderWorld(&rw);

        bool isOpen = true;
        bool dirty = false;
        if (ImGui::Begin(tmpStr("Material Properties###Material Properties %i", n), &isOpen))
        {
            ImGui::PushItemWidth(200);
            dirty |= ImGui::InputText("##Name", &mat.name);
            ImGui::PopItemWidth();
            ImGui::Gap();

            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, 208);
            ImGui::Image((void*)(uintptr_t)rw.getTexture()->handle, { 200, 200 }, { 1.f, 1.f }, { 0.f, 0.f });
            ImGui::NextColumn();

            ImGui::Text(mat.name.data());
            ImGui::Guid(mat.guid);

            const char* materialTypeNames = "Normal\0Terrain\0Track\0";
            dirty |= ImGui::Combo("Type", (i32*)&mat.materialType, materialTypeNames);

            const char* previewMeshNames = "Sphere\0Box\0Plane\0";
            dirty |= ImGui::Combo("Preview", &previewMeshIndex, previewMeshNames);

            ImGui::Columns(1);
            ImGui::Gap();

            chooseTexture(TextureType::COLOR, mat.colorTexture, "Color Texture");
            chooseTexture(TextureType::NORMAL_MAP, mat.normalMapTexture, "Normal Map");

            ImGui::Columns(2, nullptr, false);
            dirty |= ImGui::Checkbox("Culling", &mat.isCullingEnabled);
            dirty |= ImGui::Checkbox("Cast Shadow", &mat.castsShadow);
            dirty |= ImGui::Checkbox("Depth Read", &mat.isDepthReadEnabled);
            dirty |= ImGui::Checkbox("Depth Write", &mat.isDepthWriteEnabled);
            ImGui::NextColumn();
            dirty |= ImGui::Checkbox("Visible", &mat.isVisible);
            dirty |= ImGui::Checkbox("Wireframe", &mat.displayWireframe);
            dirty |= ImGui::Checkbox("Transparent", &mat.isTransparent);
            dirty |= ImGui::Checkbox("Vertex Colors", &mat.useVertexColors);
            ImGui::Columns(1);

            dirty |= ImGui::DragFloat("Alpha Cutoff", &mat.alphaCutoff, 0.005f, 0.f, 1.f);
            dirty |= ImGui::DragFloat("Shadow Alpha Cutoff", &mat.shadowAlphaCutoff, 0.005f, 0.f, 1.f);
            dirty |= ImGui::InputFloat("Depth Offset", &mat.depthOffset);

            dirty |= ImGui::ColorEdit3("Base Color", (f32*)&mat.color);
            dirty |= ImGui::ColorEdit3("Emit", (f32*)&mat.emit);
            dirty |= ImGui::DragFloat("Emit Strength", (f32*)&mat.emitPower, 0.01f, 0.f, 80.f);
            dirty |= ImGui::ColorEdit3("Specular Color", (f32*)&mat.specularColor);
            dirty |= ImGui::DragFloat("Specular Power", (f32*)&mat.specularPower, 0.05f, 0.f, 1000.f);
            dirty |= ImGui::DragFloat("Specular Strength", (f32*)&mat.specularStrength, 0.005f, 0.f, 1.f);

            dirty |= ImGui::DragFloat("Fresnel Scale", (f32*)&mat.fresnelScale, 0.005f, 0.f, 1.f);
            dirty |= ImGui::DragFloat("Fresnel Power", (f32*)&mat.fresnelPower, 0.009f, 0.f, 200.f);
            dirty |= ImGui::DragFloat("Fresnel Bias", (f32*)&mat.fresnelBias, 0.005f, -1.f, 1.f);

            dirty |= ImGui::DragFloat("Reflection Strength", (f32*)&mat.reflectionStrength, 0.005f, 0.f, 1.f);
            dirty |= ImGui::DragFloat("Reflection LOD", (f32*)&mat.reflectionLod, 0.01f, 0.f, 10.f);
            dirty |= ImGui::DragFloat("Reflection Bias", (f32*)&mat.reflectionBias, 0.005f, -1.f, 1.f);
            dirty |= ImGui::DragFloat("Wind", (f32*)&mat.windAmount, 0.01f, 0.f, 5.f);

            ImGui::End();
        }

        if (!isOpen)
        {
            rm->markClosed(r);
        }

        if (dirty)
        {
            rm->markDirty(mat.guid);
        }
    }
};

