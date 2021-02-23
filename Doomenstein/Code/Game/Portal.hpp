#pragma once

#include "Game/Entity.hpp"
#include <string>

class Portal : public Entity
{
public:
    Portal(Map* map, EntityDef const* definition);

    void OnProjectileHit(Projectile* proje) override;

public:
    std::string m_destMap;
    Vec2 m_destPos;
    float m_destYawOffset = 0.f;
};