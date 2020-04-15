#pragma once

#include <glm/vec4.hpp>
#include <glm/vec2.hpp>

#define IM_VEC2_CLASS_EXTRA                                                 \
        ImVec2(glm::vec2 const& f) { x = f.x; y = f.y; }                    \
        operator glm::vec2() const { return glm::vec2(x, y); }
#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(glm::vec4 const& f) { x = f.x; y = f.y; z = f.z; w = f.w; }   \
        operator glm::vec4() const { return glm::vec4(x, y, z, w); }

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
}
