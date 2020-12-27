#include "vehicle.h"
#include "game.h"
#include "decal.h"
#include "scene.h"
#include "renderer.h"
#include "2d.h"
#include "input.h"
#include "billboard.h"
#include "weapon.h"
#include "imgui.h"

// TODO: play with this value to find best distance
const f32 camDistance = 100.f;

Vehicle::Vehicle(Scene* scene, Mat4 const& transform, Vec3 const& startOffset,
        Driver* driver, VehicleTuning&& tuning, u32 vehicleIndex, i32 cameraIndex)
{
    this->cameraTarget = transform.position() - camDistance * 0.5f;
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
		this->preferredFollowPathIndex = (u32)(max(0, (i32)scene->getPaths().size()) * skill);
		this->currentFollowPathIndex = this->preferredFollowPathIndex;
		// TODO: change preferredFollowPathIndex per lap?
	}
    this->driver = driver;
    this->scene = scene;
    this->lastValidPosition = transform.position();
    this->cameraIndex = cameraIndex;
    this->tuning = std::move(tuning);
    this->hitPoints = this->tuning.maxHitPoints;
    this->previousTargetPosition = transform.position();
    this->rearWeaponTimer = 0.f + vehicleIndex * 0.2f;
    this->placement = vehicleIndex;

    engineSound = g_audio.playSound3D(g_res.getSound("engine2"),
            SoundType::VEHICLE, transform.position(), true);
    tireSound = g_audio.playSound3D(g_res.getSound("tires"),
            SoundType::VEHICLE, transform.position(), true, 1.f, 0.f);

    actorUserData.entityType = ActorUserData::VEHICLE;
    actorUserData.vehicle = this;
    vehiclePhysics.setup(&actorUserData, scene->getPhysicsScene(), transform, &this->tuning);

    // create weapons
    u32 frontWeaponSlotCount = 0;
    u32 weaponSlotCount = driver->getVehicleData()->weaponSlots.size();
    for (u32 i=0; i<weaponSlotCount; ++i)
    {
        VehicleConfiguration* config = driver->getVehicleConfig();
        if (config->weaponIndices[i] != -1)
        {
            auto weapon = g_weapons[config->weaponIndices[i]].create();
            weapon->upgradeLevel = config->weaponUpgradeLevel[i];
            switch(weapon->info.weaponType)
            {
                case WeaponType::FRONT_WEAPON:
                    frontWeapons.push_back(std::move(weapon));
                    frontWeapons.back()->mountTransform = driver->getVehicleData()->weaponMounts[frontWeaponSlotCount];
                    break;
                case WeaponType::REAR_WEAPON:
                    rearWeapons.push_back(std::move(weapon));
                    break;
                case WeaponType::SPECIAL_ABILITY:
                    assert(false);
            }
        }
        if (driver->getVehicleData()->weaponSlots[i].weaponType == WeaponType::FRONT_WEAPON)
        {
            ++frontWeaponSlotCount;
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

void Vehicle::reset(Mat4 const& transform)
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

void Vehicle::drawWeaponAmmo(Renderer* renderer, Vec2 pos, Weapon* weapon,
        bool showAmmo, bool selected)
{
    f32 iconSize = floorf(g_game.windowHeight * 0.05f);
    if (showAmmo)
    {
        Texture* iconbg = g_res.getTexture("weapon_iconbg");
        ui::rect(-100, iconbg, pos, {iconSize * 1.5f, iconSize});
    }
    else
    {
        Texture* iconbg = g_res.getTexture("iconbg");
        ui::rect(-100, iconbg, pos, Vec2(iconSize));
    }

    ui::rect(-90, weapon->info.icon, pos, Vec2(iconSize));
    if (selected)
    {
        Texture* selectedTex = g_res.getTexture("weapon_iconbg_selected");
        ui::rect(-80, selectedTex, pos, Vec2(iconSize * 1.5f, iconSize));
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
            ui::rect(-70, ammoTickTex,
                        pos + Vec2(iconSize + ammoTickMargin * 2.f,
                            ammoTickHeight * i + (iconSize * 0.1f) + ammoTickMargin * 0.5f),
                        Vec2(iconSize * 0.32f, ammoTickHeight - ammoTickMargin));
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
        Vec2 dim(g_game.windowWidth, g_game.windowHeight);
        Vec2 offset = layout.offsets[cameraIndex] * dim;
        Vec2 d(1.f, 1.f);
        if (offset.y > 0.f)
        {
            offset.y = g_game.windowHeight - font2.getHeight();
            d.y = -1;
        }
        Vec2 vdim = dim * layout.scale;
        Vec2 voffset = layout.offsets[cameraIndex] * dim;
        if (voffset.y > 0.f)
        {
            voffset.y = (f32)g_game.windowHeight;
        }

        f32 o20 = (f32)g_game.windowHeight * 0.02f;
        f32 o25 = (f32)g_game.windowHeight * 0.03f;
        f32 o200 = (f32)g_game.windowHeight * 0.21f;

        u32 lap = min((u32)currentLap, scene->getTotalLaps());
        char* p = tmpStr("%u", lap);
        const char* lapStr = "LAP";
        f32 lapWidth = font1.stringDimensions(lapStr).x;
        ui::text(&font1, lapStr, offset + Vec2(o20, d.y*o20), Vec3(1.f));
        ui::text(&font2, p,
                    offset + Vec2(o25 + lapWidth, d.y*o20), Vec3(1.f));
        ui::text(&font1, tmpStr("/%u", scene->getTotalLaps()),
                    offset + Vec2(o25 + lapWidth + font2.stringDimensions(p).x, d.y*o20),
                    Vec3(1.f));

        const char* placementSuffix = "th";
        if (placement == 0) placementSuffix = "st";
        else if (placement == 1) placementSuffix = "nd";
        else if (placement == 2) placementSuffix = "rd";

        Vec3 col = mix(Vec3(0, 1, 0), Vec3(1, 0, 0), placement / 8.f);

        p = tmpStr("%u", placement + 1);
        ui::text(&font2, p, offset + Vec2(o200, d.y*o20), col);
        ui::text(&font1, placementSuffix,
                    offset + Vec2(o200 + font2.stringDimensions(p).x, d.y*o20), col);

        // weapons
        f32 weaponIconX = g_game.windowHeight * 0.35f;
        for (i32 i=0; i<(i32)frontWeapons.size(); ++i)
        {
            auto& w = frontWeapons[i];
            drawWeaponAmmo(renderer, offset +
                    Vec2(weaponIconX, d.y * g_game.windowHeight * 0.018f),
                    w.get(), true, i == currentFrontWeaponIndex && frontWeapons[i]->ammo > 0);
            weaponIconX += g_game.windowHeight * 0.1f;
        }
        for (i32 i=0; i<(i32)rearWeapons.size(); ++i)
        {
            auto& w = rearWeapons[i];
            drawWeaponAmmo(renderer, offset +
                    Vec2(weaponIconX, d.y * g_game.windowHeight * 0.018f),
                    w.get(), true, i == currentRearWeaponIndex && rearWeapons[i]->ammo > 0);
            weaponIconX += g_game.windowHeight * 0.1f;
        }
        if (specialAbility)
        {
            drawWeaponAmmo(renderer, offset +
                    Vec2(weaponIconX, d.y * g_game.windowHeight * 0.018f),
                    specialAbility.get(), false, false);
        }

        // healthbar
        const f32 healthPercentage = clamp(hitPoints / tuning.maxHitPoints, 0.f, 1.f);
        const f32 maxHealthbarWidth = g_game.windowHeight * 0.14f;
        const f32 healthbarWidth = maxHealthbarWidth * healthPercentage;
        const f32 healthbarHeight = g_game.windowHeight * 0.009f;
        Vec2 pos = voffset + Vec2(vdim.x - o20, d.y*o20);
        ui::rect(-100, nullptr, pos + Vec2(-maxHealthbarWidth, healthbarHeight*d.y),
                Vec2(maxHealthbarWidth, healthbarHeight*d.y), Vec3(0));
        ui::rect(-90, nullptr, pos + Vec2(-healthbarWidth, healthbarHeight*d.y),
                Vec2(healthbarWidth, healthbarHeight*d.y), mix(Vec3(1, 0, 0),
                Vec3(0, 1, 0), healthPercentage));

        // display notifications
        u32 count = 0;
        for (auto it = notifications.begin(); it != notifications.end();)
        {
            Vec2 p = floor(layout.offsets[cameraIndex] * dim + layout.scale * dim * 0.5f -
                    Vec2(0, layout.scale.y * dim.y * 0.3f) + Vec2(0, count * dim.y * 0.04f));
            ui::text(&font3, it->text, p,
                it->color, (sinf(it->time * 6.f) + 1.f) * 0.3f + 0.5f,
                max(1.7f - it->time * 3.f, 1.f), HAlign::CENTER, VAlign::CENTER);
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

    Mat4 wheelTransforms[NUM_WHEELS];
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        wheelTransforms[i] = vehiclePhysics.wheelInfo[i].transform;
    }
    Mat4 transform = vehiclePhysics.getTransform();
    driver->getVehicleData()->render(rw, transform,
            wheelTransforms, *driver->getVehicleConfig(), this, isBraking, cameraIndex >= 0,
            Vec4(shieldColor, shieldStrength));
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
            drawSimple(rw, sphere, &g_res.white,
                        Mat4::translation(p.p + Vec3(0, 0, 1))
                            * Mat4::scaling(Vec3(0.25f)), Vec3(1, 1, 0)));

            /*
            Vec4 tp = g_game.renderer->getRenderWorld()->getCamera(0).viewProjection *
                Vec4(p.p + Vec3(0, 0, 1), 1.f);
            tp.x = (((tp.x / tp.w) + 1.f) / 2.f) * g_game.windowWidth;
            tp.y = ((-1.f * (tp.y / tp.w) + 1.f) / 2.f) * g_game.windowHeight;
            g_game.renderer->push2D(TextRenderable(font,
                tstr(std::fixed, std::setprecision(1), p.g, ", ", p.h), { tp.x, tp.y },
                Vec3(0.f, 0.f, 1.f), 1.f, 1.f, HorizontalAlign::CENTER));
            */
        }
    }
#endif
}

void Vehicle::updateCamera(RenderWorld* rw, f32 deltaTime)
{
    Vec3 pos = lastValidPosition;
    pos.z = max(pos.z, -10.f);

#if 0
    cameraTarget = pos + Vec3(0, 0, 2.f);
    cameraFrom = smoothMove(cameraFrom,
            cameraTarget - getForwardVector() * 10.f + Vec3(0, 0, 3.f), 8.f, deltaTime);
    rw->setViewportCamera(cameraIndex, cameraFrom, cameraTarget, 4.f, 200.f, 60.f);
#else
    Vec3 forwardVector = vehiclePhysics.getForwardVector();
    f32 forwardSpeed = vehiclePhysics.getForwardSpeed();
    cameraTarget = smoothMove(cameraTarget,
            pos + Vec3(normalize(forwardVector.xy), 0.f) * forwardSpeed * 0.3f,
            5.f, deltaTime);
    cameraTarget += screenShakeOffset * (screenShakeTimer * 0.5f);
    Vec3 cameraDir = normalize(Vec3(1.f, 1.f, 1.25f));
    cameraFrom = cameraTarget + cameraDir * camDistance;
    rw->setViewportCamera(cameraIndex, cameraFrom, cameraTarget, 18.f, 250.f);
    if (cameraIndex >= 0)
    {
        rw->setHighlightColor(cameraIndex,
                Vec4(driver->getVehicleConfig()->color, 1.f));

        Mat4 m = Mat4(1.f);
        m[0] = Vec4(-cameraDir, m[0].w);
        m[1] = Vec4(normalize(
                    cross(Vec3(0, 0, 1), Vec3(m[0]))), m[1].w);
        m[2] = Vec4(normalize(
                    cross(Vec3(m[0]), Vec3(m[1]))), m[2].w);

        motionBlurStrength = smoothMove(motionBlurStrength, targetMotionBlurStrength, 2.f, deltaTime);

        Vec3 vel = Vec4(m * Vec4(convert(getRigidBody()->getLinearVelocity()), 1.f)).xyz;
        Vec2 motionBlur = vel.xy * motionBlurStrength * 0.01f;
        rw->setMotionBlur(cameraIndex, motionBlur);

        motionBlurResetTimer = max(motionBlurResetTimer - deltaTime, 0.f);
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
                    Vec3 vel = convert(debris.rigidBody->getLinearVelocity());
                    Vec3 pos = convert(debris.rigidBody->getGlobalPose().p);
                    scene->smoke.spawn(pos, vel * 0.5f, min(debris.life - 4.f, 1.f),
                            Vec4(Vec3(0.06f), 1.f), random(scene->randomSeries, 0.5f, 1.f));
                }
            }
#endif
        }
    }

    if (screenShakeTimer > 0.f)
    {
        screenShakeTimer = max(screenShakeTimer - deltaTime, 0.f);
        screenShakeDirChangeTimer -= screenShakeDirChangeTimer;
        if (screenShakeDirChangeTimer <= 0.f)
        {
            screenShakeDirChangeTimer = random(scene->randomSeries, 0.01f, 0.025f);
            Vec3 targetOffset = normalize(Vec3(
                random(scene->randomSeries, -1.f, 1.f),
                random(scene->randomSeries, -1.f, 1.f),
                random(scene->randomSeries, -1.f, 1.f))) * random(scene->randomSeries, 0.75f, 1.5f);
            screenShakeVelocity = normalize(targetOffset - screenShakeOffset) *
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

            if (useResetTransform)
            {
                useResetTransform = false;
                Mat4 resetTransform = Mat4::translation(Vec3(0,0,7.f)) * startTransform;
                reset(resetTransform);
            }
            else
            {
                const TrackGraph::Node* node = graphResult.lastNode;
                if (!graphResult.lastNode)
                {
                    node = scene->getTrackGraph().getEndNode();
                }
                Vec3 pos = graphResult.position;
                pos += Vec3(random(scene->randomSeries, -5.f, 5.f),
                        random(scene->randomSeries, -5.f, 5.f), 0.f);
                pos = scene->findValidPosition(pos, 5.f);
                Mat4 resetTransform =
                    Mat4::translation(pos + Vec3(0, 0, 7.f)) * Mat4::rotationZ(node->angle);
                PxRaycastBuffer hit;
                if (scene->raycastStatic(pos + Vec3(0, 0, 8.f), { 0, 0, -1 }, 20.f, &hit))
                {
                    Vec3 up = hit.block.normal;
                    Vec3 forwardTmp = Vec3(lengthdir(node->angle, 1.f), 0.f);
                    Vec3 right = normalize(cross(up, forwardTmp));
                    Vec3 forward = normalize(cross(right, up));
                    Mat4 m(1.f);
                    m[0] = Vec4(forward, m[0].w);
                    m[1] = Vec4(right, m[1].w);
                    m[2] = Vec4(up, m[2].w);
                    resetTransform = Mat4::translation(pos + up * 7.f) * m;
                }
                reset(resetTransform);
            }

            if (!finishedRace && scene->canGo())
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

    //rw->addPointLight(getPosition() + getForwardVector() * 3.f, Vec3(1.f, 0.9f, 0.8f), 5.f, 2.f);

    input = {};
    if (isPlayerControlled)
    {
        updatePlayerInput(deltaTime, rw);
    }
    else if (scene->getPaths().size() > 0)
    {
        updateAiInput(deltaTime, rw);
    }

    shieldColor = Vec3(0, 0, 0);
    shieldStrength = 0.f;

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
                input.digital, input.accel, input.brake, input.steer, false, true, false);
    }
    else
    {
        vehiclePhysics.update(scene->getPhysicsScene(), deltaTime, false, 0.f,
                controlledBrakingTimer < 0.5f ? 0.f : 0.5f, 0.f, 0.f, true, true);
        if (vehiclePhysics.getForwardSpeed() > 1.f)
        {
            controlledBrakingTimer = min(controlledBrakingTimer + deltaTime, 1.f);
        }
        else
        {
            controlledBrakingTimer = max(controlledBrakingTimer - deltaTime, 0.f);
        }
    }

    Vec3 currentPosition = getPosition();
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
        Vec3 finishLinePosition = scene->getStart().position();
        Vec3 checkPosition = currentPosition + getForwardVector() * tuning.collisionLength * 0.5f;
        Vec3 dir = normalize(checkPosition - finishLinePosition);
        if (dot(scene->getStart().xAxis(), dir) > 0.f
                && lengthSquared(checkPosition - finishLinePosition) < square(40.f))
        {
            if (!scene->canGo())
            {
                blowUp(2.f);
                useResetTransform = true;
            }
            else
            {
                if (!finishedRace && (u32)currentLap >= scene->getTotalLaps())
                {
                    finishedRace = true;
                    scene->vehicleFinish(vehicleIndex);
                    const char* notifications[10] = {
                        "1st PLACE!",
                        "2nd PLACE!",
                        "3rd PLACE!",
                        "4th PLACE!",
                        "5th PLACE!",
                        "6th PLACE!",
                        "7th PLACE!",
                        "8th PLACE!",
                        "9th PLACE!",
                        "10th PLACE!",
                    };
                    addNotification(notifications[placement], 50.f);
                }
                if (currentLap > 0)
                {
                    g_audio.playSound3D(g_res.getSound("lap"), SoundType::GAME_SFX, checkPosition);
                }
                ++currentLap;
                if ((u32)currentLap == scene->getTotalLaps())
                {
                    addNotification("LAST LAP!", 2.f, Vec3(1, 0, 0));
                }
                graphResult.lapDistanceLowMark = scene->getTrackGraph().getStartNode()->t;
                graphResult.currentLapDistance = scene->getTrackGraph().getStartNode()->t;
                resetAmmo();
            }
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
            f32 myDistance = max((i32)currentLap - 1, 0) * lapDistance
                + (lapDistance - graphResult.lapDistanceLowMark);
            f32 otherDistance = max((i32)v->currentLap - 1, 0) * lapDistance
                + (lapDistance - v->graphResult.lapDistanceLowMark);

            if (myDistance - otherDistance > lapDistance + lappingOffset[v->vehicleIndex])
            {
                lappingOffset[v->vehicleIndex] += lapDistance;
                addBonus("LAPPING BONUS!", LAPPING_BONUS_AMOUNT, Vec3(1, 1, 0));
                // TODO: play sound
            }
        }
    }

    if (engineSound)
    {
        f32 rotationSpeed = vehiclePhysics.getAverageWheelRotationSpeed();
        f32 gearRatio = absolute(vehiclePhysics.getCurrentGearRatio());
        engineRPM = smoothMove(engineRPM, min(rotationSpeed * gearRatio, 150.f), 1.9f, deltaTime);

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
        applyDamage(10000.f, vehicleIndex);
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
    smokeTimer = max(0.f, smokeTimer - deltaTime);
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

        f32 slip = max(
                absolute(info.lateralSlip) - 0.4f,
                absolute(info.longitudinalSlip) - 0.6f);
        bool wasWheelSlipping = isWheelSlipping[i];
        isWheelSlipping[i] = (slip > 0.f || info.oilCoverage > 0.f) && !info.isInAir;
        maxSlip = max(maxSlip, slip);

        // create smoke
        if (slip > 0.f && info.isTouchingTrack)
        {
            if (smokeTimer == 0.f)
            {
                Vec3 vel(normalize(Vec3(
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f))));
                scene->smoke.spawn(
                    info.position - Vec3(0, 0, 0.2f),
                    (vel + Vec3(0, 0, 1)) * 0.8f,
                    min(1.f, slip * 0.4f));
                smoked = true;
            }
        }

        if (info.dustAmount > 0.f && smokeTimer == 0.f)
        {
            if (info.rotationSpeed > 5.f || slip > 0.f)
            {
                Vec3 vel(normalize(Vec3(
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f))));
                scene->smoke.spawn(
                    info.position - Vec3(0, 0, 0.2f),
                    (vel + Vec3(0, 0, 1)) * 0.8f,
                    clamp(
                        max(slip, absolute(info.rotationSpeed * 0.022f)) * info.dustAmount,
                        0.f, 1.f),
                    Vec4(0.58f, 0.50f, 0.22f, 1.f));
                smoked = true;
            }
        }

        // add tire marks
        if (isWheelSlipping[i] && info.isTouchingTrack)
        {
            f32 wheelWidth = i < 2 ? tuning.wheelWidthFront : tuning.wheelWidthRear;
            f32 alpha = clamp(max(slip * 3.f, info.oilCoverage), 0.f, 1.f);
            Vec3 color(1.f - clamp(info.oilCoverage, 0.f, 1.f));
            tireMarkRibbons[i].addPoint(info.contactPosition, info.contactNormal, wheelWidth / 2,
                    Vec4(color, alpha));
        }
        else if (wasWheelSlipping)
        {
            tireMarkRibbons[i].capWithLastPoint();
        }
    }
    if (!isTouchingAnyGlue)
    {
        glueSoundTimer = max(glueSoundTimer - deltaTime, 0.f);
    }

    g_audio.setSoundVolume(tireSound,
            isOnTrack ? min(1.f, min(maxSlip * 1.2f,
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
    smokeTimerDamage = max(0.f, smokeTimerDamage - deltaTime);
    f32 damagePercent = hitPoints / tuning.maxHitPoints;
    if (smokeTimerDamage <= 0.f && damagePercent < 0.3f)
    {
        // TODO: make the effect more intense the more critical the damage (fire and sparks?)
        Vec3 vehicleVel = previousVelocity + vehiclePhysics.getForwardVector();
        Vec3 vel = Vec3(normalize(Vec3(
                random(scene->randomSeries, -1.f, 1.f),
                random(scene->randomSeries, -1.f, 1.f),
                random(scene->randomSeries, -1.f, 1.f))))
            + Vec3(0, 0, 1)
            + vehicleVel * 0.5f;
        scene->smoke.spawn(currentPosition + Vec3(0, 0, 0.5f),
            vel, 1.f - damagePercent, Vec4(Vec3(0.5f), 1.f), 0.5f);
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
                addBonus("BIG AIR BONUS!", BIG_AIR_BONUS_AMOUNT, Vec3(1, 0.6f, 0.05f));
                ++totalAirBonuses;
            }
            else if (savedAirTime > 2.f)
            {
                addBonus("AIR BONUS!", AIR_BONUS_AMOUNT, Vec3(1, 0.7f, 0.05f));
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
        screenShakeOffset = Vec3(0.f);
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
        if (controller)
        {
            if (g_game.config.gameplay.forceFeedbackEnabled)
            {
                controller->playHaptic(0.5, 1000);
            }
        }
    }
}

void Vehicle::blowUp(f32 respawnTime)
{
    Mat4 transform = vehiclePhysics.getTransform();
    for (auto& d : g_vehicles[driver->vehicleIndex]->debrisChunks)
    {
        PxRigidDynamic* body = g_game.physx.physics->createRigidDynamic(
                PxTransform(convert(transform * d.transform)));
        body->attachShape(*d.collisionShape);
        PxRigidBodyExt::updateMassAndInertia(*body, 10.f);
        scene->getPhysicsScene()->addActor(*body);
        body->setLinearVelocity(
                convert(previousVelocity) +
                convert(Vec3(normalize(transform.rotation() * Vec4(d.transform.position(), 1.f)))
                    * random(scene->randomSeries, 3.f, 14.f) + Vec3(0, 0, 9.f)));
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
    deadTimer = respawnTime;
    scene->createExplosion(transform.position(), previousVelocity, 10.f);
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
    reset(Mat4::translation({ 0, 0, 1000 }));
    if (raceStatistics.destroyed + raceStatistics.accidents == 10)
    {
        addBonus("VICTIM BONUS", VICTIM_BONUS_AMOUNT, Vec3(1.f, 0.3f, 0.02f));
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
        controller = g_input.getController(driver->controllerID);
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
        for (auto& cp : g_input.getControllers())
        {
            Controller* c = &cp.value;
            f32 val = c->getAxis(AXIS_TRIGGER_RIGHT);
            if (absolute(val) > 0.f)
            {
                input.accel = val;
                input.digital = false;
                controller = c;
            }
            val = c->getAxis(AXIS_TRIGGER_LEFT);
            if (absolute(val) > 0.f)
            {
                input.brake = val;
                input.digital = false;
                controller = c;
            }
            val = -c->getAxis(AXIS_LEFT_X);
            if (absolute(val) > 0.f)
            {
                input.steer = val;
                input.digital = false;
                controller = c;
            }
            input.beginShoot = input.beginShoot || c->isButtonPressed(BUTTON_RIGHT_SHOULDER);
            input.holdShoot = input.holdShoot || c->isButtonDown(BUTTON_RIGHT_SHOULDER);
            input.beginShootRear = input.beginShootRear || c->isButtonPressed(BUTTON_LEFT_SHOULDER);
            input.holdShootRear = input.holdShootRear || c->isButtonDown(BUTTON_LEFT_SHOULDER);
            input.switchFrontWeapon = input.switchFrontWeapon || c->isButtonPressed(BUTTON_X);
            input.switchRearWeapon = input.switchRearWeapon || c->isButtonPressed(BUTTON_Y);
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

    if (g_input.isKeyPressed(KEY_U))
    {
        if (controller)
        {
            controller->playHaptic(0.5, 1000);
        }
    }
#endif

#if 0
    // test path finding
    if (g_input.isMouseButtonPressed(MOUSE_RIGHT))
    {
        Camera const& cam = rw->getCamera(0);
        Vec2 mousePos = g_input.getMousePosition();
        Vec3 rayDir = screenToWorldRay(mousePos,
                Vec2(g_game.windowWidth, g_game.windowHeight), cam.view, cam.projection);
        PxRaycastBuffer hit;
        if (scene->raycastStatic(cam.position, rayDir, 10000.f, &hit))
        {
            Vec3 hitPoint = convert(hit.block.position);
            Vec3 forwardVector = getForwardVector();
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

    Vec3 currentPosition = vehiclePhysics.getPosition();
    Vec3 forwardVector = vehiclePhysics.getForwardVector();
    Vec3 rightVector = vehiclePhysics.getRightVector();
    f32 forwardSpeed = vehiclePhysics.getForwardSpeed();

    RacingLine::Point targetPathPoint =
        scene->getPaths()[currentFollowPathIndex].getPointAt(distanceAlongPath);

    // look for other paths if too far off course
    const f32 pathStepSize = 14.f;
    f32 facingBias = dot(forwardVector, normalize(targetPathPoint.position - currentPosition)) * 5.f;
    if (currentFollowPathIndex != preferredFollowPathIndex ||
            distance(currentPosition, targetPathPoint.position) - facingBias > 26.f)
    {
        f32 minPathScore = FLT_MAX;
        for (u32 i=0; i<scene->getPaths().size(); ++i)
        {
            RacingLine::Point testPoint =
                scene->getPaths()[i].getNearestPoint(currentPosition, graphResult.currentLapDistance);
            f32 pathScore = distance(testPoint.position, currentPosition);

            // prioritize the preferred path
            if (i == preferredFollowPathIndex)
            {
                pathScore -= 15.f;
            }

            // prioritize this path if the vehicle is facing the targeted point
            pathScore -= dot(forwardVector, normalize(testPoint.position - currentPosition))
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
            + (pathLength * max(0, currentLap - 1)) + pathStepSize;
        targetPathPoint = scene->getPaths()[currentFollowPathIndex].getPointAt(distanceAlongPath);
    }
    else
    {
        if (distanceSquared(currentPosition, targetPathPoint.position) < square(16.f))
        {
            distanceAlongPath += pathStepSize;
        }
        else
        {
            f32 pathLength = scene->getPaths()[currentFollowPathIndex].length;
            f32 distanceToHere = scene->getPaths()[currentFollowPathIndex]
                    .getNearestPoint(currentPosition, graphResult.currentLapDistance).distanceToHere
                    + (pathLength * max(0, currentLap - 1));
            if (distanceToHere > distanceAlongPath)
            {
                distanceAlongPath = distanceToHere + pathStepSize;
            }
        }

        // TODO: check if we have line of site to the path point
    }

    // TODO: Use targetSpeed of the racingLine to modulate throttle

    Vec2 diff = Vec2(previousTargetPosition) - Vec2(targetPathPoint.position);
    Vec2 dir = lengthSquared(diff) > 0.f ? normalize(diff) : Vec2(forwardVector);
    Vec3 targetP = targetPathPoint.position -
        Vec3(targetOffset.x * dir + targetOffset.y * Vec2(-dir.y, dir.x), 0);
    Vec2 dirToTargetP = normalize(Vec2(currentPosition) - Vec2(targetP));
    previousTargetPosition = targetPathPoint.position;
#if 0
    Mesh* sphere = g_res.getModel("misc")->getMeshByName("world.Sphere");
    drawSimple(rw, sphere, &g_res.white, Mat4::translation(targetP),
                Vec3(1, 0, 0)));
#endif

    input.accel = (scene->timeUntilStart() < ai.drivingSkill * 0.35f + 0.0025f) ? 1.f : 0.f;
    input.accel *= 0.8;
    input.brake = 0.f;
    input.steer = clamp(dot(Vec2(rightVector), dirToTargetP) * 1.2f, -1.f, 1.f);
    f32 aggression = min(max(((f32)scene->getWorldTime() - 3.f) * 0.3f, 0.f),
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
            currentPosition + Vec3(0, 0, 0.25f), forwardVector, sweepLength,
            &hit, ignoreBody, flags) && scene->getWorldTime() > 5.f)
    {
        bool shouldAvoid = true;

        // if the object is moving away then don't attempt to avoid it
        if (hit.block.actor->getType() == PxActorType::eRIGID_DYNAMIC)
        {
            f32 mySpeed = this->vehiclePhysics.getForwardSpeed();
            f32 dot = ((PxRigidDynamic*)hit.block.actor)->getLinearVelocity().dot(
                    getRigidBody()->getLinearVelocity());
            if (mySpeed > 5.f && absolute(dot - mySpeed) < 5.f)
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
            if (dot(-dirToTargetP,
                    normalize(Vec2(userData->placeableEntity->transform.yAxis()))) > 0.5f)
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
                    Vec3 sweepFrom = currentPosition
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
    Vec4 c = isBlocked ? Vec4(1, 0, 0, 1) : Vec4(0, 1, 0, 1);
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

                Vec2 diff = Vec2(v->getPosition()) - Vec2(currentPosition);
                Vec2 targetDiff = normalize(-diff);
                f32 d = lengthSquared(diff);
                f32 vDot = dot(Vec2(forwardVector), targetDiff);
                f32 targetPriority = d + vDot * 4.f;
                // TODO: vDot < aggression seems like the wrong calculation
                if (vDot < aggression && d < square(maxTargetDist) && targetPriority < lowestTargetPriority)
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
            Vec3 diff = target->getPosition() - currentPosition;
            f32 dist = length(diff);
            if (!scene->raycastStatic(currentPosition, diff / dist, dist))
            {
                if (targetTimer > (1.f - aggression) * 2.f)
                {
                    Vec2 dirToTarget = normalize(Vec2(currentPosition - target->getPosition()));
                    // only steer toward the target if doing so would not result in veering off course
                    if (dot(dirToTarget, dirToTargetP) > 0.6f - (aggression * 0.4f))
                    {
                        f32 targetSteerAngle = dot(Vec2(rightVector), dirToTarget) * 0.4f;
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
                input.steer += sinf((f32)scene->getWorldTime() * 3.f)
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
                Vec4(1, 0, 0, 1), Vec4(1, 0, 0, 1));
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
                Vec4(0, 1, 0, 1), Vec4(0, 1, 0, 1));
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
            attackTimer = max(attackTimer - deltaTime * 4.f, 0.f);
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
