#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/World.hpp"
#include "Game/TileMap.hpp"
#include "Game/GameCommon.hpp"
#include "Game/MapMaterial.hpp"
#include "Game/MapRegion.hpp"
#include "Game/Entity.hpp"
#include "Game/Actor.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/Server.hpp"
#include "Game/NetworkObserver.hpp"
#include "Game/Client.hpp"
#include "Game/MaterialSheet.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/MeshUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MatrixUtils.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/GPUMesh.hpp"
#include "Engine/Renderer/Sampler.hpp"
#include "Engine/Core/AxisConvention.hpp"
#include "Engine/Audio/AudioSystem.hpp"

//////////////////////////////////////////////////////////////////////////
void Game::StartUp()
{
    g_theGame = this;
    m_gameClock = new Clock();
    g_theObserver->UpdateSendTimer(m_gameClock);
    g_theRNG = new RandomNumberGenerator();
    g_theRenderer->SetupParentClock(m_gameClock);
    g_theFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

     //camera
    m_worldCamera = new Camera();
    m_worldCamera->SetClearMode(CLEAR_DEPTH_BIT | CLEAR_COLOR_BIT, Rgba8::BLACK, 1.f);
    Vec2 windDim = g_theApp->GetWindowDimensions();
    Vec2 halfDim = windDim * .5f;
    m_worldCamera->SetOrthoView(-halfDim, halfDim);
    m_worldCamera->SetProjectionPerspective(60.f, -.1f, -100.f);
    m_worldCamera->SetAxisConvention(AXIS__YZ_X);

    m_uiCamera = new Camera();
    m_uiCamera->SetOrthoView(-halfDim, halfDim);
    m_uiCamera->SetProjectionOrthographic(1080.f);

    //init world       
    MapMaterialType::InitMapMaterialTypesFromFile("data/definitions/mapMaterialTypes.xml");
    MapRegionType::InitMapRegionTypesFromFile("data/definitions/mapRegionTypes.xml");
    EntityDef::InitEntityDefinitionsFromFile("data/definitions/entityTypes.xml");
    m_world = new World(this, "data/maps/");
}

//////////////////////////////////////////////////////////////////////////
void Game::ShutDown()
{
    delete m_world;
    delete m_worldCamera;
    delete m_uiCamera;
    delete g_theRNG;

    for (Entity* e : m_entities) {
        delete e;
    }
}

//////////////////////////////////////////////////////////////////////////
void Game::Update()
{
    m_world->Update();
}

//////////////////////////////////////////////////////////////////////////
void Game::UpdateLocal()
{
    m_world->UpdateLocal();

    for (int i = 0; i < m_entities.size(); i++) {
        Entity* e = m_entities[i];
        if (e && e->IsGarbage()) {
            delete e;
            m_entities[i] = nullptr;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void Game::Render(Entity* playerPawn) const
{
    RenderForWorld(playerPawn);
    RenderForUI(playerPawn);
    RenderForDebug();

    DebugRenderWorldToCamera(m_worldCamera);
}

//////////////////////////////////////////////////////////////////////////
void Game::InitializeAssets(AudioSystem* audioSys, RenderContext* rtx)
{
    //load
    MaterialSheet::InitRenderAssets();
    MapMaterialType::InitRenderAssets();
    m_world->InitForRenderAssets();

    m_testSoundID = audioSys->CreateOrGetSound("data/audio/Teleporter.wav");
    Texture* hudTex = rtx->CreateOrGetTextureFromFile("data/images/hud_base.png");
    rtx->CreateOrGetTextureFromFile("data/images/terrain_8x8.png");
    Texture* viewTex = rtx->CreateOrGetTextureFromFile("data/images/viewModelsSpriteSheet_8x8.png");

    //init UI
    Vec2 windDim = g_theApp->GetWindowDimensions();
    IntVec2 hudTexSize = hudTex->GetTextureSize();
    float hudHeight = windDim.x / (float)hudTexSize.x * (float)hudTexSize.y;
    AABB2 uiBounds = m_uiCamera->GetBounds();
    m_hud = AABB2(uiBounds.mins, Vec2(uiBounds.maxs.x, uiBounds.mins.y + hudHeight));

    float gunHalfHeight = 1000.f - hudHeight * .5f;
    Vec2 halfGun(gunHalfHeight, gunHalfHeight);
    m_gun = AABB2(-halfGun, halfGun);
    m_gun.Translate(Vec2(0.f, m_hud.maxs.y + gunHalfHeight));
    m_viewSheet = new SpriteSheet(*viewTex, IntVec2(8, 8));
}

//////////////////////////////////////////////////////////////////////////
Entity* Game::SpawnEntityAtPlayerStart(size_t playerIdx, std::string const& entityDefName)
{
    Entity* entity = m_world->SpawnEntityAtPlayerStart(playerIdx, entityDefName);
    entity->SetIsControlledByAI(false);
    return entity;
}

//////////////////////////////////////////////////////////////////////////
void Game::SwitchMapForEntity(std::string const& mapName, Entity* playerPawn)
{
    m_world->SwitchMap(playerPawn, mapName);
}

//////////////////////////////////////////////////////////////////////////
void Game::RemoveEntity(Entity* playerPawn)
{
    if (playerPawn == nullptr) {
        return;
    }

    playerPawn->GetMap()->RemoveEntity(playerPawn);
    for (size_t i = 0; i < m_entities.size(); i++) {
        if (m_entities[i] == playerPawn) {
            m_entities[i] = nullptr;
            break;
        }
    }

    playerPawn->MarkAsGarbage();
}

//////////////////////////////////////////////////////////////////////////
void Game::AddNewlySpawnedEntity(Entity* entity)
{
    entity->SetIndex(m_entityIdx++);
    for (size_t i = 0; i < m_entities.size(); i++) {
        if (m_entities[i] == nullptr){
            m_entities[i] = entity;
            return;
        }
    }

    m_entities.push_back(entity);
}

//////////////////////////////////////////////////////////////////////////
void Game::PlayTeleportSound() const
{
    g_theServer->PlayGlobalSound(m_testSoundID);
}

//////////////////////////////////////////////////////////////////////////
void Game::MoveEntity(Entity* playerPawn, Vec2 const& pos, Vec3 const& pitchYawRoll)
{
    playerPawn->SetPosition(pos);
    playerPawn->SetPitchYawRollDegrees(pitchYawRoll);
}

//////////////////////////////////////////////////////////////////////////
void Game::UpdateCameraForPawn(Entity* playerPawn)
{
    m_worldCamera->SetPosition(playerPawn->GetEntityEyePosition());
    m_worldCamera->SetPitchYawRollDegreesClamp(playerPawn->GetEntityPitchYawRollDegrees());
}

//////////////////////////////////////////////////////////////////////////
void Game::UpdatePlayerForInput(Entity* playerPawn, InputInfo const& info)
{
    if (playerPawn == nullptr) {
        g_theConsole->PrintString(Rgba8::YELLOW, "update null pawn for input info");
        return;
    }

    float deltaSeconds = (float)m_gameClock->GetLastDeltaSeconds();

    Vec2 actualMove = info.playerMove * playerPawn->GetWalkSpeed() * deltaSeconds;
    Vec3 pawnForward = playerPawn->GetEntityForward();
    Vec2 forward(pawnForward.x,pawnForward.y);
    forward.Normalize();
    Vec2 left=forward.GetRotated90Degrees();
    Vec2 worldTrans = actualMove.x*forward + actualMove.y*left;

    Vec2 pitchYawDelta;
    pitchYawDelta.y = -info.mouseMove.x * deltaSeconds * CAMERA_ROTATE_SPEED;
    pitchYawDelta.x = -info.mouseMove.y * deltaSeconds * CAMERA_ROTATE_SPEED;

    playerPawn->Translate(worldTrans);
    playerPawn->RotateDeltaPitchYawRollDegrees(pitchYawDelta);
}

//////////////////////////////////////////////////////////////////////////
void Game::UpdateKeyboardStates(InputInfo& info) const
{
    UpdateInputForMovement(info);

    //fire bullet
    if (g_theInput->WasKeyJustPressed(MOUSE_LBUTTON)) {
        info.isFiring=true;
    }
    else {
        info.isFiring = false;
    }

    //ESC quit
    if (g_theInput->WasKeyJustPressed(KEY_ESC))
    {
        info.isClosing = true;
    }
    else {
        info.isClosing = false;
    }
}

//////////////////////////////////////////////////////////////////////////
Entity* Game::GetEntityOfIndex(int idx) const
{
    for (Entity* e : m_entities) {
        if (e && e->GetIndex() == idx) {
            return e;
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void Game::UpdateInputForMovement(InputInfo& info) const
{
    Vec2 horizontalDeltaPos;
    if (g_theInput->IsKeyDown('W')) {
        horizontalDeltaPos.x += 1.f;
    }
    if (g_theInput->IsKeyDown('S')) {
        horizontalDeltaPos.x -= 1.f;
    }
    if (g_theInput->IsKeyDown('A')) {
        horizontalDeltaPos.y += 1.f;
    }
    if (g_theInput->IsKeyDown('D')) {
        horizontalDeltaPos.y -= 1.f;
    }
    info.playerMove = horizontalDeltaPos;
    info.mouseMove = g_theInput->GetMouseRelativeMove();
}

//////////////////////////////////////////////////////////////////////////
void Game::RenderForWorld(Entity* playerPawn) const
{
    g_theRenderer->BeginCamera(m_worldCamera);

    m_world->Render(playerPawn);

    g_theRenderer->EndCamera(m_worldCamera);
}

//////////////////////////////////////////////////////////////////////////
void Game::RenderForUI(Entity* playerPawn) const
{
    AABB2 bounds = m_uiCamera->GetBounds();

    g_theRenderer->BeginCamera(m_uiCamera);
    g_theRenderer->BindShaderStateByName("data/shaders/states/ui.shaderstate");

    TODO("actual update hud for player pawn");
    //hud
    g_theRenderer->BindDiffuseTexture("data/images/hud_base.png");
    g_theRenderer->DrawAABB2D(m_hud);

    //gun
    g_theRenderer->BindDiffuseTexture("data/images/viewModelsSpriteSheet_8x8.png");
    Vec2 gunMins, gunMaxs;
    m_viewSheet->GetSpriteUVs(gunMins, gunMaxs, 0);
    g_theRenderer->DrawAABB2D(m_gun, Rgba8::WHITE, gunMins, gunMaxs);

    //health
    std::vector<Vertex_PCU> verts;
    if (playerPawn->GetEntityType() == ENTITY_ACTOR) {
        Actor* player = (Actor*)playerPawn;
        float health = player->GetHealth();
        std::string text = Stringf("%.0f", health);
        g_theFont->AddVertsForTextInBox2D(verts, m_hud, 40.f, text.c_str(), Rgba8(200,200,200), 
            1.f, Vec2(.29f,.63f));
        g_theRenderer->BindDiffuseTexture(g_theFont->GetTexture());
        g_theRenderer->DrawVertexArray(verts);
        verts.clear();
    }

    //debug text
    std::string printText = Stringf("[F1] Debug Mode\nFrametime: %.5f\nFPS: %.2f\n[-,+] Send: %.1f", m_gameClock->GetLastDeltaSeconds(), 
        1.0 / m_gameClock->GetLastDeltaSeconds(), g_sendRatePerSec);
    g_theFont->AddVertsForTextInBox2D(verts, bounds, 15.f, printText.c_str(),
        Rgba8::WHITE, 1.f, ALIGN_TOP_RIGHT);
    g_theRenderer->BindDiffuseTexture(g_theFont->GetTexture());
    g_theRenderer->DrawVertexArray(verts);

    g_theRenderer->EndCamera(m_uiCamera);
}

//////////////////////////////////////////////////////////////////////////
void Game::RenderForDebug() const
{
    if (!g_debugDrawing) {
        return;
    }

    AABB2 bounds = m_uiCamera->GetBounds();

    //debug draw screen basis
    Vec3 camPos = m_worldCamera->GetPosition();
    Vec3 forward = (m_worldCamera->ClientToWorldPosition(Vec2(.5f, .5f), 1.f) - camPos).GetNormalized();
    Mat44 basis;
    basis.Translate3D(camPos + .12f * forward);
    basis.ScaleUniform3D(.01f);
    DebugAddWorldBasis(basis, 0.f, DEBUG_RENDER_ALWAYS);
    //debug draw world basis
    DebugAddWorldBasis(Mat44::IDENTITY, 0.f, DEBUG_RENDER_ALWAYS);

    Vec3 rotation = m_worldCamera->GetPitchYawRollDegrees();
    Vec3 position = m_worldCamera->GetPosition();
    Mat44 camModel = m_worldCamera->GetCameraModel();
    std::string uiHint = Stringf("[F4] Start Jobs\n[F5] Claim Complete Jobs\n[F6] Close Job System\n[w,a,s,d] Cam Horizontal Move\n\
[q,e]     Cam Vertical Move\n[~]     Open Console\n\
Camera:\n    (pitch: %.1f, yaw: %.1f, roll: %.1f) [i] reset\n    Position: (%.2f, %.2f, %.2f)\n    iBasis: (%.2f, %.2f, %.2f)\n\
    jBasis: (%.2f, %.2f, %.2f)\n    kBasis: (%.2f, %.2f, %.2f)",
        rotation.x, rotation.y, rotation.z, position.x, position.y, position.z, camModel.Ix, camModel.Iy, camModel.Iz,
        camModel.Jx, camModel.Jy, camModel.Jz, camModel.Kx, camModel.Ky, camModel.Kz);
    DebugAddScreenText(bounds, Vec2(0.02f, 0.98f), 15.f, Rgba8::WHITE, Rgba8::WHITE, 0.f, uiHint.c_str());
}

//////////////////////////////////////////////////////////////////////////
void Game::RenderForRaycastDebug(Entity* pawn) const
{
    RaycastResult result = pawn->GetRaycastResult();
    if (result.didImpact) {
        DebugAddWorldPoint(result.impactPosition, .05f, Rgba8::RED);
        Rgba8 transWhite(255, 255, 255, 100);
        DebugAddWorldArrow(result.impactPosition, Rgba8::WHITE, Rgba8::WHITE,
            result.impactPosition + .3f * result.impactSurfaceNormal, transWhite, transWhite, 0.f);
    }
}

