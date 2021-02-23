#pragma once

#include "Game/Client.hpp"

class RenderContext;
class InputSystem;
class BitmapFont;
class AudioSystem;
class DevConsole;

class PlayerClient : public Client 
{
public:
    PlayerClient();

    void Startup(Entity* playerPawn, Server* server) override;     //request server to add player
    void BeginFrame() override;  //request server world info
    void Update() override;      //send user input; 
    void Render() const override;      //render graphics and sounds
    void EndFrame() override;
    void Shutdown() override;    //request server to delete player

    bool PlaySoundOnClient(size_t id) override;
    bool CouldUpdateInput() const override;
};