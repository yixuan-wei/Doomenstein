#include "Game/PlayerClient.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/AuthoritativeServer.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/MathUtils.hpp"

//////////////////////////////////////////////////////////////////////////
PlayerClient::PlayerClient()
{
    DebugRenderSystemStartup(g_theRenderer);
    EnableDebugRendering();

    g_theInput->PushMouseOptions(eMousePositionMode::MOUSE_RELATIVE, false, true);
}

//////////////////////////////////////////////////////////////////////////
void PlayerClient::Startup(Entity* playerPawn, Server* server)
{
    m_playerPawn = playerPawn;
    m_server = server;

    m_server->m_theGame->InitializeAssets(g_theAudio,g_theRenderer);
}

//////////////////////////////////////////////////////////////////////////
void PlayerClient::BeginFrame()
{
    g_theConsole->BeginFrame();

    DebugRenderBeginFrame();

    if (!g_theConsole->IsOpen() && m_playerPawn) {
        m_server->m_theGame->UpdateKeyboardStates(m_input);
        m_shot = m_input.isFiring || m_shot;
    }
}

//////////////////////////////////////////////////////////////////////////
void PlayerClient::Update()
{
    g_theConsole->Update();

    if (g_theConsole->IsOpen()) {
        return;
    }

    //~ console
    if (g_theInput->WasKeyJustPressed(KEY_TILDE))
    {
        bool isOpen = g_theConsole->IsOpen();
        g_theConsole->SetIsOpen(!isOpen);
    }

    //debug drawing
    if (g_theInput->WasKeyJustPressed(KEY_F1)) {
        g_debugDrawing = !g_debugDrawing;
    }

    float deltaChange = (float)g_theGame->GetClock()->GetLastDeltaSeconds() * SEND_RATE_CHANGE_RATE;
    if (g_theInput->IsKeyDown(KEY_MINUS)) {
        g_sendRatePerSec-=deltaChange;
    }
    if (g_theInput->IsKeyDown(KEY_PLUS)) {
        g_sendRatePerSec += deltaChange;
    }
    g_sendRatePerSec = Clamp(g_sendRatePerSec,10.f,30.f);
}

//////////////////////////////////////////////////////////////////////////
void PlayerClient::Render() const
{
    m_server->m_theGame->Render(m_playerPawn);

    g_theConsole->Render(g_theRenderer);
    DebugRenderScreenTo(g_theRenderer->GetFrameColorTarget());
}

//////////////////////////////////////////////////////////////////////////
void PlayerClient::EndFrame()
{
    DebugRenderEndFrame();
    g_theConsole->EndFrame();    
}

//////////////////////////////////////////////////////////////////////////
void PlayerClient::Shutdown()
{
    Client::Shutdown();
    
    g_theInput->PopMouseOptions();
    DebugRenderSystemShutdown();   
}

//////////////////////////////////////////////////////////////////////////
bool PlayerClient::PlaySoundOnClient(size_t id)
{
    SoundPlaybackID playback = g_theAudio->PlaySound(id);
    return playback!=MISSING_SOUND_ID;
}

//////////////////////////////////////////////////////////////////////////
bool PlayerClient::CouldUpdateInput() const
{
    return !g_theConsole->IsOpen() && m_playerPawn;
}
