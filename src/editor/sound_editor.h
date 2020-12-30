#include "../resources.h"
#include "../imgui.h"
#include "resource_editor.h"
#include "resource_manager.h"

class SoundEditor : public ResourceEditor
{
public:
    void onUpdate(Resource* r, ResourceManager* rm, class Renderer* renderer, f32 deltaTime) override
    {
        Sound& sound = *(Sound*)r;

        bool dirty = false;
        bool isOpen = true;
        if (ImGui::Begin("Sound Properties", &isOpen))
        {
            ImGui::PushItemWidth(150);
            dirty |= ImGui::InputText("##Name", &sound.name);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("Load Sound"))
            {
                const char* filename =
                    chooseFile(true, "Audio Files", { "*.wav", "*.ogg" }, tmpStr("%s/sounds", ASSET_DIRECTORY));
                if (filename)
                {
                    sound.sourceFilePath = path::relative(filename);
                    sound.loadFromFile(filename);
                }
                dirty = true;
            }
            ImGui::SameLine();
            if (!sound.sourceFilePath.empty() && ImGui::Button("Reimport"))
            {
                sound.loadFromFile(tmpStr("%s/%s", ASSET_DIRECTORY, sound.sourceFilePath.data()));
                dirty = true;
            }

            ImGui::Gap();

            ImGui::Text(sound.sourceFilePath.data());
            ImGui::Text("Format: %s", sound.format == AudioFormat::RAW ? "WAV" : "OGG VORBIS");
            ImGui::Guid(sound.guid);

            dirty |= ImGui::SliderFloat("Volume", &sound.volume, 0.f, 1.f);
            dirty |= ImGui::SliderFloat("Falloff Distance", &sound.falloffDistance, 50.f, 1000.f);

            ImGui::Gap();

            static SoundHandle previewSoundHandle = 0;
            if (ImGui::Button("Preview"))
            {
                g_audio.stopSound(previewSoundHandle);
                previewSoundHandle = g_audio.playSound(&sound, SoundType::MENU_SFX);
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop"))
            {
                g_audio.stopSound(previewSoundHandle);
            }
        }
        ImGui::End();

        if (!isOpen)
        {
            rm->closeResource(r);
        }

        if (dirty)
        {
            rm->markDirty(r->guid);
        }
    }
};

