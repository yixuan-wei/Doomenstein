#pragma once

#include "Game/Entity.hpp"

class Actor : public Entity
{
public:
    Actor(Map* map, EntityDef const* definition);

    void Update() override;
    void OnProjectileHit(Projectile* proje) override;

    void SetHealth(float newHealth);

    float GetHealth() const {return m_health;}

public:
    unsigned short m_lastHealthSeqNo = 0;

private:
    float m_health = 0.f;
};