#include "editor.h"
#include "game.h"
#include "input.h"
#include "renderer.h"
#include "scene.h"
#include "input.h"
#include "terrain.h"
#include "mesh_renderables.h"
#include "track.h"

void Editor::onStart(Scene* scene)
{

}

void Editor::onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime)
{
    renderer->setViewportCount(1);
    renderer->addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));

    f32 right = (f32)g_input.isKeyDown(KEY_D) - (f32)g_input.isKeyDown(KEY_A);
    f32 up = (f32)g_input.isKeyDown(KEY_S) - (f32)g_input.isKeyDown(KEY_W);
    glm::vec3 moveDir = (right != 0.f || up != 0.f) ? glm::normalize(glm::vec3(right, up, 0.f)) : glm::vec3(0, 0, 0);
    glm::vec3 forward(lengthdir(cameraAngle, 1.f), 0.f);
    glm::vec3 sideways(lengthdir(cameraAngle + M_PI / 2.f, 1.f), 0.f);

    cameraVelocity += (((forward * moveDir.y) + (sideways * moveDir.x)) * (deltaTime * (120.f + cameraDistance * 1.5f)));
    cameraTarget += cameraVelocity * deltaTime;
    cameraVelocity = smoothMove(cameraVelocity, glm::vec3(0, 0, 0), 7.f, deltaTime);

    if (g_input.isMouseButtonPressed(MOUSE_RIGHT))
    {
        lastMousePosition = g_input.getMousePosition();
    }
    else if (g_input.isMouseButtonDown(MOUSE_RIGHT))
    {
        cameraRotateSpeed = (((lastMousePosition.x) - g_input.getMousePosition().x) / g_game.windowWidth * 2.f) * (1.f / deltaTime);
        lastMousePosition = g_input.getMousePosition();
    }

    cameraAngle += cameraRotateSpeed * deltaTime;
    cameraRotateSpeed = smoothMove(cameraRotateSpeed, 0, 8.f, deltaTime);

    bool isMouseClickHandled = false;
    //bool isMouseWheelHandled = g_input.getMouseScroll() != 0;

    if (g_input.isKeyDown(KEY_LCTRL) && g_input.getMouseScroll() != 0)
    {
        brushRadius = clamp(brushRadius + g_input.getMouseScroll(), 2.0f, 40.f);
    }
    else if (g_input.getMouseScroll() != 0)
    {
        zoomSpeed = g_input.getMouseScroll() * 1.25f;
    }
    cameraDistance = clamp(cameraDistance - zoomSpeed, 10.f, 180.f);
    zoomSpeed = smoothMove(zoomSpeed, 0.f, 10.f, deltaTime);

    glm::vec3 cameraFrom = cameraTarget + glm::normalize(glm::vec3(lengthdir(cameraAngle, 1.f), 1.25f)) * cameraDistance;
    renderer->setViewportCamera(0, cameraFrom, cameraTarget, 10, 400.f);

    glm::vec2 mousePos = g_input.getMousePosition();
    mousePos.y = g_game.windowHeight - mousePos.y;
    Camera const& cam = renderer->getCamera(0);
    glm::vec3 rayDir = screenToWorldRay(mousePos,
            glm::vec2(g_game.windowWidth, g_game.windowHeight), cam.view, cam.projection);

    f32 height = g_game.windowHeight;
    Font* font = &g_resources.getFont("font", height * 0.03f);
    Font* fontSmall = &g_resources.getFont("font", height * 0.018f);

    if (g_input.isKeyPressed(KEY_TAB))
    {
        editMode = EditMode(((u32)editMode + 1) % (u32)EditMode::MAX);
    }
    f32 modeSelectionMaxY = 0.f;
    {
        const char* modeNames[] = { "Terrain", "Track", "Decoration" };
        const char* icons[] = { "terrain_icon", "track_icon", "decoration_icon" };
        glm::vec2 offset(height * 0.012f);
        u32 padding = height * 0.015f;
        u32 buttonHeight = height * 0.03f;
        f32 textHeight = font->getHeight();
        for (u32 i=0; i<(u32)EditMode::MAX; ++i)
        {
            u32 buttonWidth = height * 0.16f;
            f32 alpha = 0.75f;
            glm::vec3 color(0.f);
            if (i == (u32)editMode)
            {
                buttonWidth = height * 0.20f;
                alpha = 0.9f;
                color = glm::vec3(0.3f);
            }
            if (pointInRectangle(g_input.getMousePosition(),
                        offset + glm::vec2(0, (buttonHeight + padding * 2) * i),
                        buttonWidth + padding * 2, buttonHeight + padding * 2))
            {
                alpha = 0.9f;
                color = glm::vec3(0.3f);
                isMouseClickHandled = true;
                if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                {
                    editMode = EditMode(i);
                }
            }
            renderer->push(QuadRenderable(g_resources.getTexture("white"),
                        offset + glm::vec2(0, (buttonHeight + padding * 2) * i),
                        buttonWidth + padding * 2, buttonHeight + padding * 2,
                        color, alpha));
            u32 iconSize = buttonHeight;
            renderer->push(TextRenderable(font, modeNames[i],
                offset + glm::vec2(padding * 2 + iconSize,
                    ((padding * 2 + buttonHeight) - textHeight) / 2 + (buttonHeight + padding * 2) * i),
                glm::vec3(1), 1.f, 1.f));
            renderer->push(QuadRenderable(g_resources.getTexture(icons[i]),
                        offset + glm::vec2(padding, padding + (buttonHeight + padding * 2) * i),
                        iconSize, iconSize));
        }
        modeSelectionMaxY = offset.y + (buttonHeight + padding * 2) * (f32)EditMode::MAX;
    }

    scene->terrain->setBrushSettings(0.f, 0.f, 0.f, {});
    if (editMode == EditMode::TERRAIN)
    {
        const f32 step = 0.01f;
        glm::vec3 p = cam.position;
        while (p.z > scene->terrain->getZ(glm::vec2(p)))
        {
            p += rayDir * step;
        }

        if (g_input.isKeyPressed(KEY_SPACE))
        {
            terrainTool = TerrainTool(((u32)terrainTool + 1) % (u32)TerrainTool::MAX);
        }

        const char* toolNames[] = { "Raise / Lower", "Perturb", "Flatten", "Smooth" };
        const char* icons[] = { "terrain_icon", "terrain_icon", "terrain_icon", "terrain_icon" };
        glm::vec2 offset(height * 0.012f, modeSelectionMaxY + height * 0.02f);
        u32 padding = height * 0.01f;
        u32 buttonHeight = height * 0.02f;
        f32 textHeight = fontSmall->getHeight();
        for (u32 i=0; i<(u32)TerrainTool::MAX; ++i)
        {
            u32 buttonWidth = height * 0.12f;
            f32 alpha = 0.75f;
            glm::vec3 color(0.f);
            if (i == (u32)terrainTool)
            {
                buttonWidth = height * 0.15f;
                alpha = 0.9f;
                color = glm::vec3(0.3f);
            }
            if (pointInRectangle(g_input.getMousePosition(),
                        offset + glm::vec2(0, (buttonHeight + padding * 2) * i),
                        buttonWidth + padding * 2, buttonHeight + padding * 2))
            {
                alpha = 0.9f;
                color = glm::vec3(0.3f);
                isMouseClickHandled = true;
                if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                {
                    terrainTool = TerrainTool(i);
                }
            }
            renderer->push(QuadRenderable(g_resources.getTexture("white"),
                        offset + glm::vec2(0, (buttonHeight + padding * 2) * i),
                        buttonWidth + padding * 2, buttonHeight + padding * 2,
                        color, alpha));
            u32 iconSize = buttonHeight;
            renderer->push(TextRenderable(fontSmall, toolNames[i],
                offset + glm::vec2(padding * 2 + iconSize,
                    ((padding * 2 + buttonHeight) - textHeight) / 2 + (buttonHeight + padding * 2) * i),
                glm::vec3(1), 1.f, 1.f));
            renderer->push(QuadRenderable(g_resources.getTexture(icons[i]),
                        offset + glm::vec2(padding, padding + (buttonHeight + padding * 2) * i),
                        iconSize, iconSize));
        }

        if (!isMouseClickHandled)
        {
            scene->terrain->setBrushSettings(brushRadius, brushFalloff, brushStrength, p);
            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                brushStartZ = scene->terrain->getZ(glm::vec2(p));
            }
            if (g_input.isMouseButtonDown(MOUSE_LEFT))
            {
                switch (terrainTool)
                {
                case TerrainTool::RAISE:
                    scene->terrain->raise(glm::vec2(p), brushRadius, brushFalloff, brushStrength * deltaTime);
                    break;
                case TerrainTool::PERTURB:
                    scene->terrain->perturb(glm::vec2(p), brushRadius, brushFalloff, brushStrength * deltaTime);
                    break;
                case TerrainTool::FLATTEN:
                    scene->terrain->flatten(glm::vec2(p), brushRadius, brushFalloff, brushStrength * deltaTime, brushStartZ);
                    break;
                case TerrainTool::SMOOTH:
                    scene->terrain->smooth(glm::vec2(p), brushRadius, brushFalloff, brushStrength * deltaTime);
                    break;
                default:
                    break;
                }
            }
        }
    }
    else if (editMode == EditMode::TRACK)
    {
        scene->track->trackModeUpdate(renderer, scene, deltaTime, isMouseClickHandled);
    }
}
