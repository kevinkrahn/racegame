#include "../resources.h"
#include "../imgui.h"
#include "resource_editor.h"
#include "resource_manager.h"

class TextureEditor : public ResourceEditor
{
public:
    void onUpdate(Resource* r, ResourceManager* rm, class Renderer* renderer, f32 deltaTime, u32 n) override
    {
        Texture& tex = *(Texture*)r;
        bool changed = false;

        bool isOpen = true;
        if (ImGui::Begin(tmpStr("Texture Properties###Texture Properties %i", n), &isOpen))
        {
            ImGui::PushItemWidth(150);
            ImGui::InputText("##Name", &tex.name);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (tex.getSourceFileCount() == 1)
            {
                if (ImGui::Button("Load Image"))
                {
                    const char* filename = chooseFile(true, "Image Files", { "*.png", "*.jpg", "*.bmp" },
                            tmpStr("%s/textures", ASSET_DIRECTORY));
                    if (filename)
                    {
                        tex.setSourceFile(0, path::relative(filename));
                        tex.regenerate();
                    }
                }
                ImGui::SameLine();
            }
            if (ImGui::Button("Reimport"))
            {
                tex.reloadSourceFiles();
            }

            ImGui::Gap();

            if (tex.getSourceFileCount() == 1 && tex.getPreviewHandle())
            {
                ImGui::BeginChild("Texture Preview",
                        { ImGui::GetWindowWidth(), (f32)min(tex.height, 300u) }, false,
                        ImGuiWindowFlags_HorizontalScrollbar);
                // TODO: Add controls to pan the image view and zoom in and out
                ImGui::Image((void*)(uintptr_t)tex.getPreviewHandle(), ImVec2(tex.width, tex.height));
                ImGui::EndChild();

                ImGui::Text(tex.getSourceFile(0).path.data());
                ImGui::Text("%i x %i", tex.width, tex.height);
            }
            else if (tex.getSourceFileCount() > 1)
            {
                for (u32 i=0; i<tex.getSourceFileCount(); ++i)
                {
                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, 80);
                    auto const& sf = tex.getSourceFile(i);
                    f32 ratio = 72.f / sf.width;
                    ImGui::Image((void*)(uintptr_t)sf.previewHandle, ImVec2(sf.width * ratio, sf.height * ratio));
                    ImGui::NextColumn();
                    if (ImGui::Button("Load File"))
                    {
                        const char* filename = chooseFile(true, "Image Files", { "*.png", "*.jpg", "*.bmp" },
                                    tmpStr("%s/textures", ASSET_DIRECTORY));
                        if (filename)
                        {
                            tex.setSourceFile(i, path::relative(filename));
                            tex.regenerate();
                        }
                    }
                    if (!sf.path.empty())
                    {
                        ImGui::Text(sf.path.data());
                    }
                    if (sf.width > 0 && sf.height > 0)
                    {
                        ImGui::Text("%i x %i", sf.width, sf.height);
                    }
                    ImGui::Columns(1);
                }
                ImGui::Gap();
            }

            ImGui::Guid(tex.guid);

            const char* textureTypeNames = "Color\0Grayscale\0Normal Map\0Cube Map\0";
            i32 textureType = tex.getTextureType();
            ImGui::Combo("Type", &textureType, textureTypeNames);
            if (textureType != tex.getTextureType())
            {
                if (tex.sourceFilesExist())
                {
                    tex.setTextureType(textureType);
                    tex.reloadSourceFiles();
                }
            }

            bool compressed = tex.compressed;
            if (ImGui::Checkbox("Compressed", &tex.compressed))
            {
                if (!tex.reloadSourceFiles())
                {
                    tex.compressed = compressed;
                }
            }
            if (textureType == TextureType::GRAYSCALE)
            {
                ImGui::HelpMarker("Compressed grayscale images cannot be in SRGB color space.");
            }
            if (textureType == TextureType::COLOR)
            {
                bool preserveAlpha = tex.preserveAlpha;
                if (ImGui::Checkbox("Preserve Alpha", &tex.preserveAlpha))
                {
                    if (!tex.reloadSourceFiles())
                    {
                        tex.preserveAlpha = preserveAlpha;
                    }
                }
            }
            if (textureType != TextureType::CUBE_MAP)
            {
                changed |= ImGui::Checkbox("Repeat", &tex.repeat);
            }
            changed |= ImGui::Checkbox("Generate Mip Maps", &tex.generateMipMaps);
            changed |= ImGui::InputFloat("LOD Bias", &tex.lodBias, 0.1f);

            changed |= ImGui::InputInt("Anisotropy", &tex.anisotropy);
            tex.anisotropy = clamp(tex.anisotropy, 0, 16);

            const char* filterNames = "Nearest\0Bilinear\0Trilinear\0";
            changed |= ImGui::Combo("Filtering", &tex.filter, filterNames);

            ImGui::End();
        }

        if (!isOpen)
        {
            rm->markClosed(r);
        }

        if (changed)
        {
            tex.regenerate();
        }
    }
};

