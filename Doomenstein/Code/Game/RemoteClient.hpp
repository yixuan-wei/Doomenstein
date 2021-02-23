#pragma once

#include "Game/Client.hpp"

class RemoteClient : public Client 
{
public:
    RemoteClient();

    void Startup(Entity* playerPawn, Server* server) override;    //confirm start up
    void BeginFrame() override{}    //receive input; offer world info
    void Update() override {}
    void Render() const override{}
    void EndFrame() override {}
    void Shutdown() override;

    bool PlaySoundOnClient(size_t id) override;
    bool CouldUpdateInput() const override {return m_playerPawn;}
};