#pragma once

#include "Game/Game.hpp"

//////////////////////////////////////////////////////////////////////////
class SingleplayerGame : public Game
{
	friend class AuthoritativeServer;
	friend class RemoteServer;

public:
	SingleplayerGame();
};