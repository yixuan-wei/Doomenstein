#pragma once

#include <vector>
#include <string>
#include <queue>
#include "Engine/Core/Timer.hpp"

class Entity;

void PackUpMessages(std::vector<std::string> const& msgs, std::queue<std::string>& packages, bool reliable=false);

//////////////////////////////////////////////////////////////////////////
class NetworkObserver
{
public:
    NetworkObserver();

    void AddEntityTransformUpdate(Entity* entity);
    void AddSoundPlay(size_t id);
    void AddMessage(std::string const& package);

    void Restart();
    void EndFrame();  //send messages; clear

    void UpdateSendTimer(Clock* clock);

private:
    void UpdateSoundPlayMessages();
    void UpdateEntityTransformMessages();
    void UpdatePackages();

private:
    std::vector<size_t> m_SFXToPlay;
    std::vector<Entity*> m_entityTransformChanged;

    std::vector<std::string> m_messages;
    std::queue<std::string> m_packages;

    Timer m_sendTimer;
};