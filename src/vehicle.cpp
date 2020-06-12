#include "vehicle.h"
#include "game.h"
#include "decal.h"
#include "scene.h"
#include "renderer.h"
#include "2d.h"
#include "input.h"
#include "mesh_renderables.h"
#include "billboard.h"
#include "weapon.h"
#include "imgui.h"

// TODO: play with this value to find best distance
const f32 camDistance = 80.f;

Vehicle::Vehicle(Scene* scene, glm::mat4 const& transform, glm::vec3 const& startOffset,
        Driver* driver, VehicleTuning&& tuning, u32 vehicleIndex, i32 cameraIndex)
{
    this->cameraTarget = translationOf(transform) - camDistance * 0.5f;
    this->cameraFrom = cameraTarget;
    this->startTransform = transform;
    this->targetOffset = startOffset;
    this->startOffset = startOffset;
    this->lastDamagedBy = vehicleIndex;
    this->vehicleIndex = vehicleIndex;
    this->offsetChangeInterval = random(scene->randomSeries, 6.f, 15.f);
	if (driver->aiIndex != -1)
	{
		auto& ai = g_ais[driver->aiIndex];
		f32 skill = clamp(1.f - ai.drivingSkill
				+ random(scene->randomSeries, -0.2f, 0.2f), 0.f, 0.99999f);
		this->preferredFollowPathIndex = (u32)(glm::max(0, (i32)scene->getPaths().size()) * skill);
		this->currentFollowPathIndex = this->preferredFollowPathIndex;
		// TODO: change preferredFollowPathIndex per lap?
	}
    this->driver = driver;
    this->scene = scene;
    this->lastValidPosition = translationOf(transform);
    this->cameraIndex = cameraIndex;
    this->tuning = std::move(tuning);
    this->hitPoints = this->tuning.maxHitPoints;
    this->previousTargetPosition = translationOf(transform);
    this->rearWeaponTimer = 0.f + vehicleIndex * 0.2f;
    this->placement = vehicleIndex;

    engineSound = g_audio.playSound3D(g_res.getSound("engine2"),
            SoundType::VEHICLE, translationOf(transform), true);
    tireSound = g_audio.playSound3D(g_res.getSound("tires"),
            SoundType::VEHICLE, translationOf(transform), true, 1.f, 0.f);

    actorUserData.entityType = ActorUserData::VEHICLE;
    actorUserData.vehicle = this;
    vehiclePhysics.setup(&actorUserData, scene->getPhysicsScene(), transform, &this->tuning);

    // create weapons
    for (u32 i=0; i<ARRAY_SIZE(VehicleConfiguration::frontWeaponIndices); ++i)
    {
        VehicleConfiguration* config = driver->getVehicleConfig();
        if (config->frontWeaponIndices[i] != -1)
        {
            frontWeapons.push_back(g_weapons[config->frontWeaponIndices[i]].create());
            frontWeapons.back()->upgradeLevel = config->frontWeaponUpgradeLevel[i];
            frontWeapons.back()->mountTransform = driver->getVehicleData()->weaponMounts[i];
        }
    }
    for (u32 i=0; i<ARRAY_SIZE(VehicleConfiguration::rearWeaponIndices); ++i)
    {
        VehicleConfiguration* config = driver->getVehicleConfig();
        if (config->rearWeaponIndices[i] != -1)
        {
            rearWeapons.push_back(g_weapons[config->rearWeaponIndices[i]].create());
            rearWeapons.back()->upgradeLevel = config->rearWeaponUpgradeLevel[i];
        }
    }
    if (driver->getVehicleConfig()->specialAbilityIndex != -1)
    {
        specialAbility = g_weapons[driver->getVehicleConfig()->specialAbilityIndex].create();
    }
}

Vehicle::~Vehicle()
{
    if (engineSound) g_audio.stopSound(engineSound);
    if (tireSound) g_audio.stopSound(tireSound);
    for (auto& d : vehicleDebris)
    {
        d.rigidBody->release();
    }
}

void Vehicle::resetAmmo()
{
    for(auto& w : frontWeapons)
    {
        w->refillAmmo();
    }
    for(auto& w : rearWeapons)
    {
        w->refillAmmo();
    }
    if (specialAbility)
    {
        specialAbility->refillAmmo();
    }
}

void Vehicle::reset(glm::mat4 const& transform)
{
    airTime = 0.f;
    airBonusGracePeriod = 0.f;
    savedAirTime = 0.f;
    for (auto& w : frontWeapons)
    {
        w->reset();
    }
    for (auto& w : rearWeapons)
    {
        w->reset();
    }
    if (specialAbility)
    {
        specialAbility->reset();
    }
    vehiclePhysics.reset(transform);
}

void Vehicle::drawWeaponAmmo(Renderer* renderer, glm::vec2 pos, Weapon* weapon,
        bool showAmmo, bool selected)
{
    f32 iconSize = glm::floor(g_game.windowHeight * 0.05f);
    if (showAmmo)
    {
        Texture* iconbg = g_res.getTexture("weapon_iconbg");
        renderer->push2D(QuadRenderable(iconbg, pos, iconSize * 1.5f, iconSize));
    }
    else
    {
        Texture* iconbg = g_res.getTexture("iconbg");
        renderer->push2D(QuadRenderable(iconbg, pos, iconSize, iconSize));
    }

    renderer->push2D(QuadRenderable(weapon->info.icon,
                pos, iconSize, iconSize));
    if (selected)
    {
        Texture* selectedTex = g_res.getTexture("weapon_iconbg_selected");
        renderer->push2D(QuadRenderable(selectedTex, pos, iconSize * 1.5f, iconSize));
    }
    if (showAmmo)
    {
        u32 ammoTickCountMax = weapon->getMaxAmmo() / weapon->ammoUnitCount;
        u32 ammoTickCount = (weapon->ammo + weapon->ammoUnitCount - 1) / weapon->ammoUnitCount;
        f32 ammoTickMargin = iconSize * 0.025f;
        f32 ammoTickHeight = (f32)(iconSize - iconSize * 0.2f) / (f32)ammoTickCountMax;
        Texture* ammoTickTex = g_res.getTexture("ammotick");
        for (u32 i=0; i<ammoTickCount; ++i)
        {
            renderer->push2D(QuadRenderable(ammoTickTex,
                        pos + glm::vec2(iconSize + ammoTickMargin * 2.f,
                            ammoTickHeight * i + (iconSize * 0.1f) + ammoTickMargin * 0.5f),
                        iconSize * 0.32f, ammoTickHeight - ammoTickMargin));
        }
    }
}

void Vehicle::drawHUD(Renderer* renderer, f32 deltaTime)
{
    TIMED_BLOCK();

    if (cameraIndex >= 0)
    {
        Font& font1 = g_res.getFont("font_bold", (u32)(g_game.windowHeight * 0.04f));
        Font& font2 = g_res.getFont("font_bold", (u32)(g_game.windowHeight * 0.08f));
        Font& font3 = g_res.getFont("font_bold", (u32)(g_game.windowHeight * 0.05f));

        ViewportLayout const& layout =
            viewportLayout[renderer->getRenderWorld()->getViewportCount() - 1];
        glm::vec2 dim(g_game.windowWidth, g_game.windowHeight);
        glm::vec2 offset = layout.offsets[cameraIndex] * dim;
        glm::vec2 d(1.f, 1.f);
        if (offset.y > 0.f)
        {
            offset.y = g_game.windowHeight - font2.getHeight();
            d.y = -1;
        }
        glm::vec2 vdim = dim * layout.scale;
        glm::vec2 voffset = layout.offsets[cameraIndex] * dim;
        if (voffset.y > 0.f)
        {
            voffset.y = (f32)g_game.windowHeight;
        }

        f32 o20 = (f32)g_game.windowHeight * 0.02f;
        f32 o25 = (f32)g_game.windowHeight * 0.03f;
        f32 o200 = (f32)g_game.windowHeight * 0.21f;

        char* p = tstr(glm::min((u32)currentLap, scene->getTotalLaps()));
        const char* lapStr = "LAP";
        f32 lapWidth = font1.stringDimensions(lapStr).x;
        renderer->push2D(TextRenderable(&font1, lapStr,
                    offset + glm::vec2(o20, d.y*o20), glm::vec3(1.f)));
        renderer->push2D(TextRenderable(&font2, p,
                    offset + glm::vec2(o25 + lapWidth, d.y*o20), glm::vec3(1.f)));
        renderer->push2D(TextRenderable(&font1, tstr('/', scene->getTotalLaps()),
                    offset + glm::vec2(o25 + lapWidth + font2.stringDimensions(p).x, d.y*o20), glm::vec3(1.f)));

        const char* placementSuffix = "th";
        if (placement == 0) placementSuffix = "st";
        else if (placement == 1) placementSuffix = "nd";
        else if (placement == 2) placementSuffix = "rd";

        glm::vec3 col = glm::mix(glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), placement / 8.f);

        p = tstr(placement + 1);
        renderer->push2D(TextRenderable(&font2, p, offset + glm::vec2(o200, d.y*o20), col));
        renderer->push2D(TextRenderable(&font1, placementSuffix,
                    offset + glm::vec2(o200 + font2.stringDimensions(p).x, d.y*o20), col));

        // weapons
        f32 weaponIconX = g_game.windowHeight * 0.35f;
        for (i32 i=0; i<(i32)frontWeapons.size(); ++i)
        {
            auto& w = frontWeapons[i];
            drawWeaponAmmo(renderer, offset +
                    glm::vec2(weaponIconX, d.y * g_game.windowHeight * 0.018f),
                    w.get(), true, i == currentFrontWeaponIndex && frontWeapons[i]->ammo > 0);
            weaponIconX += g_game.windowHeight * 0.1f;
        }
        for (i32 i=0; i<(i32)rearWeapons.size(); ++i)
        {
            auto& w = rearWeapons[i];
            drawWeaponAmmo(renderer, offset +
                    glm::vec2(weaponIconX, d.y * g_game.windowHeight * 0.018f),
                    w.get(), true, i == currentRearWeaponIndex && rearWeapons[i]->ammo > 0);
            weaponIconX += g_game.windowHeight * 0.1f;
        }
        if (specialAbility)
        {
            drawWeaponAmmo(renderer, offset +
                    glm::vec2(weaponIconX, d.y * g_game.windowHeight * 0.018f),
                    specialAbility.get(), false, false);
        }

        // healthbar
        Texture* white = &g_res.white;
        const f32 healthPercentage = glm::clamp(hitPoints / tuning.maxHitPoints, 0.f, 1.f);
        const f32 maxHealthbarWidth = g_game.windowHeight * 0.14f;
        const f32 healthbarWidth = maxHealthbarWidth * healthPercentage;
        const f32 healthbarHeight = g_game.windowHeight * 0.009f;
        glm::vec2 pos = voffset + glm::vec2(vdim.x - o20, d.y*o20);
        renderer->push2D(QuadRenderable(white, pos + glm::vec2(-maxHealthbarWidth, healthbarHeight*d.y),
                pos, {}, {}, glm::vec3(0)));
        renderer->push2D(QuadRenderable(white, pos + glm::vec2(-healthbarWidth, healthbarHeight*d.y),
                pos, {}, {}, glm::mix(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), healthPercentage)));

        // display notifications
        u32 count = 0;
        for (auto it = notifications.begin(); it != notifications.end();)
        {
            glm::vec2 p = glm::floor(layout.offsets[cameraIndex] * dim + layout.scale * dim * 0.5f -
                    glm::vec2(0, layout.scale.y * dim.y * 0.3f) + glm::vec2(0, count * dim.y * 0.04f));
            renderer->push2D(TextRenderable(&font3, it->text, p,
                it->color, (glm::sin(it->time * 6.f) + 1.f) * 0.3f + 0.5f,
                glm::max(1.7f - it->time * 3.f, 1.f),
                HorizontalAlign::CENTER, VerticalAlign::CENTER));
            ++count;
            it->time += deltaTime;
            if (it->time > it->secondsToKeepOnScreen)
            {
                notifications.erase(it);
                continue;
            }
            ++it;
        }
    }
}

void Vehicle::onRender(RenderWorld* rw, f32 deltaTime)
{
    TIMED_BLOCK();

    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        scene->ribbons.addChunk(&tireMarkRibbons[i]);
    }

    glm::mat4 wheelTransforms[NUM_WHEELS];
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        wheelTransforms[i] = vehiclePhysics.wheelInfo[i].transform;
    }
    glm::mat4 transform = vehiclePhysics.getTransform();
    driver->getVehicleData()->render(rw, transform,
            wheelTransforms, *driver->getVehicleConfig(), this, isBraking, cameraIndex >= 0,
            shieldColor);
    driver->getVehicleData()->renderDebris(rw, vehicleDebris,
            *driver->getVehicleConfig());

    // visualize path finding
#if 0
    if (motionPath.size() > 0)
    {
        Mesh* sphere = g_res.getModel("misc")->getMeshByName("world.Sphere");
        //Font* font = &g_res.getFont("font", 18);
        for (auto& p : motionPath)
        {
            rw->push(LitRenderable(sphere,
                        glm::translate(glm::mat4(1.f), p.p + glm::vec3(0, 0, 1))
                            * glm::scale(glm::mat4(1.f), glm::vec3(0.25f)),
                        nullptr, glm::vec3(1, 1, 0)));

            /*
            glm::vec4 tp = g_game.renderer->getRenderWorld()->getCamera(0).viewProjection *
                glm::vec4(p.p + glm::vec3(0, 0, 1), 1.f);
            tp.x = (((tp.x / tp.w) + 1.f) / 2.f) * g_game.windowWidth;
            tp.y = ((-1.f * (tp.y / tp.w) + 1.f) / 2.f) * g_game.windowHeight;
            g_game.renderer->push2D(TextRenderable(font,
                tstr(std::fixed, std::setprecision(1), p.g, ", ", p.h), { tp.x, tp.y },
                glm::vec3(0.f, 0.f, 1.f), 1.f, 1.f, HorizontalAlign::CENTER));
            */
        }
    }
#endif
}

void Vehicle::updateCamera(RenderWorld* rw, f32 deltaTime)
{
    glm::vec3 pos = lastValidPosition;
    pos.z = glm::max(pos.z, -10.f);

#if 0
    cameraTarget = pos + glm::vec3(0, 0, 2.f);
    cameraFrom = smoothMove(cameraFrom,
            cameraTarget - getForwardVector() * 10.f + glm::vec3(0, 0, 3.f), 8.f, deltaTime);
    rw->setViewportCamera(cameraIndex, cameraFrom, cameraTarget, 4.f, 200.f, 60.f);
#else
    glm::vec3 forwardVector = vehiclePhysics.getForwardVector();
    f32 forwardSpeed = vehiclePhysics.getForwardSpeed();
    cameraTarget = smoothMove(cameraTarget,
            pos + glm::vec3(glm::normalize(glm::vec2(forwardVector)), 0.f) * forwardSpeed * 0.3f,
            5.f, deltaTime);
    cameraTarget += screenShakeOffset * (screenShakeTimer * 0.5f);
    glm::vec3 cameraDir = glm::normalize(glm::vec3(1.f, 1.f, 1.25f));
    cameraFrom = cameraTarget + cameraDir * camDistance;
    rw->setViewportCamera(cameraIndex, cameraFrom, cameraTarget, 18.f, 250.f);
    if (cameraIndex >= 0)
    {
        rw->setHighlightColor(cameraIndex,
                glm::vec4(driver->getVehicleConfig()->color, 1.f));

        glm::mat4 m = glm::mat4(1.f);
        m[0] = glm::vec4(-cameraDir, m[0].w);
        m[1] = glm::vec4(glm::normalize(
                    glm::cross(glm::vec3(0, 0, 1), glm::vec3(m[0]))), m[1].w);
        m[2] = glm::vec4(glm::normalize(
                    glm::cross(glm::vec3(m[0]), glm::vec3(m[1]))), m[2].w);

        motionBlurStrength = smoothMove(motionBlurStrength, targetMotionBlurStrength, 2.f, deltaTime);

        glm::vec3 vel = m * glm::vec4(convert(getRigidBody()->getLinearVelocity()), 1.f);
        glm::vec2 motionBlur = glm::vec2(vel) * motionBlurStrength * 0.01f;
        rw->setMotionBlur(cameraIndex, motionBlur);

        motionBlurResetTimer = glm::max(motionBlurResetTimer - deltaTime, 0.f);
        if (motionBlurResetTimer <= 0.f)
        {
            targetMotionBlurStrength = 0.f;
        }
    }
#endif
}

void Vehicle::onUpdate(RenderWorld* rw, f32 deltaTime)
{
    TIMED_BLOCK();

    bool isPlayerControlled = driver->isPlayer;

    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        tireMarkRibbons[i].update(deltaTime);
    }

    // update debris chunks
    for (u32 i=0; i<(u32)vehicleDebris.size();)
    {
        VehicleDebris& debris = vehicleDebris[i];
        debris.life -= deltaTime;
        if (debris.life <= 0.f)
        {
            scene->getPhysicsScene()->removeActor(*debris.rigidBody);
            debris.rigidBody->release();
            if (vehicleDebris.size() > 1)
            {
                debris = vehicleDebris.back();
            }
			vehicleDebris.pop_back();
        }
        else
        {
            ++i;
#if 0
            if (debris.life > 4.f)
            {
                if ((u32)(debris.life * 1000) % 2 == 0)
                {
                    glm::vec3 vel = convert(debris.rigidBody->getLinearVelocity());
                    glm::vec3 pos = convert(debris.rigidBody->getGlobalPose().p);
                    scene->smoke.spawn(pos, vel * 0.5f, glm::min(debris.life - 4.f, 1.f),
                            glm::vec4(glm::vec3(0.06f), 1.f), random(scene->randomSeries, 0.5f, 1.f));
                }
            }
#endif
        }
    }

    if (screenShakeTimer > 0.f)
    {
        screenShakeTimer = glm::max(screenShakeTimer - deltaTime, 0.f);
        screenShakeDirChangeTimer -= screenShakeDirChangeTimer;
        if (screenShakeDirChangeTimer <= 0.f)
        {
            screenShakeDirChangeTimer = random(scene->randomSeries, 0.01f, 0.025f);
            glm::vec3 targetOffset = glm::normalize(glm::vec3(
                random(scene->randomSeries, -1.f, 1.f),
                random(scene->randomSeries, -1.f, 1.f),
                random(scene->randomSeries, -1.f, 1.f))) * random(scene->randomSeries, 0.75f, 1.5f);
            screenShakeVelocity = glm::normalize(targetOffset - screenShakeOffset) *
                random(scene->randomSeries, 20.f, 30.f);
        }
        screenShakeOffset += screenShakeVelocity * deltaTime;
    }

    if (deadTimer > 0.f)
    {
        deadTimer -= deltaTime;
        if (engineSound)
        {
            g_audio.setSoundVolume(engineSound, 0.f);
        }
        if (tireSound)
        {
            g_audio.setSoundVolume(tireSound, 0.f);
        }
        if (deadTimer <= 0.f)
        {
            backupTimer = 0.f;
            deadTimer = 0.f;
            hitPoints = tuning.maxHitPoints;

            glm::vec3 pos = graphResult.position;
            const TrackGraph::Node* node = graphResult.lastNode;
            if (!graphResult.lastNode)
            {
                node = scene->getTrackGraph().getEndNode();
            }
            pos += glm::vec3(random(scene->randomSeries, -5.f, 5.f),
                    random(scene->randomSeries, -5.f, 5.f), 0.f);
            pos = scene->findValidPosition(pos, 5.f);

            glm::mat4 resetTransform = glm::translate(glm::mat4(1.f), pos + glm::vec3(0, 0, 7.f)) *
                  glm::rotate(glm::mat4(1.f), node->angle, glm::vec3(0, 0, 1));

            PxRaycastBuffer hit;
            if (scene->raycastStatic(pos + glm::vec3(0, 0, 8.f), { 0, 0, -1 }, 20.f, &hit))
            {
                glm::vec3 up = convert(hit.block.normal);
                glm::vec3 forwardTmp = glm::vec3(lengthdir(node->angle, 1.f), 0.f);
                glm::vec3 right = glm::normalize(glm::cross(up, forwardTmp));
                glm::vec3 forward = glm::normalize(glm::cross(right, up));
                glm::mat4 m(1.f);
                m[0] = glm::vec4(forward, m[0].w);
                m[1] = glm::vec4(right, m[1].w);
                m[2] = glm::vec4(up, m[2].w);
                resetTransform = glm::translate(glm::mat4(1.f), pos + up * 7.f) * m;
            }

            reset(resetTransform);

            if (!finishedRace)
            {
                const f32 respawnSpeed = 11.f;
                getRigidBody()->addForce(
                        convert(vehiclePhysics.getForwardVector() * respawnSpeed),
                        PxForceMode::eVELOCITY_CHANGE);
            }
        }
        if (cameraIndex >= 0)
        {
            updateCamera(rw, deltaTime);
        }

        return;
    }

    //rw->addPointLight(getPosition() + getForwardVector() * 3.f, glm::vec3(1.f, 0.9f, 0.8f), 5.f, 2.f);

    input = {};
    if (isPlayerControlled)
    {
        updatePlayerInput(deltaTime, rw);
    }
    else if (scene->getPaths().size() > 0)
    {
        updateAiInput(deltaTime, rw);
    }

    shieldColor = glm::vec3(0, 0, 0);

    // update weapons
    for (i32 i=0; i<(i32)frontWeapons.size(); ++i)
    {
        auto& w = frontWeapons[i];
        w->update(scene, this, currentFrontWeaponIndex == i ? input.beginShoot : false,
                currentFrontWeaponIndex == i ? input.holdShoot : false, deltaTime);
        if (w->ammo == 0 && currentFrontWeaponIndex == i)
        {
            input.switchFrontWeapon = true;
        }
    }
    for (i32 i=0; i<(i32)rearWeapons.size(); ++i)
    {
        auto& w = rearWeapons[i];
        w->update(scene, this, currentRearWeaponIndex == i ? input.beginShootRear : false,
                currentRearWeaponIndex == i ? input.holdShootRear : false, deltaTime);
        if (w->ammo == 0 && currentRearWeaponIndex == i)
        {
            input.switchRearWeapon = true;
        }
    }

    if (input.switchFrontWeapon)
    {
        currentFrontWeaponIndex = (currentFrontWeaponIndex + 1) % frontWeapons.size();
    }
    if (input.switchRearWeapon)
    {
        currentRearWeaponIndex = (currentRearWeaponIndex + 1) % rearWeapons.size();
    }

    if (specialAbility)
    {
        specialAbility->update(scene, this, false, false, deltaTime);
    }

    // update vehicle physics
    if (!finishedRace)
    {
        vehiclePhysics.update(scene->getPhysicsScene(), deltaTime,
                input.digital, input.accel, input.brake, input.steer, false, scene->canGo(), false);
    }
    else
    {
        vehiclePhysics.update(scene->getPhysicsScene(), deltaTime, false, 0.f,
                controlledBrakingTimer < 0.5f ? 0.f : 0.5f, 0.f, 0.f, true, true);
        if (vehiclePhysics.getForwardSpeed() > 1.f)
        {
            controlledBrakingTimer = glm::min(controlledBrakingTimer + deltaTime, 1.f);
        }
        else
        {
            controlledBrakingTimer = glm::max(controlledBrakingTimer - deltaTime, 0.f);
        }
    }

    glm::vec3 currentPosition = getPosition();
    lastValidPosition = currentPosition;

    // periodically choose random offset
    offsetChangeTimer += deltaTime;
    if (offsetChangeTimer > offsetChangeInterval)
    {
        //targetOffset.x = clamp(targetOffset.x + random(scene->randomSeries, -1.f, 1.f), -4.5f, 4.5f);
        //targetOffset.y = clamp(targetOffset.y + random(scene->randomSeries, -1.f, 1.f), -4.5f, 4.5f);
        targetOffset.x = random(scene->randomSeries, -2.5f, 2.5f);
        targetOffset.y = random(scene->randomSeries, -2.5f, 2.5f);
        offsetChangeTimer = 0.f;
        offsetChangeInterval = random(scene->randomSeries, 6.f, 16.f);
    }

    const f32 maxSkippableDistance = 250.f;
    if (scene->canGo())
    {
        scene->getTrackGraph().findLapDistance(currentPosition, graphResult, maxSkippableDistance);
    }

    // check if crossed finish line
    if (!finishedRace && graphResult.lapDistanceLowMark < maxSkippableDistance)
    {
        glm::vec3 finishLinePosition = translationOf(scene->getStart());
        glm::vec3 dir = glm::normalize(currentPosition - finishLinePosition);
        if (glm::dot(xAxisOf(scene->getStart()), dir) > 0.f
                && glm::length2(currentPosition - finishLinePosition) < square(40.f))
        {
            if (!finishedRace && (u32)currentLap >= scene->getTotalLaps())
            {
                finishedRace = true;
                scene->vehicleFinish(vehicleIndex);
            }
            if (currentLap > 0)
            {
                g_audio.playSound3D(g_res.getSound("lap"), SoundType::GAME_SFX, currentPosition);
            }
            ++currentLap;
            if ((u32)currentLap == scene->getTotalLaps())
            {
                addNotification("LAST LAP!", 2.f, glm::vec3(1, 0, 0));
            }
            graphResult.lapDistanceLowMark = scene->getTrackGraph().getStartNode()->t;
            graphResult.currentLapDistance = scene->getTrackGraph().getStartNode()->t;
            resetAmmo();
        }
    }

    // check for lapping bonus
    if (!finishedRace)
    {
        for (auto& v : scene->getVehicles())
        {
            if (v.get() == this)
            {
                continue;
            }

            f32 lapDistance = scene->getTrackGraph().getStartNode()->t;
            f32 myDistance = glm::max((i32)currentLap - 1, 0) * lapDistance
                + (lapDistance - graphResult.lapDistanceLowMark);
            f32 otherDistance = glm::max((i32)v->currentLap - 1, 0) * lapDistance
                + (lapDistance - v->graphResult.lapDistanceLowMark);

            if (myDistance - otherDistance > lapDistance + lappingOffset[v->vehicleIndex])
            {
                lappingOffset[v->vehicleIndex] += lapDistance;
                addBonus("LAPPING BONUS!", LAPPING_BONUS_AMOUNT, glm::vec3(1, 1, 0));
                // TODO: play sound
            }
        }
    }

    if (engineSound)
    {
        f32 rotationSpeed = vehiclePhysics.getAverageWheelRotationSpeed();
        f32 gearRatio = glm::abs(vehiclePhysics.getCurrentGearRatio());
        engineRPM = smoothMove(engineRPM, glm::min(rotationSpeed * gearRatio, 150.f), 1.9f, deltaTime);

        //g_audio.setSoundPitch(engineSound, 0.8f + getEngineRPM() * 0.0007f);
        g_audio.setSoundPitch(engineSound, 1.f + engineRPM * 0.04f);

        engineThrottleLevel =
            smoothMove(engineThrottleLevel, vehiclePhysics.getEngineThrottle(), 1.5f, deltaTime);
        g_audio.setSoundVolume(engineSound, clamp(engineThrottleLevel + 0.7f, 0.f, 1.f));

        g_audio.setSoundPosition(engineSound, lastValidPosition);
    }
    if (tireSound)
    {
        g_audio.setSoundPosition(tireSound, lastValidPosition);
    }

    if (cameraIndex >= 0)
    {
        updateCamera(rw, deltaTime);
    }

    // destroy vehicle if off track or out of bounds
    bool onGround = false;
    if (currentPosition.z < -32.f)
    {
        applyDamage(100.f, vehicleIndex);
    }
    else
    {
        bool validGround = false;
        PxRaycastBuffer hit;
        if (scene->raycastStatic(currentPosition, { 0, 0, -1 }, 3.0f, &hit))
        {
            onGround = true;
            PxMaterial* hitMaterial =
                hit.block.shape->getMaterialFromInternalFaceIndex(hit.block.faceIndex);
            if (hitMaterial == g_game.physx.materials.track || hitMaterial == g_game.physx.materials.offroad)
            {
                validGround = true;
            }
        }
        if (!validGround)
        {
            if (scene->raycastStatic(currentPosition, { 0, 0, -1 }, 3.0f, nullptr, COLLISION_FLAG_TRACK))
            {
                onGround = true;
                validGround = true;
            }
        }
        if (onGround && !validGround)
        {
            applyDamage(100.f, vehicleIndex);
        }
    }

    // update wheels
    smokeTimer = glm::max(0.f, smokeTimer - deltaTime);
    const f32 smokeInterval = 0.015f;
    bool smoked = false;
    u32 numWheelsOnTrack = 0;
    isOnTrack = false;
    isInAir = true;
    bool isTouchingAnyGlue = false;
    f32 maxSlip = 0.f;
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        auto& info = vehiclePhysics.wheelInfo[i];
        if (!info.isInAir)
        {
            if (info.isTouchingTrack || info.isOffroad)
            {
                ++numWheelsOnTrack;
            }

            if (info.isTouchingTrack)
            {
                isOnTrack = true;
            }

            if (info.isTouchingGlue)
            {
                isTouchingAnyGlue = true;
                if (glueSoundTimer == 0.f)
                {
                    g_audio.playSound3D(g_res.getSound("sticky"), SoundType::GAME_SFX,
                            currentPosition, false, 1.f, 0.95f);
                    glueSoundTimer = 0.5f;
                }
            }
            isInAir = false;
        }

        f32 slip = glm::max(
                glm::abs(info.lateralSlip) - 0.4f,
                glm::abs(info.longitudinalSlip) - 0.6f);
        bool wasWheelSlipping = isWheelSlipping[i];
        isWheelSlipping[i] = (slip > 0.f || info.oilCoverage > 0.f) && !info.isInAir;
        maxSlip = glm::max(maxSlip, slip);

        // create smoke
        if (slip > 0.f && info.isTouchingTrack)
        {
            if (smokeTimer == 0.f)
            {
                glm::vec3 vel(glm::normalize(glm::vec3(
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f))));
                scene->smoke.spawn(
                    info.position - glm::vec3(0, 0, 0.2f),
                    (vel + glm::vec3(0, 0, 1)) * 0.8f,
                    glm::min(1.f, slip * 0.4f));
                smoked = true;
            }
        }

        if (info.dustAmount > 0.f && smokeTimer == 0.f)
        {
            if (info.rotationSpeed > 5.f || slip > 0.f)
            {
                glm::vec3 vel(glm::normalize(glm::vec3(
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f))));
                scene->smoke.spawn(
                    info.position - glm::vec3(0, 0, 0.2f),
                    (vel + glm::vec3(0, 0, 1)) * 0.8f,
                    glm::clamp(
                        glm::max(slip, glm::abs(info.rotationSpeed * 0.022f)) * info.dustAmount,
                        0.f, 1.f),
                    glm::vec4(0.58f, 0.50f, 0.22f, 1.f));
                smoked = true;
            }
        }

        // add tire marks
        if (isWheelSlipping[i] && info.isTouchingTrack)
        {
            f32 wheelWidth = i < 2 ? tuning.wheelWidthFront : tuning.wheelWidthRear;
            f32 alpha = glm::clamp(glm::max(slip * 3.f, info.oilCoverage), 0.f, 1.f);
            glm::vec3 color(1.f - glm::clamp(info.oilCoverage, 0.f, 1.f));
            tireMarkRibbons[i].addPoint(info.contactPosition, info.contactNormal, wheelWidth / 2,
                    glm::vec4(color, alpha));
        }
        else if (wasWheelSlipping)
        {
            tireMarkRibbons[i].capWithLastPoint();
        }
    }
    if (!isTouchingAnyGlue)
    {
        glueSoundTimer = glm::max(glueSoundTimer - deltaTime, 0.f);
    }

    g_audio.setSoundVolume(tireSound,
            isOnTrack ? glm::min(1.f, glm::min(maxSlip * 1.2f,
                    getRigidBody()->getLinearVelocity().magnitude() * 0.1f)) : 0.f);

    if (smoked)
    {
        smokeTimer = smokeInterval;
    }

    // destroy vehicle if it is flipped and unable to move
    if (onGround && numWheelsOnTrack <= 2
            && getRigidBody()->getLinearVelocity().magnitude() < 6.f)
    {
        flipTimer += deltaTime;
        if (flipTimer > 1.5f)
        {
            applyDamage(100.f, vehicleIndex);
        }
    }
    else
    {
        flipTimer = 0.f;
    }

    // explode
    if (hitPoints <= 0.f)
    {
        blowUp();
    }

    // spawn smoke when critically damaged
    smokeTimerDamage = glm::max(0.f, smokeTimerDamage - deltaTime);
    f32 damagePercent = hitPoints / tuning.maxHitPoints;
    if (smokeTimerDamage <= 0.f && damagePercent < 0.3f)
    {
        // TODO: make the effect more intense the more critical the damage (fire and sparks?)
        glm::vec3 vehicleVel = previousVelocity + vehiclePhysics.getForwardVector();
        glm::vec3 vel = glm::vec3(glm::normalize(glm::vec3(
                random(scene->randomSeries, -1.f, 1.f),
                random(scene->randomSeries, -1.f, 1.f),
                random(scene->randomSeries, -1.f, 1.f))))
            + glm::vec3(0, 0, 1)
            + vehicleVel * 0.5f;
        scene->smoke.spawn(currentPosition + glm::vec3(0, 0, 0.5f),
            vel, 1.f - damagePercent, glm::vec4(glm::vec3(0.5f), 1.f), 0.5f);
        smokeTimerDamage = 0.015f;
    }
    if (g_input.isKeyPressed(KEY_R) && isPlayerControlled)
    {
        applyDamage(15, vehicleIndex);
    }

    // give air bonus if in air long enough
    if (!onGround)
    {
        airTime += deltaTime;
        airBonusGracePeriod = 0.f;
    }
    else
    {
        if (airTime > 2.f)
        {
            savedAirTime = airTime;
        }
        airTime = 0.f;
        airBonusGracePeriod += deltaTime;
        if (savedAirTime > 0.f && airBonusGracePeriod > 0.4f && totalAirBonuses < 10)
        {
            if (savedAirTime > 4.f)
            {
                addBonus("BIG AIR BONUS!", BIG_AIR_BONUS_AMOUNT, glm::vec3(1, 0.6f, 0.05f));
                ++totalAirBonuses;
            }
            else if (savedAirTime > 2.f)
            {
                addBonus("AIR BONUS!", AIR_BONUS_AMOUNT, glm::vec3(1, 0.7f, 0.05f));
                ++totalAirBonuses;
            }
            airBonusGracePeriod = 0.f;
            savedAirTime = 0.f;
        }
    }

    previousVelocity = convert(getRigidBody()->getLinearVelocity());
}

void Vehicle::shakeScreen(f32 intensity)
{
    if (intensity * 0.08f > screenShakeTimer)
    {
        screenShakeTimer = intensity * 0.08f;
        screenShakeOffset = glm::vec3(0.f);
    }
}

void Vehicle::applyDamage(f32 amount, u32 instigator)
{
    if (specialAbility)
    {
        amount = specialAbility->onDamage(amount, this);
    }
    if (amount > 0.f)
    {
        hitPoints -= amount;
        lastDamagedBy = instigator;
        if (instigator != vehicleIndex)
        {
            lastOpponentDamagedBy = instigator;
            lastTimeDamagedByOpponent = scene->getWorldTime();
        }
        if (smokeTimerDamage <= 0.f)
        {
            smokeTimerDamage = 0.015f;
        }
    }
}

void Vehicle::blowUp()
{
    glm::mat4 transform = vehiclePhysics.getTransform();
    for (auto& d : g_vehicles[driver->vehicleIndex]->debrisChunks)
    {
        PxRigidDynamic* body = g_game.physx.physics->createRigidDynamic(
                PxTransform(convert(transform * d.transform)));
        body->attachShape(*d.collisionShape);
        PxRigidBodyExt::updateMassAndInertia(*body, 10.f);
        scene->getPhysicsScene()->addActor(*body);
        body->setLinearVelocity(
                convert(previousVelocity) +
                convert(glm::vec3(glm::normalize(rotationOf(transform)
                            * glm::vec4(translationOf(d.transform), 1.f)))
                    * random(scene->randomSeries, 3.f, 14.f) + glm::vec3(0, 0, 9.f)));
        body->setAngularVelocity(PxVec3(
                    random(scene->randomSeries, 0.f, 9.f),
                    random(scene->randomSeries, 0.f, 9.f),
                    random(scene->randomSeries, 0.f, 9.f)));

        createVehicleDebris(VehicleDebris{
            &d,
            body,
            random(scene->randomSeries, 6.f, 7.f)
        });
    }
    deadTimer = 0.8f;
    scene->createExplosion(translationOf(transform), previousVelocity, 10.f);
    if (scene->getWorldTime() - lastTimeDamagedByOpponent < 0.5)
    {
        scene->attackCredit(lastOpponentDamagedBy, vehicleIndex);
    }
    else
    {
        scene->attackCredit(lastDamagedBy, vehicleIndex);
    }
    const char* sounds[] = {
        "explosion4",
        "explosion5",
        "explosion6",
        "explosion7",
    };
    u32 index = irandom(scene->randomSeries, 0, ARRAY_SIZE(sounds));
    g_audio.playSound3D(g_res.getSound(sounds[index]), SoundType::GAME_SFX, getPosition(), false, 1.f, 0.95f);
    reset(glm::translate(glm::mat4(1.f), { 0, 0, 1000 }));
    if (raceStatistics.destroyed + raceStatistics.accidents == 10)
    {
        addBonus("VICTIM BONUS", VICTIM_BONUS_AMOUNT, glm::vec3(1.f, 0.3f, 0.02f));
    }
}

void Vehicle::onTrigger(ActorUserData* userData)
{
}

void Vehicle::showDebugInfo()
{
    ImGui::Text("Engine RPM: %f", vehiclePhysics.getEngineRPM());
    ImGui::Text("Speed: %f", vehiclePhysics.getForwardSpeed());
    const char* gearNames[] = { "REVERSE", "NEUTRAL", "1", "2", "3", "4", "5", "6", "7", "8" };
    ImGui::Text("Gear: %s", gearNames[vehiclePhysics.getCurrentGear()]);
    ImGui::Text("Lap Progress: %f", graphResult.currentLapDistance);
    ImGui::Text("Lap Low Mark: %f", graphResult.lapDistanceLowMark);
}

void Vehicle::updatePlayerInput(f32 deltaTime, RenderWorld* rw)
{
    if (driver->useKeyboard || scene->getNumHumanDrivers() == 1)
    {
        input.digital = true;
        input.accel = g_input.isKeyDown(KEY_UP);
        input.brake = g_input.isKeyDown(KEY_DOWN);
        input.steer = (f32)g_input.isKeyDown(KEY_LEFT) - (f32)g_input.isKeyDown(KEY_RIGHT);
        input.beginShoot = g_input.isKeyPressed(KEY_C);
        input.holdShoot = g_input.isKeyDown(KEY_C);
        input.beginShootRear = g_input.isKeyPressed(KEY_V);
        input.holdShootRear = g_input.isKeyDown(KEY_V);
        input.switchFrontWeapon = g_input.isKeyPressed(KEY_X);
        input.switchRearWeapon = g_input.isKeyPressed(KEY_B);
    }
    else if (!driver->controllerGuid.empty())
    {
        Controller* controller = g_input.getController(driver->controllerID);
        if (!controller)
        {
            driver->controllerID = g_input.getControllerId(driver->controllerGuid);
            controller = g_input.getController(driver->controllerID);
        }
        if (controller)
        {
            input.accel = controller->getAxis(AXIS_TRIGGER_RIGHT);
            input.brake = controller->getAxis(AXIS_TRIGGER_LEFT);
            input.steer = -controller->getAxis(AXIS_LEFT_X);
            input.beginShoot = controller->isButtonPressed(BUTTON_RIGHT_SHOULDER);
            input.holdShoot = controller->isButtonDown(BUTTON_RIGHT_SHOULDER);
            input.beginShootRear = controller->isButtonPressed(BUTTON_LEFT_SHOULDER);
            input.holdShootRear = controller->isButtonDown(BUTTON_LEFT_SHOULDER);
            input.switchFrontWeapon = controller->isButtonPressed(BUTTON_X);
            input.switchRearWeapon = controller->isButtonPressed(BUTTON_Y);
        }
    }
    if (scene->getNumHumanDrivers() == 1)
    {
        for (auto& c : g_input.getControllers())
        {
            const Controller* controller = &c.value;
            f32 val = controller->getAxis(AXIS_TRIGGER_RIGHT);
            if (glm::abs(val) > 0.f)
            {
                input.accel = val;
                input.digital = false;
            }
            val = controller->getAxis(AXIS_TRIGGER_LEFT);
            if (glm::abs(val) > 0.f)
            {
                input.brake = val;
                input.digital = false;
            }
            val = -controller->getAxis(AXIS_LEFT_X);
            if (glm::abs(val) > 0.f)
            {
                input.steer = val;
                input.digital = false;
            }
            input.beginShoot = input.beginShoot || controller->isButtonPressed(BUTTON_RIGHT_SHOULDER);
            input.holdShoot = input.holdShoot || controller->isButtonDown(BUTTON_RIGHT_SHOULDER);
            input.beginShootRear = input.beginShootRear || controller->isButtonPressed(BUTTON_LEFT_SHOULDER);
            input.holdShootRear = input.holdShootRear || controller->isButtonDown(BUTTON_LEFT_SHOULDER);
            input.switchFrontWeapon = input.switchFrontWeapon || controller->isButtonPressed(BUTTON_X);
            input.switchRearWeapon = input.switchRearWeapon || controller->isButtonPressed(BUTTON_Y);
        }
    }

#if 1
    if (g_input.isKeyPressed(KEY_F))
    {
        getRigidBody()->addForce(PxVec3(0, 0, 15), PxForceMode::eVELOCITY_CHANGE);
        /*
        getRigidBody()->addTorque(
                getRigidBody()->getGlobalPose().q.rotate(PxVec3(5, 0, 0)),
                PxForceMode::eVELOCITY_CHANGE);
        */
    }

    if (g_input.isKeyPressed(KEY_G))
    {
        getRigidBody()->addForce(PxVec3(0, 0, -50), PxForceMode::eVELOCITY_CHANGE);
    }

    if (g_input.isKeyPressed(KEY_H))
    {
        getRigidBody()->addForce(convert(vehiclePhysics.getForwardVector() * 30.f), PxForceMode::eVELOCITY_CHANGE);
    }

    if (g_input.isKeyPressed(KEY_J))
    {
        getRigidBody()->addForce(convert(vehiclePhysics.getRightVector() * 15.f), PxForceMode::eVELOCITY_CHANGE);
    }
#endif

#if 0
    // test path finding
    if (g_input.isMouseButtonPressed(MOUSE_RIGHT))
    {
        Camera const& cam = rw->getCamera(0);
        glm::vec2 mousePos = g_input.getMousePosition();
        glm::vec3 rayDir = screenToWorldRay(mousePos,
                glm::vec2(g_game.windowWidth, g_game.windowHeight), cam.view, cam.projection);
        PxRaycastBuffer hit;
        if (scene->raycastStatic(cam.position, rayDir, 10000.f, &hit))
        {
            glm::vec3 hitPoint = convert(hit.block.position);
            glm::vec3 forwardVector = getForwardVector();
            bool isSomethingBlockingMe = isBlocking(tuning.collisionWidth / 2 + 0.05f,
                    forwardVector, 20.f);
            scene->getMotionGrid().findPath(currentPosition, hitPoint, isSomethingBlockingMe,
                    forwardVector, motionPath);
        }
    }
#endif
}

void Vehicle::updateAiInput(f32 deltaTime, RenderWorld* rw)
{
    auto& ai = g_ais[driver->aiIndex];

    glm::vec3 currentPosition = vehiclePhysics.getPosition();
    glm::vec3 forwardVector = vehiclePhysics.getForwardVector();
    glm::vec3 rightVector = vehiclePhysics.getRightVector();
    f32 forwardSpeed = vehiclePhysics.getForwardSpeed();

    RacingLine::Point targetPathPoint =
        scene->getPaths()[currentFollowPathIndex].getPointAt(distanceAlongPath);

    // look for other paths if too far off course
    const f32 pathStepSize = 14.f;
    f32 facingBias = glm::dot(forwardVector, glm::normalize(targetPathPoint.position - currentPosition)) * 5.f;
    if (currentFollowPathIndex != preferredFollowPathIndex ||
            glm::distance(currentPosition, targetPathPoint.position) - facingBias > 26.f)
    {
        f32 minPathScore = FLT_MAX;
        for (u32 i=0; i<scene->getPaths().size(); ++i)
        {
            RacingLine::Point testPoint =
                scene->getPaths()[i].getNearestPoint(currentPosition, graphResult.currentLapDistance);
            f32 pathScore = glm::distance(testPoint.position, currentPosition);

            // prioritize the preferred path
            if (i == preferredFollowPathIndex)
            {
                pathScore -= 15.f;
            }

            // prioritize this path if the vehicle is facing the targeted point
            pathScore -= glm::dot(forwardVector, glm::normalize(testPoint.position - currentPosition))
                * 18.f;

            if (minPathScore > pathScore)
            {
                minPathScore = pathScore;
                currentFollowPathIndex = i;
                targetPathPoint = testPoint;
            }
        }
        f32 pathLength = scene->getPaths()[currentFollowPathIndex].length;
        distanceAlongPath = targetPathPoint.distanceToHere
            + (pathLength * glm::max(0, currentLap - 1)) + pathStepSize;
        targetPathPoint = scene->getPaths()[currentFollowPathIndex].getPointAt(distanceAlongPath);
    }
    else
    {
        if (glm::distance2(currentPosition, targetPathPoint.position) < square(16.f))
        {
            distanceAlongPath += pathStepSize;
        }
        else
        {
            f32 pathLength = scene->getPaths()[currentFollowPathIndex].length;
            f32 distanceToHere = scene->getPaths()[currentFollowPathIndex]
                    .getNearestPoint(currentPosition, graphResult.currentLapDistance).distanceToHere
                    + (pathLength * glm::max(0, currentLap - 1));
            if (distanceToHere > distanceAlongPath)
            {
                distanceAlongPath = distanceToHere + pathStepSize;
            }
        }

        // TODO: check if we have line of site to the path point
    }

    // TODO: Use targetSpeed of the racingLine to modulate throttle

    glm::vec2 diff = glm::vec2(previousTargetPosition) - glm::vec2(targetPathPoint.position);
    glm::vec2 dir = glm::length2(diff) > 0.f ? glm::normalize(diff) : glm::vec2(forwardVector);
    glm::vec3 targetP = targetPathPoint.position -
        glm::vec3(targetOffset.x * dir + targetOffset.y * glm::vec2(-dir.y, dir.x), 0);
    glm::vec2 dirToTargetP = glm::normalize(glm::vec2(currentPosition) - glm::vec2(targetP));
    previousTargetPosition = targetPathPoint.position;
#if 0
    Mesh* sphere = g_res.getModel("misc")->getMeshByName("world.Sphere");
    rw->push(LitRenderable(sphere, glm::translate(glm::mat4(1.f), targetP),
                nullptr, glm::vec3(1, 0, 0)));
#endif

    input.accel = 1.f;
    input.brake = 0.f;
    input.steer = clamp(glm::dot(glm::vec2(rightVector), dirToTargetP) * 1.2f, -1.f, 1.f);
    f32 aggression = glm::min(glm::max(((f32)scene->getWorldTime() - 3.f) * 0.3f, 0.f),
            ai.aggression);

    // obstacle avoidance
#if 1
    u32 flags = COLLISION_FLAG_DYNAMIC | COLLISION_FLAG_OIL  |
                COLLISION_FLAG_GLUE    | COLLISION_FLAG_MINE |
                COLLISION_FLAG_CHASSIS;// | COLLISION_FLAG_BOOSTER;
    PxSweepBuffer hit;
    f32 sweepLength = 13.f + ai.awareness * 8.f;
    PxRigidBody* ignoreBody = getRigidBody();
    isBlocked = false;
    isNearHazard = false;
    f32 mySpeed = getRigidBody()->getLinearVelocity().magnitude();
    if (mySpeed < 35.f && scene->sweep(tuning.collisionWidth * 0.5f + 0.05f,
            currentPosition + glm::vec3(0, 0, 0.25f), forwardVector, sweepLength,
            &hit, ignoreBody, flags) && scene->getWorldTime() > 5.f)
    {
        bool shouldAvoid = true;

        // if the object is moving away then don't attempt to avoid it
        if (hit.block.actor->getType() == PxActorType::eRIGID_DYNAMIC)
        {
            f32 mySpeed = this->vehiclePhysics.getForwardSpeed();
            f32 dot = ((PxRigidDynamic*)hit.block.actor)->getLinearVelocity().dot(
                    getRigidBody()->getLinearVelocity());
            if (mySpeed > 5.f && glm::abs(dot - mySpeed) < 5.f)
            {
                shouldAvoid = false;
            }
        }

        // TODO: don't attempt avoid oil and glue if already way off course (this only messes up the AI even more)

        // if chasing a target, don't try to avoid opponent vehicles
        ActorUserData* userData = (ActorUserData*)hit.block.actor->userData;
        if (target && userData && userData->entityType == ActorUserData::VEHICLE)
        {
            shouldAvoid = false;
        }

        // avoid booster pads that are facing the wrong way
        // TODO: fix this (it seems to have the wrong effect; the ai drive toward the backwards boosters)
        if (userData && userData->flags & ActorUserData::BOOSTER)
        {
            if (glm::dot(-dirToTargetP,
                    glm::normalize(glm::vec2(yAxisOf(userData->placeableEntity->transform)))) > 0.5f)
            {
                shouldAvoid = false;
            }
        }

        if (shouldAvoid)
        {
            isBlocked = true;
            isNearHazard = true;
            bool foundOpening = false;
            for (u32 sweepOffsetCount = 1; sweepOffsetCount <= 3; ++sweepOffsetCount)
            {
                // TODO: prefer to swerve to the side that is closer to the angle the AI wanted to steer to anyways
                for (i32 sweepSide = -1; sweepSide < 2; sweepSide += 1)
                {
                    if (sweepSide == 0)
                    {
                        continue;
                    }
                    f32 cw = tuning.collisionWidth * 0.25f;
                    glm::vec3 sweepFrom = currentPosition
                        + rightVector * (f32)sweepSide * (sweepOffsetCount * tuning.collisionWidth * 0.6f);
                    u32 collisionFlags = COLLISION_FLAG_DYNAMIC | COLLISION_FLAG_OIL  |
                                         COLLISION_FLAG_GLUE    | COLLISION_FLAG_MINE |
                                         COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBJECT;
                    if (!scene->sweep(cw, sweepFrom, forwardVector, sweepLength * 0.7f, nullptr,
                            ignoreBody, collisionFlags))
                    {
                        input.steer = -(0.4f + sweepOffsetCount * 0.1f) * sweepSide;
                        foundOpening = true;
                        break;
                    }
                }
                if (foundOpening)
                {
                    break;
                }
            }
        }
    }
    /*
    glm::vec4 c = isBlocked ? glm::vec4(1, 0, 0, 1) : glm::vec4(0, 1, 0, 1);
    scene->debugDraw.line(currentPosition, currentPosition + forwardVector * sweepLength, c, c);
    */
#endif

#if 1
    if (!isInAir && aggression > 0.f)
    {
        // search for target if there is none
        if (!target && frontWeapons.size() > 0
                && frontWeapons[currentFrontWeaponIndex]->ammo > 0)
        {
            f32 maxTargetDist = aggression * 25.f + 15.f;
            f32 lowestTargetPriority = FLT_MAX;
            for (auto& v : scene->getVehicles())
            {
                if (v.get() == this)
                {
                    continue;
                }

                glm::vec2 diff = glm::vec2(v->getPosition()) - glm::vec2(currentPosition);
                glm::vec2 targetDiff = glm::normalize(-diff);
                f32 d = glm::length2(diff);
                f32 dot = glm::dot(glm::vec2(forwardVector), targetDiff);
                f32 targetPriority = d + dot * 4.f;
                // TODO: dot < aggression seems like the wrong calculation
                if (dot < aggression && d < square(maxTargetDist) && targetPriority < lowestTargetPriority)
                {
                    target = v.get();
                    lowestTargetPriority = targetPriority;
                }
            }
        }

        // get behind target
        if (target && !target->isDead())
        {
            targetTimer += deltaTime;
            glm::vec3 diff = target->getPosition() - currentPosition;
            f32 dist = glm::length(diff);
            if (!scene->raycastStatic(currentPosition, diff / dist, dist))
            {
                if (targetTimer > (1.f - aggression) * 2.f)
                {
                    glm::vec2 dirToTarget = glm::normalize(currentPosition - target->getPosition());
                    // only steer toward the target if doing so would not result in veering off course
                    if (glm::dot(dirToTarget, dirToTargetP) > 0.6f - (aggression * 0.4f))
                    {
                        f32 targetSteerAngle = glm::dot(glm::vec2(rightVector), dirToTarget) * 0.4f;
                        input.steer = clamp(targetSteerAngle, -0.5f, 0.5f);
                    }
                    else
                    {
                        targetTimer = 0.f;
                        target = nullptr;
                    }
                }
            }
            if (targetTimer > 3.f + aggression)
            {
                targetTimer = 0.f;
                target = nullptr;
            }
        }
        else
        {
            targetTimer = 0.f;
            target = nullptr;
        }
    }
#endif

    // fear
#if 1
    if (ai.fear > 0.f)
    {
        // TODO: shouldn't this use fear rather than aggression?
        f32 fearRayLength = aggression * 35.f + 10.f;
        if (scene->sweep(0.5f, currentPosition, -forwardVector,
                    fearRayLength, nullptr, getRigidBody(), COLLISION_FLAG_CHASSIS))
        {
            fearTimer += deltaTime;
            if (fearTimer > 1.f * (1.f - ai.fear) + 0.5f)
            {
                input.steer += glm::sin((f32)scene->getWorldTime() * 3.f)
                    * (ai.fear * 0.25f);
            }
            isFollowed = true;
        }
        else
        {
            fearTimer = 0.f;
            isFollowed = false;
        }
        /*
        scene->debugDraw.line(
                currentPosition,
                currentPosition-getForwardVector()*fearRayLength,
                glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));
        */
    }
#endif

    if (scene->canGo() && scene->getWorldTime() > 5.f)
    {
        if (isBackingUp)
        {
            isBackingUp = true;
            input.accel = 0.f;
            input.brake = 1.f;
            input.steer *= -1.f;
            backupTimer += deltaTime;
            if (backupTimer > 4.5f || forwardSpeed < -9.f)
            {
                backupTimer = 0.f;
                isBackingUp = false;
            }
            else if (backupTimer > 1.f &&
                !scene->sweep(tuning.collisionWidth * 0.3f, currentPosition, forwardVector,
                    6.f, nullptr, getRigidBody(), COLLISION_FLAG_OBJECT | COLLISION_FLAG_CHASSIS))
            {
                backupTimer = 0.f;
                isBackingUp = false;
            }
        }
        else
        {
            if (!isInAir && forwardSpeed < 2.5f &&
                scene->sweep(tuning.collisionWidth * 0.35f, currentPosition, forwardVector,
                    4.2f, nullptr, getRigidBody(), COLLISION_FLAG_OBJECT | COLLISION_FLAG_CHASSIS))
            {
                backupTimer += deltaTime;
                if (backupTimer > 0.6f)
                {
                    isBackingUp = true;
                    backupTimer = 0.f;
                }
            }
            else if (!isInAir && forwardSpeed < 2.f)
            {
                backupTimer += deltaTime;
                if (backupTimer > 2.f)
                {
                    isBackingUp = true;
                    backupTimer = 0.f;
                }
            }
            else
            {
                backupTimer = 0.f;
            }

            if (forwardSpeed < -0.25f)
            {
                input.steer *= -1;
            }
        }
    }

    // front weapons
    if (aggression > 0.f && frontWeapons.size() > 0
            && frontWeapons[currentFrontWeaponIndex]->ammo > 0)
    {
        f32 rayLength = aggression * 50.f + 10.f;
        /*
        scene->debugDraw.line(
                currentPosition,
                currentPosition+getForwardVector()*rayLength,
                glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1));
        */
        PxSweepBuffer hit;
        if (scene->sweep(0.5f, currentPosition, forwardVector, rayLength, &hit, getRigidBody(),
                    COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBJECT)
                && hit.block.actor->userData
                && ((ActorUserData*)(hit.block.actor->userData))->entityType == ActorUserData::VEHICLE)
        {
            attackTimer += deltaTime;
        }
        else
        {
            attackTimer = glm::max(attackTimer - deltaTime * 4.f, 0.f);
        }

        if (frontWeapons[currentFrontWeaponIndex]->fireMode == Weapon::CONTINUOUS)
        {
            if (attackTimer > 2.f * (1.f - aggression) + 0.4f)
            {
                input.holdShoot = true;
            }
        }
        else
        {
            if (attackTimer > 2.5f * (1.f - aggression) + 0.7f)
            {
                attackTimer = 0.f;
                input.beginShoot = true;
            }
        }
    }
    else
    {
        attackTimer = 0.f;
    }

    // rear weapons
    if (rearWeapons.size() > 0)
    {
        rearWeaponTimer += deltaTime;
        if (rearWeaponTimer > 0.2f)
        {
            if (rearWeapons[currentRearWeaponIndex]->shouldUse(scene, this))
            {
                input.beginShootRear = true;
            }
            rearWeaponTimer = 0.f;
        }
    }

    // TODO: make the sign correct in the first place
    input.steer *= -1.f;
}
