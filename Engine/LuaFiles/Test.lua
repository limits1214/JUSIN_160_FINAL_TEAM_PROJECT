local Script = {}

local function Check(condition, message)
    if not condition then
        error("[Lua Smoke Test] " .. message)
    end
end

function Script.OnCreate(ctx)
    Check(ctx.ownerHandle ~= nil, "ownerHandle was not supplied.")
    Check(Object.IsValid(ctx.ownerHandle), "ownerHandle is invalid.")
    Check(ctx.ownerHandle:IsValid(), "ObjectHandle:IsValid failed.")

    local invalidHandle = ObjectHandle()
    Check(not Object.IsValid(invalidHandle), "default handle must be invalid.")
    Check(Transform.GetPosition(invalidHandle) == nil,
        "invalid Transform access must return nil.")

    local objectTag = Object.GetTag(ctx.ownerHandle)
    Check(objectTag == "LuaTestObject" or objectTag == "LuaSpawnedTest",
        "object tag binding returned an unexpected value.")
    Check(Object.GetType(ctx.ownerHandle) == "CLuaTestObject",
        "Lua test object type binding returned an unexpected value.")

    Check(Object.CanSpawn("LuaTestObject"),
        "LuaTestObject spawn factory was not registered.")
    Check(not Object.CanSpawn("MissingSpawnType"),
        "unknown spawn type must not be registered.")

    local moveIntentTag = "LuaMoveIntent"
    Check(Component.CanAttach("CharacterMoveIntent"),
        "CharacterMoveIntent attach factory was not registered.")
    Check(not Component.CanAttach("MissingComponentType"),
        "unknown component type must not be registered.")
    Check(not Component.Has(ctx.ownerHandle, moveIntentTag),
        "move intent component must not exist before attach.")
    Check(Component.Attach(
        ctx.ownerHandle,
        "CharacterMoveIntent",
        moveIntentTag,
        {}),
        "registered component attach failed.")
    Check(Component.Has(ctx.ownerHandle, moveIntentTag),
        "attached move intent component was not found.")
    Check(not Component.Attach(
        ctx.ownerHandle,
        "CharacterMoveIntent",
        moveIntentTag),
        "duplicate component tag must be rejected.")

    Check(MoveIntent.SetMove(
        ctx.ownerHandle,
        moveIntentTag,
        Vector3(0.0, 0.0, 1.0),
        1.0),
        "failed to write move intent.")
    Check(MoveIntent.ClearMove(ctx.ownerHandle, moveIntentTag),
        "failed to clear move intent.")
    print("[Smoke Test] Component attach checks passed", moveIntentTag)

    local position = Transform.GetPosition(ctx.ownerHandle)
    Check(position ~= nil, "failed to read owner Transform.")
    Check(Transform.SetPosition(ctx.ownerHandle, position),
        "failed to write owner Transform.")

    ctx.reloadCount = 0
    ctx.canRunSpawnTest = objectTag == "LuaTestObject"
    ctx.moveIntentTag = moveIntentTag
    print("[Smoke Test] Create checks passed", ctx.ownerHandle)
end

function Script.OnReload(ctx)
    ctx.reloadCount = (ctx.reloadCount or 0) + 1
    print("[Smoke Test] Hot reload passed", ctx.reloadCount)
end

function Script.OnDestroy(ctx)
    if ctx.sound2DTestId ~= nil and Sound.IsValid(ctx.sound2DTestId) then
        Sound.Stop(ctx.sound2DTestId)
    end
    if ctx.sound3DTestId ~= nil and Sound.IsValid(ctx.sound3DTestId) then
        Sound.Stop(ctx.sound3DTestId)
    end
    print("[Smoke Test] OnDestroy", ctx.ownerHandle)
end

function Script.Update(ctx, dt)
    --print(dt)
end

function Script.PriorityUpdate(ctx, dt)
    if Input.WasKeyPressed(Key.Space) then
        print("[Smoke Test] Input passed: Space Down")
    end

    if Input.IsKeyHeld(Key.W) then
        local position = Transform.GetPosition(ctx.ownerHandle)
        if position ~= nil then
            Transform.SetPosition(
                ctx.ownerHandle,
                position + Vector3(0.0, 0.0, dt))
        end
    end

    if Input.WasKeyReleased(Key.Escape) then
        print("[Smoke Test] Input passed: Escape Up")
    end

    if Input.WasMousePressed(Mouse.Left) then
        print("[Smoke Test] Input passed: Left Click")
    end

    if Input.WasKeyPressed(Key.Delete) then
        Check(Object.Destroy(ctx.ownerHandle), "Destroy request failed.")
        Check(not Object.IsValid(ctx.ownerHandle),
            "pending-destroy handle must become invalid immediately.")
        print("[Smoke Test] Destroy and invalid-handle checks passed")
    end

    if ctx.canRunSpawnTest and Input.WasKeyPressed(Key.Insert) then
        local spawnedHandle = Object.Spawn(
            "LuaTestObject",
            "00_LuaSpawnedTest",
            { tag = "LuaSpawnedTest" })

        Check(Object.IsValid(spawnedHandle), "registered spawn failed.")
        Check(Object.GetTag(spawnedHandle) == "LuaSpawnedTest",
            "spawn parameter table was not applied.")
        print("[Smoke Test] Registered spawn passed", spawnedHandle)
    end

    if Input.WasKeyPressed(Key.F6) then
        local soundId = Sound.Play2D(
            "./Resources/SampleClient/Sound/UI/ButtonSelect.wav",
            {
                bus = SoundBus.UI,
                volume = 0.5,
                pitch = 1.0
            })
        Check(soundId ~= Sound.InvalidID, "2D sound playback failed.")
        ctx.sound2DTestId = soundId
        print("[Smoke Test] Sound.Play2D passed", soundId)
    end

    if Input.WasKeyPressed(Key.F7) then
        local position = Transform.GetPosition(ctx.ownerHandle)
        local soundId = Sound.Play3D(
            "./Resources/SampleClient/Sound/Player/StepSound/Step_01.wav",
            position,
            {
                bus = SoundBus.SFX,
                volume = 1.0,
                minDistance = 1.0,
                maxDistance = 20.0,
                rolloff = SoundRolloff.INVERSE
            })
        Check(soundId ~= Sound.InvalidID, "3D sound playback failed.")
        ctx.sound3DTestId = soundId
        print("[Smoke Test] Sound.Play3D passed", soundId)
    end

    if Input.WasKeyPressed(Key.F8) then
        if ctx.sound2DTestId ~= nil and Sound.IsValid(ctx.sound2DTestId) then
            Check(Sound.FadeOutAndStop(ctx.sound2DTestId, 0.5),
                "2D sound fade-out failed.")
        end
        if ctx.sound3DTestId ~= nil and Sound.IsValid(ctx.sound3DTestId) then
            Check(Sound.FadeOutAndStop(ctx.sound3DTestId, 0.5),
                "3D sound fade-out failed.")
        end
        print("[Smoke Test] Sound fade-out requested")
    end

    local dx = Input.MouseDelta(MouseMove.X)
    local dy = Input.MouseDelta(MouseMove.Y)
    if dx ~= 0 or dy ~= 0 then
        -- print("Mouse:", dx, dy)
    end
end

return Script
