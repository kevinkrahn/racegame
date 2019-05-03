#include "editor.h"
#include "game.h"
#include "input.h"

void Editor::onUpdate(f32 deltaTime)
{
    game.renderer.setViewportCount(1);
    game.renderer.addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));

    f32 right = (f32)game.input.isKeyDown(KEY_D) - (f32)game.input.isKeyDown(KEY_A);
    f32 up = (f32)game.input.isKeyDown(KEY_S) - (f32)game.input.isKeyDown(KEY_W);
    glm::vec3 moveDir = (right != 0.f || up != 0.f) ? glm::normalize(glm::vec3(right, up, 0.f)) : glm::vec3(0, 0, 0);
    glm::vec3 forward(lengthdir(cameraAngle, 1.f), 0.f);
    glm::vec3 sideways(lengthdir(cameraAngle + M_PI / 2.f, 1.f), 0.f);

    cameraVelocity += (((forward * moveDir.y) + (sideways * moveDir.x)) * (deltaTime * 240.f));
    cameraTarget += cameraVelocity * deltaTime;
    cameraVelocity = smoothMove(cameraVelocity, glm::vec3(0, 0, 0), 7.f, deltaTime);

    if (game.input.isMouseButtonPressed(MOUSE_RIGHT))
    {
        lastMousePosition = game.input.getMousePosition();
    }
    else if (game.input.isMouseButtonDown(MOUSE_RIGHT))
    {
        cameraRotateSpeed = (((lastMousePosition.x) - game.input.getMousePosition().x) / game.windowWidth * 2.f) * (1.f / deltaTime);
        lastMousePosition = game.input.getMousePosition();
    }

    cameraAngle += cameraRotateSpeed * deltaTime;
    cameraRotateSpeed = smoothMove(cameraRotateSpeed, 0, 8.f, deltaTime);

    if (game.input.getMouseScroll() != 0) {
        zoomSpeed = game.input.getMouseScroll() * 1.f;
    }
    cameraDistance = clamp(cameraDistance - zoomSpeed, 10.f, 120.f);
    zoomSpeed = smoothMove(zoomSpeed, 0.f, 10.f, deltaTime);

    glm::vec3 cameraFrom = cameraTarget + glm::normalize(glm::vec3(lengthdir(cameraAngle, 1.f), 1.25f)) * cameraDistance;
    game.renderer.setViewportCamera(0, cameraFrom, cameraTarget,  32, 400.f);

    //game.renderer.drawMesh(*game.resources.getMesh("world.Terrain"), glm::mat4(1.f), game.resources.getMaterial("grass"));

    if (game.input.isMouseButtonDown(MOUSE_LEFT)) {
        glm::vec2 mousePos = game.input.getMousePosition();
        mousePos.y = game.windowHeight - mousePos.y;
        Camera const& cam = game.renderer.getCamera(0);
        glm::vec3 rayDir = screenToWorldRay(mousePos,
                glm::vec2(game.windowWidth, game.windowHeight), cam.view, cam.projection);

        const f32 step = 0.1f;
        glm::vec3 p = cam.position;// + step * 10.f;
        while (p.z > terrain.getZ(glm::vec2(p)))
        {
            p += rayDir * step;
        }
        game.renderer.drawMesh(*game.resources.getMesh("world.Arrow"), glm::translate(glm::mat4(1.f), p), game.resources.getMaterial("concrete"));

        terrain.raise(glm::vec2(p), brushRadius, brushFalloff, brushStrength * deltaTime);
    }

    terrain.render();
}
