#include "Game/Actor.hpp"
#include "Game/Game.hpp"
#include "Game/Projectile.hpp"
#include "Game/Server.hpp"
#include "Game/AuthoritativeServer.hpp"
#include "Game/NetworkObserver.hpp"
#include "Game/EntityDefinition.hpp"
#include "Engine/Core/Clock.hpp"

//////////////////////////////////////////////////////////////////////////
Actor::Actor(Map* map, EntityDef const* definition)
    : Entity(map, definition)
{
    m_health = definition->m_health;
}

//////////////////////////////////////////////////////////////////////////
void Actor::Update()
{
    Entity::Update();

    if (m_isControlledByAI) {
        float timer = m_timerAI;
        float deltaSeconds = (float)g_theGame->GetClock()->GetLastDeltaSeconds();
        Vec2 forward(m_forward.x, m_forward.y);
        if (timer > 1.f) {
            forward = -forward;
        }
        Translate(forward * deltaSeconds);
    }
}

//////////////////////////////////////////////////////////////////////////
void Actor::OnProjectileHit(Projectile* proje)
{
    if (!g_theServer->m_isAuthoritative) {
        return;
    }

    AuthoritativeServer* server = (AuthoritativeServer*)g_theServer;
    m_health -= proje->GetDamage();
    server->AddHealthMessageToClients(this);
    if (m_health < 0) {
        server->DeleteEntity(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void Actor::SetHealth(float newHealth)
{
    m_health = newHealth;
}
