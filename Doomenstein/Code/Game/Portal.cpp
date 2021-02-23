#include "Game/Portal.hpp"
#include "Game/Projectile.hpp"
#include "Game/TileMap.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"

//////////////////////////////////////////////////////////////////////////
Portal::Portal(Map* map, EntityDef const* definition)
    : Entity(map, definition)
{

}

//////////////////////////////////////////////////////////////////////////
void Portal::OnProjectileHit(Projectile* proje)
{
    Map* projMap = proje->GetMap();
    Vec3 pitchYawRoll = proje->GetEntityPitchYawRollDegrees();
    g_theGame->MoveEntity(proje, m_destPos, pitchYawRoll+Vec3(0.f,m_destYawOffset,0.f));
    if (!m_destMap.empty() && m_destMap != projMap->GetName()) {
        g_theGame->SwitchMapForEntity(m_destMap,proje);
    }
}
