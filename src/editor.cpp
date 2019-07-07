#include "editor.h"
#include "game.h"
#include "input.h"
#include "renderer.h"
#include "scene.h"
#include "input.h"
#include "terrain.h"

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

    if (g_input.getMouseScroll() != 0) {
        zoomSpeed = g_input.getMouseScroll() * 1.25f;
    }
    cameraDistance = clamp(cameraDistance - zoomSpeed, 10.f, 180.f);
    zoomSpeed = smoothMove(zoomSpeed, 0.f, 10.f, deltaTime);

    glm::vec3 cameraFrom = cameraTarget + glm::normalize(glm::vec3(lengthdir(cameraAngle, 1.f), 1.25f)) * cameraDistance;
    renderer->setViewportCamera(0, cameraFrom, cameraTarget,  32, 400.f);

    //g_game.renderer.drawMesh(*g_resources.getMesh("world.Terrain"), glm::mat4(1.f), g_resources.getMaterial("grass"));

    if (g_input.isMouseButtonDown(MOUSE_LEFT)) {
        glm::vec2 mousePos = g_input.getMousePosition();
        mousePos.y = g_game.windowHeight - mousePos.y;
        Camera const& cam = renderer->getCamera(0);
        glm::vec3 rayDir = screenToWorldRay(mousePos,
                glm::vec2(g_game.windowWidth, g_game.windowHeight), cam.view, cam.projection);

        const f32 step = 0.1f;
        glm::vec3 p = cam.position;// + step * 10.f;
        /*
        while (p.z > terrain.getZ(glm::vec2(p)))
        {
            p += rayDir * step;
        }
        renderer->drawMesh(g_resources.getMesh("world.Arrow"), glm::translate(glm::mat4(1.f), p), g_resources.getMaterial("concrete"));

        terrain.raise(glm::vec2(p), brushRadius, brushFalloff, brushStrength * deltaTime);
        */
    }
}
