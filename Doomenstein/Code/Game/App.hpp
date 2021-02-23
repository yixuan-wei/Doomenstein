#pragma once

#include "Engine/Math/Vec2.hpp"
#include <string>

class SingleplayerGame;
class Window;
class InputSystem;
class AuthoritativeServer;
class PlayerClient;
struct IntVec2;
enum eGameType:int;

//---------------------------------------------------
class App
{
public:
	App()=default;
	~App()=default;
	void Startup();
	void Shutdown();
	void RunFrame();

	bool IsQuiting() const { return m_isQuiting; };
	bool HandleQuitRequisted();

	void StartServer(eGameType gameType, int port);
	void StartClient(eGameType gameType, std::string const& serverIP, int serverPort);

	Window* GetWindow() const {return m_theWindow;}
	Vec2 GetWindowDimensions() const;
	IntVec2 GetDimensions() const;

private:
	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();

	void ClearExistingServerAndClient();

	//Variables
	bool  m_isQuiting = false;
	Window* m_theWindow = nullptr;
	SingleplayerGame* m_theGame = nullptr;

	AuthoritativeServer* m_theServer = nullptr;
	PlayerClient* m_theClient = nullptr;
};