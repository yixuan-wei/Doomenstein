#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Server.hpp"
#include "Game/Client.hpp"
#include "Game/PlayerClient.hpp"
#include "Game/AuthoritativeServer.hpp"
#include "Game/RemoteServer.hpp"
#include "Game/NetworkObserver.hpp"
#include "Game/NetworkMessage.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Core/Job.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Network/Network.hpp"
#include "Engine/Network/TCPServer.hpp"

static NamedProperties props;

//////////////////////////////////////////////////////////////////////////
COMMAND(StartMultiplayerServer, "start server, port=48000", eEventFlag::EVENT_CONSOLE)
{
	int port = args.GetValue("0", 48000);

	g_theApp->StartServer(GAME_MULTI_PLAYER, port);
	return true;
}

//////////////////////////////////////////////////////////////////////////
COMMAND(ConnectToMultiplayerServer, "connect server, ip=127.0.0.1, port=48000", eEventFlag::EVENT_CONSOLE)
{
	std::string ip = args.GetValue("ip","127.0.0.1");
	int port = args.GetValue("port",48000);
	if (ip == "127.0.0.1" && port == 48000) {
		ip=args.GetValue("0", "127.0.0.1");
		port = args.GetValue("1", 48000);
	}

	g_theApp->StartClient(GAME_MULTI_PLAYER, ip, port);
	return true;
}

//////////////////////////////////////////////////////////////////////////
COMMAND(quit, "Quit the game", eEventFlag::EVENT_GLOBAL) {
	UNUSED(args)
	g_theApp->HandleQuitRequisted();
	return true;
}

//////////////////////////////////////////////////////////////////////////
void App::Startup()
{
	//config setting
	float windowClientRatioOfHeight = g_gameConfigBlackboard->GetValue( "windowHeightRatio", 0.8f );
	float aspectRatio = g_gameConfigBlackboard->GetValue( "windowAspect", 16.0f / 9.0f );
	std::string windowTitle = g_gameConfigBlackboard->GetValue( "windowTitle", "SD2.A01" );
	props.SetValue("aspect", aspectRatio);
	props.SetValue("clientHeightRaio", windowClientRatioOfHeight);
	props.SetValue("title", windowTitle);

	g_theApp = &(*this);		
    g_theEvents = new EventSystem();
    g_theRenderer = new RenderContext();
    g_theInput = new InputSystem();
	g_theConsole = new DevConsole(g_theInput);
    g_theAudio = new AudioSystem();
	g_theNetwork = new Network();	
	g_theObserver = new NetworkObserver();

	//set up window
	m_theWindow = new Window();
	m_theWindow->Open( windowTitle, aspectRatio, windowClientRatioOfHeight );
	m_theWindow->SetInputSystem(g_theInput);
	props.SetValue("dimension", IntVec2((int)m_theWindow->GetClientWidth(), (int)m_theWindow->GetClientHeight()));
    
    g_theRenderer->Startup(g_theApp->GetWindow());
    g_theInput->Startup();
	g_theConsole->Startup();
    g_theAudio->Startup();
    g_theNetwork->Startup(); 

    g_theServer = new AuthoritativeServer();
    g_theClient = new PlayerClient();
	g_theServer->Startup(GAME_SINGLE_PLAYER);
    g_theServer->AddPlayer(g_theClient);

	Clock::SystemStartup();
	InitJobSystem(); 
}

//////////////////////////////////////////////////////////////////////////
void App::Shutdown()
{
	CloseJobSystem();
	Clock::SystemShutdown();

	g_theServer->Shutdown();
	g_theNetwork->Shutdown();
    g_theAudio->Shutdown();
    g_theRenderer->Shutdown();
    g_theInput->Shutdown();
    g_theConsole->Shutdown();
	m_theWindow->Close();

	delete g_theObserver;
	g_theObserver = nullptr;

    delete g_theConsole;
    g_theConsole = nullptr;

    delete g_theRenderer;
    delete g_theAudio;
    delete g_theInput;
    g_theRenderer = nullptr;
    g_theAudio = nullptr;
    g_theInput = nullptr;

	delete g_theNetwork;
	g_theNetwork = nullptr;

	delete g_theServer;
	g_theServer = nullptr;

	delete g_theEvents;
	g_theEvents = nullptr;

	delete m_theWindow;
	m_theWindow = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void App::RunFrame()
{
	BeginFrame();     //engine only
	Update();//game only
	Render();         //game only
	EndFrame();	      //engine only
}

//////////////////////////////////////////////////////////////////////////
bool App::HandleQuitRequisted()
{
	m_isQuiting = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////
void App::StartServer(eGameType gameType, int port)
{
	ClearExistingServerAndClient();	

	AuthoritativeServer* authorServer = new AuthoritativeServer();
	authorServer->SetUpTCPServer(port);
	g_theServer = static_cast<Server*>(authorServer);
	g_theClient = new PlayerClient();
    g_theServer->Startup(gameType);
    g_theServer->AddPlayer(g_theClient);
}

//////////////////////////////////////////////////////////////////////////
void App::StartClient(eGameType gameType, std::string const& serverIP, int serverPort)
{
	ClearExistingServerAndClient();

	RemoteServer* remoServer = new RemoteServer();
	remoServer->SetUpTCPClient(serverIP,serverPort);
	g_theServer = static_cast<Server*>(remoServer);
	g_theClient = new PlayerClient();
	g_theServer->Startup(gameType);
	g_theServer->AddPlayer(g_theClient);
}

//////////////////////////////////////////////////////////////////////////
Vec2 App::GetWindowDimensions() const
{
	return props.GetValue("dimension", Vec2(1920.f, 1080.f));
}

//////////////////////////////////////////////////////////////////////////
IntVec2 App::GetDimensions() const
{
	return props.GetValue("dimension", IntVec2(1920, 1080));
}

//////////////////////////////////////////////////////////////////////////
void App::BeginFrame()
{
	Clock::BeginFrame();

	m_theWindow->BeginFrame();
	g_theNetwork->BeginFrame();
	g_theServer->BeginFrame();

	JobSystemBeginFrame();
}

//////////////////////////////////////////////////////////////////////////
void App::Update()
{
	if( m_theWindow->IsQuiting() )
	{
		HandleQuitRequisted();
		return;
	}

	g_theServer->Update();
}

//////////////////////////////////////////////////////////////////////////
void App::Render() const
{	
	if (IsQuiting()) {
		return;
	}

	g_theServer->Render();
}

//////////////////////////////////////////////////////////////////////////
void App::EndFrame()
{
	g_theObserver->EndFrame();
	g_theServer->EndFrame();
    g_theAudio->EndFrame();
    g_theInput->EndFrame();
    g_theRenderer->EndFrame();
	m_theWindow->EndFrame();
}

//////////////////////////////////////////////////////////////////////////
void App::ClearExistingServerAndClient()
{
    g_theNetwork->DisconnectClients();
	g_theNetwork->StopServers();
	g_theNetwork->StopUDPSockets();
	g_theObserver->Restart();

    if (g_theServer != nullptr) {
        g_theServer->Shutdown();
        delete g_theServer;
        g_theServer = nullptr;
    }
}

