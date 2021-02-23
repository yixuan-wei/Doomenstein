#include "Game/Map.hpp"
#include "Game/GameCommon.hpp"
#include "Game/TileMap.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/Entity.hpp"
#include "Game/Actor.hpp"
#include "Game/Projectile.hpp"
#include "Game/Game.hpp"
#include "Game/Portal.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/MeshUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/GPUMesh.hpp"
#include "Engine/Renderer/DebugRender.hpp"

GPUMesh* sEntityDebugMesh = nullptr;

//////////////////////////////////////////////////////////////////////////
static void InitEntityDebugMesh()
{
    if (sEntityDebugMesh != nullptr) {
        return;
    }

    sEntityDebugMesh = new GPUMesh(g_theRenderer);
    std::vector<Vertex_PCU> verts;
    std::vector<unsigned int> inds;
    AppendIndexedVertexesForCylinder(verts, inds, Vec3::ZERO, Vec3(0.f, 0.f, 1.f), 1.f, 8, Rgba8(0, 255, 255));
    sEntityDebugMesh->UpdateIndices((unsigned int)inds.size(), &inds[0]);
    sEntityDebugMesh->UpdateVertices((unsigned int)verts.size(), &verts[0]);

}

//////////////////////////////////////////////////////////////////////////
static void ClearEntityDebugMesh()
{
    if (sEntityDebugMesh != nullptr) {
        delete sEntityDebugMesh;
        sEntityDebugMesh = nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////
Map* Map::CreateFromMapPath(char const* mapDefPath)
{
    XmlDocument mapDefDoc;
    XmlError code = mapDefDoc.LoadFile(mapDefPath);
    if (code != XmlError::XML_SUCCESS) {
        g_theConsole->PrintError(Stringf("Fail to load map %s, code %i", mapDefPath, (int)code));
        return nullptr;
    }

    XmlElement* root = mapDefDoc.RootElement();
    if (root == nullptr) {
        g_theConsole->PrintError(Stringf("Fail to retrieve root element of %s", mapDefPath));
        return nullptr;
    }
    if (strcmp(root->Name(), "MapDefinition") != 0) {
        g_theConsole->PrintError(Stringf("Root %s invalid for %s", root->Name(), mapDefPath));
        return nullptr;
    }

    std::string type = ParseXmlAttribute(*root, "type", "");
    if (type == "TileMap") {
        TileMap* newMap = new TileMap(*root);
        if(newMap->IsValid()){
            newMap->m_name = GetFileNameFromPath(mapDefPath);
            return (Map*)newMap;
        }
        else {
            g_theConsole->PrintError(Stringf("Fail to construct map %s", mapDefPath));
            delete newMap;
            return nullptr;
        }
    }
    else {
        g_theConsole->PrintError(Stringf("Unrecognized map type %s in %s", type.c_str(), mapDefPath));
        return nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////
void Map::InitForRenderAssets()
{
    m_mesh = new GPUMesh(g_theRenderer);
    InitEntityDebugMesh();
}

//////////////////////////////////////////////////////////////////////////
void Map::DebugDrawEntity(Mat44 const& entityMat) const
{
    DebugAddWireMeshToWorld(entityMat, sEntityDebugMesh, Rgba8::WHITE, 0.f, DEBUG_RENDER_XRAY);
}

void Map::ClearForRenderAssets()
{
    ClearEntityDebugMesh();
}

//////////////////////////////////////////////////////////////////////////
Entity* Map::SpawnNewEntityOfType(std::string const& entityDefName)
{
    EntityDef* definition = EntityDef::GetEntityDefinitionFromName(entityDefName);
    if (definition == nullptr) {
        g_theConsole->PrintError(Stringf("Fail to entity definition of %s", entityDefName.c_str()));
        return nullptr;
    }
    else {
        return SpawnNewEntityOfType(*definition);
    }
}

//////////////////////////////////////////////////////////////////////////
Entity* Map::SpawnNewEntityOfType(EntityDef const& entityDef)
{
    return SpawnNewEntity(&entityDef);
}

//////////////////////////////////////////////////////////////////////////
Entity* Map::GetEntityWithinRange(Vec3 const& forward, float sectorDegrees, Vec3 const& position, float maxDist) const
{
    float maxDistSquared = maxDist*maxDist;
    for (size_t i = 0; i < m_entities.size(); i++) {
        Entity* entity =  m_entities[i];
        if (entity == nullptr) {
            continue;
        }

        Vec3 entityPos = entity->GetEntityPosition();
        if (GetDistanceSquared3D(entityPos, position) <= maxDistSquared) {
            float angleDegrees = GetAngleDegreesBetweenVectors3D(forward, entityPos-position) * 2.f;
            if (angleDegrees >= -sectorDegrees && angleDegrees <= sectorDegrees) {
                return entity;
            }
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
PlayerStart const& Map::GetPlayerStartOfIndex(size_t index)
{
    size_t idx = index%m_playerStarts.size();
    return m_playerStarts[idx];
}

//////////////////////////////////////////////////////////////////////////
Entity* Map::SpawnNewEntity(EntityDef const* entityDefinition)
{
    eEntityType type = entityDefinition->m_type;
    Entity* newEntity = nullptr;
    switch (type) {
        case ENTITY_ACTOR:      newEntity = new Actor(this, entityDefinition); break;
        case ENTITY_PORTAL:     newEntity = new Portal(this, entityDefinition);break;
        case ENTITY_PROJECTILE: newEntity = new Projectile(this, entityDefinition); break;
        case ENTITY_ENTITY:     newEntity = new Entity(this, entityDefinition); break;
        case ENTITY_INVALID:    return nullptr;
    }

    AppendNewEntity(newEntity);
    g_theGame->AddNewlySpawnedEntity(newEntity);
    return newEntity;
}

//////////////////////////////////////////////////////////////////////////
void Map::AppendNewEntity(Entity* newEntity)
{
    switch (newEntity->GetEntityType()) {
        case ENTITY_ACTOR:      AppendNewActor((Actor*)newEntity); break;
        case ENTITY_PORTAL:     AppendNewPortal((Portal*)newEntity); break;
        case ENTITY_PROJECTILE: AppendNewProjectile((Projectile*)newEntity); break;
    }

    for (size_t i = 0; i < m_entities.size(); i++) {
        Entity* entity = m_entities[i];
        if (entity == nullptr) {
            m_entities[i] = newEntity;
            return;
        }
    }
    m_entities.push_back(newEntity);
}

//////////////////////////////////////////////////////////////////////////
void Map::AppendNewActor(Actor* newActor)
{
    for (size_t i = 0; i < m_actors.size(); i++) {
        Actor* actor = m_actors[i];
        if (actor == nullptr) {
            m_actors[i] = newActor;
            return;
        }
    }

    m_actors.push_back(newActor);
}

//////////////////////////////////////////////////////////////////////////
void Map::AppendNewPortal(Portal* newPortal)
{
    for (size_t i = 0; i < m_portals.size(); i++) {
        Portal* portal = m_portals[i];
        if (portal == nullptr) {
            m_portals[i] = newPortal;
            return;
        }
    }

    m_portals.push_back(newPortal);
}

//////////////////////////////////////////////////////////////////////////
void Map::AppendNewProjectile(Projectile* newProjectile)
{
    for (size_t i = 0; i < m_projectiles.size(); i++) {
        Projectile* proj = m_projectiles[i];
        if (proj == nullptr) {
            m_projectiles[i] = newProjectile;
            return;
        }
    }

    m_projectiles.push_back(newProjectile);
}

//////////////////////////////////////////////////////////////////////////
void Map::RemoveEntity(Entity* entity)
{
    for (size_t i = 0; i < m_entities.size(); i++) {
        if (m_entities[i] == entity) {
            m_entities[i] = nullptr;
            break;
        }
    }

    switch (entity->GetEntityType())
    {
    case ENTITY_ACTOR:      RemoveActor((Actor*)entity); break;
    case ENTITY_PORTAL:     RemovePortal((Portal*)entity); break;
    case ENTITY_PROJECTILE: RemoveProjectile((Projectile*)entity); break;
    default:                break;
    }
}

//////////////////////////////////////////////////////////////////////////
void Map::RemoveActor(Actor* actor)
{
    for (size_t i = 0; i < m_actors.size(); i++) {
        if (m_actors[i] == actor) {
            m_actors[i] = nullptr;
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void Map::RemovePortal(Portal* portal)
{
    for (size_t i = 0; i < m_portals.size(); i++) {
        if (m_portals[i] == portal) {
            m_portals[i] = nullptr;
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void Map::RemoveProjectile(Projectile* projectile)
{
    for (size_t i = 0; i < m_projectiles.size(); i++) {
        if (m_projectiles[i] == projectile) {
            m_projectiles[i]=nullptr;
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
RaycastResult Map::RaycastForEntities(Vec3 const& startPos, Vec3 const& forwardNormal, float maxDist) const
{
    RaycastResult result;
    result.impactDistance = maxDist;
    Vec2 start(startPos.x, startPos.y);
    Vec2 forward(forwardNormal.x, forwardNormal.y);
    forward.Normalize();

    for (size_t i=0;i<m_entities.size();i++) {
        Entity* e = m_entities[i];
        if (e != nullptr) {
            Vec2 entityPos = e->GetEntityPosition2D();
            Vec2 startToE = entityPos-start;
            float discProjected = DotProduct2D(startToE, forward);
            if (discProjected < 0.f || discProjected >= result.impactDistance) {  //opposite direction
                 continue;
            }

            float startDiscSqurd = startToE.GetLengthSquared();            
            float discToForwardSqrd = startDiscSqurd-discProjected*discProjected;
            float radius = e->GetEntityRadius();
            float radiusSqrd = radius*radius;
            if (discToForwardSqrd >= radiusSqrd) { //no impact
                continue;
            }

            float delta = SqrtFloat(radiusSqrd-discToForwardSqrd);
            float multiplier = 1.f/SqrtFloat(forwardNormal.x*forwardNormal.x + forwardNormal.y*forwardNormal.y);
            float minInterDist = (discProjected-delta)*multiplier;
            if (minInterDist >= result.impactDistance || minInterDist<0.f) {    //already impacted before, inside entity
                continue;
            }

            float maxInterDist = (discProjected+delta)*multiplier;
            float startInterZ = startPos.z + minInterDist*forwardNormal.z;
            float endInterZ = startPos.z + maxInterDist*forwardNormal.z;
            FloatRange intersectZ(startInterZ,endInterZ);
            FloatRange entityHeight(0.f, e->GetEntityHeight());
            float surfaceDir = -1.f;
            float entityInTarget = entityHeight.minimum;
            if (forwardNormal.z < 0.f) {
                intersectZ = FloatRange(endInterZ, startInterZ);
                entityInTarget = entityHeight.maximum;
                surfaceDir = 1.f;
                
            }
            if (intersectZ.DoesOverlap(entityHeight)) { //impacted
                result.impactDistance = RangeMapFloat(startInterZ, endInterZ, minInterDist, maxInterDist, entityInTarget);
                result.impactDistance = Clamp(result.impactDistance, minInterDist, maxInterDist);
                result.didImpact = true;
                result.impactEntity = e;
                result.impactPosition = startPos + result.impactDistance * forwardNormal;
                if (result.impactDistance == minInterDist) {    //impact on sides
                    result.impactSurfaceNormal = (Vec2(result.impactPosition.x, result.impactPosition.y) - entityPos).GetNormalized();
                }
                else {  //impact on top
                    result.impactSurfaceNormal = Vec3(0.f, 0.f, surfaceDir);
                }
            }
        }
    }
    return result;
}
