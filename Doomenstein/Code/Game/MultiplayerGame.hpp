#pragma once

#include "Game/Game.hpp"

//////////////////////////////////////////////////////////////////////////
class MultiplayerGame : public Game
{
    friend class AuthoritativeServer;
    friend class RemoteServer;

public:
    MultiplayerGame();
};