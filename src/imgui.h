#pragma once

#define IMGUI_STB_TRUETYPE_FILENAME   <stb_truetype.h>
#define IMGUI_STB_RECT_PACK_FILENAME  <stb_rect_pack.h>
#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#define IMGUI_USE_STB_SPRINTF
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#include "str.h"
#include <SDL2/SDL.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

namespace ImGui
{
    // NOTE: custom ImGui functions go here

    inline void Initialize(SDL_Window* window, SDL_GLContext context)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        ImGui::StyleColorsDark();
        ImGui_ImplSDL2_InitForOpenGL(window, context);
        ImGui_ImplOpenGL3_Init("#version 130");
        io.Fonts->AddFontFromFileTTF("font.ttf", 16.f);
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowBorderSize = 1.f;
        style.FrameBorderSize = 0.f;
        style.PopupBorderSize = 1.f;
        style.WindowRounding = 2.f;
        style.FrameRounding = 2.f;
        style.TabRounding = 6.f;
        style.Colors[2] = { 0.02f, 0.02f, 0.02f, 1.f };
    }

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

    inline bool InputFloatClamp(const char* label, f32* v, f32 min, f32 max, f32 step=0.f, f32 stepFast=0.f,
            const char* format="%.3f", u32 flags=0)
    {
        f32 previousValue = *v;
        InputFloat(label, v, step, stepFast, format, flags);
        *v = clamp(*v, min, max);
        return *v != previousValue;
    }
}
