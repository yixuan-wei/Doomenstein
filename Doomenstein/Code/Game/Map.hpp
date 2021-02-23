#pragma once

#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Mat44.hpp"

class GPUMesh;
class Entity;
class Actor;
class Portal;
class Projectile;
class EntityDef;
class Camera;
class World;

struct RaycastResult
{
    bool didImpact = false;
    Vec3 impactPosition;
    Entity* impactEntity = nullptr;
    float impactDistance = 0.f;
    Vec3 impactSurfaceNormal;
};

struct PlayerStart
{
    Vec2 position;
    float yaw = 0.f;
};

//////////////////////////////////////////////////////////////////////////
class Map
{
    friend class World;

public:
    static Map* CreateFromMapPath(char const* path);

    void InitForRenderAssets();
    void DebugDrawEntity(Mat44 const& entityMat) const;
    void ClearForRenderAssets();

    virtual ~Map() = default;

    virtual void UpdateMeshes() = 0;
    virtual void UpdateForCollisions() = 0;
    virtual void UpdateForEntities() = 0;
    virtual void UpdateForOutOfMap() = 0;
    virtual void UpdateForEntitiesLocal(float deltaSeconds) = 0;
    virtual void Render(Entity* playerPawn) const = 0;

    virtual RaycastResult Raycast(Vec3 const& startPos, Vec3 const& forwardNormal, float maxDist) const = 0;

    virtual Entity* SpawnNewEntityOfType(std::string const& entityDefName);
    virtual Entity* SpawnNewEntityOfType(EntityDef const& entityDef);

    void AppendNewEntity(Entity* newEntity);
    void RemoveEntity(Entity* entity);

    Entity* GetEntityWithinRange(Vec3 const& forward, float sectorDegrees, Vec3 const& position, float maxDist) const;
    bool IsValid() const {return m_isValid;}
    std::string GetName() const {return m_name;}
    PlayerStart const& GetPlayerStartOfIndex(size_t index);

protected:
    std::string m_name;
    GPUMesh* m_mesh = nullptr;
    bool m_isValid = false;

    Entity* SpawnNewEntity(EntityDef const* newEntity);
    void AppendNewActor(Actor* newActor);
    void AppendNewPortal(Portal* newPortal);
    void AppendNewProjectile(Projectile* newProjectile);

    void RemoveActor(Actor* actor);
    void RemovePortal(Portal* portal);
    void RemoveProjectile(Projectile* projectile);

    virtual RaycastResult RaycastForEntities(Vec3 const& startPos, Vec3 const& forwardNormal, float maxDist) const;

public:
    std::vector<PlayerStart> m_playerStarts;
    World* m_theWorld = nullptr;

    std::vector<Entity*>     m_entities;
    std::vector<Actor*>      m_actors;
    std::vector<Portal*>     m_portals;
    std::vector<Projectile*> m_projectiles;

};