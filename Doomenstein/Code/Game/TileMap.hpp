#pragma once

#include "Game/Map.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <vector>

class MapTile;
struct Vertex_PCU;

class TileMap : public Map
{
public:
    TileMap(XmlElement const& root);
    ~TileMap() override;

    void UpdateForEntities() override;
    void UpdateForEntitiesLocal(float deltaSeconds) override;
    void UpdateForCollisions() override;
    void UpdateMeshes() override;
    void UpdateForOutOfMap() override;

    void Render(Entity* playerPawn) const override;

    RaycastResult Raycast(Vec3 const& startPos, Vec3 const& forwardNormal, float maxDist) const override;

    int         GetIndexFromTileCoords(IntVec2 const& tileCoords) const;
    bool        IsTileSolid(IntVec2 const& tileCoords) const;
    IntVec2     GetTileCoordsForPosition(Vec2 const& pos) const;

private:
    void AppendForNonSolidTile(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indices, MapTile const* mapTile);
    void AppendForSolidTile(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indices, MapTile const* mapTile);

    void DetectCollisionForEntities();
    void DetectCollisionForTilesAndEntities();
    void ResolveEntityCollision(Entity* entityA, Entity* entityB);
    bool ResolveForTeleporterCollision(Entity* entityA, Entity* entityB);
    bool ResolveForBulletCollision(Entity* entityA, Entity* entityB);
    void TeleportEntity(Entity* entity, Vec2 const& destPos, float destYawOffset);

    void RenderEntities(Entity* playerPawn) const;
    void RenderForDebug(Entity* playerPawn) const;
    void RenderForHealth(Entity* playerPawn) const;

    RaycastResult RaycastForTiles(Vec3 const& startPos, Vec3 const& forwardNormal, float maxDist) const;
    RaycastResult RaycastForCeilingFloor(Vec3 const& startPos, Vec3 const& forwardNormal, float maxDist) const;

private:
    IntVec2 m_mapSize;
    std::vector<MapTile*> m_tiles;
};