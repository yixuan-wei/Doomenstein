#include "Game/Projectile.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Game/EntityDefinition.hpp"
#include "Engine/Core/Clock.hpp"

//////////////////////////////////////////////////////////////////////////
Projectile::Projectile(Map* map, EntityDef const* definition)
    : Entity(map, definition)
{
    m_damage = definition->m_damage.GetRandomInRange(*g_theRNG);
}

//////////////////////////////////////////////////////////////////////////
void Projectile::Update()
{
    float deltaSeconds = (float)g_theGame->GetClock()->GetLastDeltaSeconds();
    Vec3 deltaMove = m_forward*deltaSeconds*m_entityDef->m_speed;
    Translate(Vec2(deltaMove.x,deltaMove.y));
    m_height += deltaMove.z;
}

//////////////////////////////////////////////////////////////////////////
void Projectile::Render(Entity* playerPawn) const
{
    RenderForPosition(playerPawn->GetEntityEyePosition(), playerPawn->GetEntityForward(),
        Vec3(m_position, m_height));
}

//////////////////////////////////////////////////////////////////////////
FloatRange Projectile::GetEntityHeightRange() const
{
    float halfHeight = m_entityDef->m_height*.5f;
    return FloatRange(m_height-halfHeight,m_height+halfHeight);
}

//////////////////////////////////////////////////////////////////////////
void Projectile::SetHeight(float newHeight)
{
    m_height = newHeight;
}

//////////////////////////////////////////////////////////////////////////
void Projectile::SetDamage(float newDamage)
{
    m_damage = newDamage;
}
