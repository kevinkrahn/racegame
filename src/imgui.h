#pragma once

#define IMGUI_STB_TRUETYPE_FILENAME   <stb_truetype.h>
#define IMGUI_STB_RECT_PACK_FILENAME  <stb_rect_pack.h>
#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#define IMGUI_USE_STB_SPRINTF
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#include "str.h"
#include <imgui/imgui.h>

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

    template <u32 SIZE>
    inline bool InputText(const char* label, Str<SIZE>* str, ImGuiInputTextFlags flags=0)
    {
        return ImGui::InputText(label, str->data(), SIZE, flags);
    }

    inline void Guid(i64 guid)
    {
        ImGui::TextDisabled("GUID: %x", guid);
        ImGui::SameLine();
        if (ImGui::SmallButton("Copy"))
        {
            ImGui::LogToClipboard();
            ImGui::LogText("%x", guid);
            ImGui::LogFinish();
        }
    }
}
