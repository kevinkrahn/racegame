#pragma once

#define IMGUI_STB_TRUETYPE_FILENAME   <stb_truetype.h>
#define IMGUI_STB_RECT_PACK_FILENAME  <stb_rect_pack.h>
#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#define IMGUI_USE_STB_SPRINTF
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

namespace ImGui
{
    // NOTE: custom ImGui functions go here

    inline void Gap()
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    inline void HelpMarker(const char* desc, bool sameLine=true)
    {
        if (sameLine)
        {
            ImGui::SameLine();
        }
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}
