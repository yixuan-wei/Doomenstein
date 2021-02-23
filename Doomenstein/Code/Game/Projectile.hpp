#pragma once

#include "Game/Entity.hpp"

class Projectile : public Entity
{
public:
    Projectile(Map* map, EntityDef const* definition);

    void Update() override;
    void Render(Entity* playerPawn) const override;

    FloatRange GetEntityHeightRange() const override;

    void SetHeight(float newHeight);
    void SetDamage(float newDamage);

    float GetDamage() const {return m_damage;}
    float GetFlyHeight() const {return m_height;}

private:
    float m_height = 0.f;
    float m_damage = 0.f;
};