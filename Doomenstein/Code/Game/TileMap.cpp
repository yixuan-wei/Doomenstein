#include "Game/TileMap.hpp"
#include "Game/GameCommon.hpp"
#include "Game/MapTile.hpp"
#include "Game/MapRegion.hpp"
#include "Game/Entity.hpp"
#include "Game/Portal.hpp"
#include "Game/World.hpp"
#include "Game/Game.hpp"
#include "Game/Server.hpp"
#include "Game/Actor.hpp"
#include "Game/AuthoritativeServer.hpp"
#include "Game/EntityDefinition.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/GPUMesh.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Core/MeshUtils.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"

//For TEST
#include "Engine/Core/ErrorWarningAssert.hpp"

//////////////////////////////////////////////////////////////////////////
// Static Definitions
struct sLegend
{
    char glyph;
    MapRegionType* type = nullptr;
};

//////////////////////////////////////////////////////////////////////////
static MapRegionType* GetRegionTypeFromGlyphInArray(char glyph, std::vector<sLegend> const& legendArray)
{
    for (auto iter = legendArray.begin(); iter != legendArray.end(); iter++) {
        if (iter->glyph == glyph) {
            return iter->type;
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void TileMap::AppendForNonSolidTile(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indices, MapTile const* mapTile)
{
    IntVec2 tileCoords = mapTile->m_tileCoords;
    //floor based
    Vec2 bottomLeft((float)tileCoords.x, (float)tileCoords.y);
    Vec2 bottomRight = bottomLeft + Vec2(1.f, 0.f);
    Vec2 topRight = bottomRight + Vec2(0.f, 1.f);
    Vec2 topLeft = bottomLeft + Vec2(0.f, 1.f);
    AABB2 floorUVs = mapTile->m_type->GetFloorUVs();
    AABB2 ceilingUVs = mapTile->m_type->GetCeilingUVs();
    AppendIndexedVertexesForQuaterPolygon2D(verts, indices, Vec3(bottomLeft), Vec3(bottomRight), Vec3(topRight), Vec3(topLeft), Rgba8::WHITE, floorUVs.mins, floorUVs.maxs);                //floor
    AppendIndexedVertexesForQuaterPolygon2D(verts, indices, Vec3(topLeft,1.f), Vec3(topRight,1.f), Vec3(bottomRight,1.f), Vec3(bottomLeft,1.f), Rgba8::WHITE, ceilingUVs.mins, ceilingUVs.maxs);//ceiling
}

//////////////////////////////////////////////////////////////////////////
void TileMap::AppendForSolidTile(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indices, MapTile const* mapTile)
{
    IntVec2 tileCoords = mapTile->m_tileCoords;    

    //floor based
    Vec2 bottomLeft((float)tileCoords.x, (float)tileCoords.y);
    Vec2 bottomRight = bottomLeft + Vec2(1.f, 0.f);
    Vec2 topRight = bottomRight + Vec2(0.f, 1.f);
    Vec2 topLeft = bottomLeft + Vec2(0.f, 1.f);

    AABB2 sideUVs = mapTile->m_type->GetSideUVs();

    IntVec2 eastCoords = tileCoords + IntVec2(0, -1);
    IntVec2 westCoords = tileCoords + IntVec2(0, 1);
    IntVec2 northCoords = tileCoords + IntVec2(1, 0);
    IntVec2 southCoords = tileCoords + IntVec2(-1, 0);

    if(!IsTileSolid(eastCoords)){
        AppendIndexedVertexesForQuaterPolygon2D(verts, indices, Vec3(bottomLeft),   Vec3(bottomRight),  Vec3(bottomRight,1.f),  Vec3(bottomLeft,1.f),   Rgba8::WHITE, sideUVs.mins, sideUVs.maxs);  //east
    }
    if(!IsTileSolid(westCoords)){
        AppendIndexedVertexesForQuaterPolygon2D(verts, indices, Vec3(topRight),     Vec3(topLeft),      Vec3(topLeft,1.f),      Vec3(topRight,1.f),     Rgba8::WHITE, sideUVs.mins, sideUVs.maxs);    //west
    }
    if(!IsTileSolid(southCoords)){
        AppendIndexedVertexesForQuaterPolygon2D(verts, indices, Vec3(topLeft),      Vec3(bottomLeft),   Vec3(bottomLeft,1.f),   Vec3(topLeft,1.f),      Rgba8::WHITE, sideUVs.mins, sideUVs.maxs);     //south
    }
    if(!IsTileSolid(northCoords)){
        AppendIndexedVertexesForQuaterPolygon2D(verts, indices, Vec3(bottomRight),  Vec3(topRight),     Vec3(topRight,1.f),     Vec3(bottomRight,1.f),  Rgba8::WHITE, sideUVs.mins, sideUVs.maxs); //north
    }
}

//////////////////////////////////////////////////////////////////////////
void TileMap::DetectCollisionForEntities()
{
    for (size_t i = 0; i < m_entities.size(); i++)    {
        Entity* entityA = m_entities[i];
        if (entityA == nullptr || entityA->IsGarbage()) {
            continue;
        }

        for (size_t j = i + 1; j < m_entities.size(); j++) {
            Entity* entityB = m_entities[j];
            if (entityB == nullptr || entityB->IsGarbage()) {
                continue;
            }
            
            ResolveEntityCollision(entityA, entityB);            
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void TileMap::DetectCollisionForTilesAndEntities()
{
    for (size_t i = 0; i < m_entities.size(); i++) {
        Entity* entity = m_entities[i];
        if (entity == nullptr || !entity->CanPushedByWalls() || entity->IsGarbage()) {
            continue;
        }

        IntVec2 searchOrder[8] = {
            IntVec2(1,0), IntVec2(0,1),  IntVec2(-1,0),  IntVec2(0,-1),
            IntVec2(1,1), IntVec2(-1,1), IntVec2(-1,-1), IntVec2(1,-1)
        };
        float radius = entity->GetEntityRadius();
        Vec2 entityPos = entity->GetEntityPosition2D();
        Vec2 prevPos = entityPos;
        IntVec2 posCoords = GetTileCoordsForPosition(entityPos);
        for (int j = 0; j < 8; j++) {
            IntVec2 tileCoords = posCoords + searchOrder[j];
            if (IsTileSolid(tileCoords)) {
                Vec2 coords((float)tileCoords.x, (float)tileCoords.y);
                AABB2 tileBounds(coords, coords+Vec2(1.f,1.f));
                PushDiscOutOfAABB2D(entityPos, radius, tileBounds);
            }
        }
        if (prevPos != entityPos && entity->GetEntityType()==ENTITY_PROJECTILE
            && g_theServer->m_isAuthoritative) {
            ((AuthoritativeServer*)g_theServer)->DeleteEntity(entity);
        }
        entity->SetPosition(entityPos);
    }
}

//////////////////////////////////////////////////////////////////////////
void TileMap::ResolveEntityCollision(Entity* entityA, Entity* entityB)
{
    if (!entityA->CanPushedByEntities() && !entityB->CanPushedByEntities()) {   //both not pushed
        return;
    }

    if (!entityA->CanPushEntities() && !entityB->CanPushEntities()) {   //both not push
        return;
    }

    Vec2 posA = entityA->GetEntityPosition2D();
    Vec2 posB = entityB->GetEntityPosition2D();
    float radiusA = entityA->GetEntityRadius();
    float radiusB = entityB->GetEntityRadius();
    Vec2 aToB = posB-posA;
    float posDistSqrd = aToB.GetLengthSquared();
    float radiusSum = radiusA+radiusB;
    if (posDistSqrd>=radiusSum*radiusSum) {  //not overlap
        return;
    }

    if (ResolveForBulletCollision(entityA, entityB)) {  //bullet
        return;
    }
    if (ResolveForTeleporterCollision(entityA, entityB)) {  //teleport
        return;
    }

    float posDist = aToB.GetLength();
    aToB.Normalize();
    Vec2 deltaMoveB = (radiusSum-posDist)*aToB;
    if (entityA->CanPushedByEntities() && entityB->CanPushedByEntities()) { //pushes each other out
        deltaMoveB *= .5f;
        entityA->Translate(-deltaMoveB);
        entityB->Translate(deltaMoveB);
    }
    else if (entityA->CanPushedByEntities() && entityB->CanPushEntities()) {    //push a out of b
        entityA->Translate(-deltaMoveB);
    }
    else if (entityA->CanPushEntities() && entityB->CanPushedByEntities()) {    //push b out of a
        entityB->Translate(deltaMoveB);
    }
}

//////////////////////////////////////////////////////////////////////////
bool TileMap::ResolveForTeleporterCollision(Entity* entityA, Entity* entityB)
{
    eEntityType typeA = entityA->GetEntityType();
    eEntityType typeB = entityB->GetEntityType();
    if ((typeA != ENTITY_PORTAL && typeB != ENTITY_PORTAL) ||
        (typeA==ENTITY_PORTAL && typeB==ENTITY_PORTAL)) {
        return false;
    }

    Portal* portal = nullptr;
    Entity* playerPawn = nullptr;
    if (typeA == ENTITY_PORTAL) {
        portal = (Portal*)entityA;
        playerPawn = entityB;
    }
    else {
        portal = (Portal*)entityB;
        playerPawn = entityA;
    }

    if (!g_theServer->IsPawnAPlayer(playerPawn)) {
        if (portal->m_destMap.empty() || portal->m_destMap == m_name) {
            TeleportEntity(playerPawn, portal->m_destPos, portal->m_destYawOffset);
            return true;
        }
        else{
            return false;
        }
    }
        
    TeleportEntity(playerPawn, portal->m_destPos, portal->m_destYawOffset);
    if(!portal->m_destMap.empty() && portal->m_destMap != m_name){
        return m_theWorld->SwitchMap(playerPawn,portal->m_destMap, portal->m_destPos,
            playerPawn->GetEntityPitchYawRollDegrees().y);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool TileMap::ResolveForBulletCollision(Entity* entityA, Entity* entityB)
{
    eEntityType typeA = entityA->GetEntityType();
    eEntityType typeB = entityB->GetEntityType();
    if (typeA != eEntityType::ENTITY_PROJECTILE && typeB != eEntityType::ENTITY_PROJECTILE) {
        return false;
    }

    //check bullet height overlap
    FloatRange heightA = entityA->GetEntityHeightRange();
    FloatRange heightB = entityB->GetEntityHeightRange();
    if (!heightA.DoesOverlap(heightB)) {
        return true;
    }

    AuthoritativeServer* server = (AuthoritativeServer*)g_theServer;
    if (typeA == eEntityType::ENTITY_PROJECTILE && typeB == eEntityType::ENTITY_PROJECTILE) {
        server->DeleteEntity(entityA);
        server->DeleteEntity(entityB);
        return true;
    }

    Entity* otherEntity = nullptr;
    Projectile* proje = nullptr;
    if (typeA == eEntityType::ENTITY_PROJECTILE) {
        proje = (Projectile*)entityA;
        otherEntity = entityB;
    }
    else {  //entityB is projectile
        proje = (Projectile*)entityB;
        otherEntity = entityA;
    }

    otherEntity->OnProjectileHit(proje);
    server->DeleteEntity((Entity*)proje);
    return true;
}

//////////////////////////////////////////////////////////////////////////
void TileMap::TeleportEntity(Entity* entity, Vec2 const& destPos, float destYawOffset)
{
    entity->SetPosition(destPos);
    entity->RotateDeltaPitchYawRollDegrees(Vec3(0.f, destYawOffset, 0.f));

    g_theGame->PlayTeleportSound();
}

//////////////////////////////////////////////////////////////////////////
void TileMap::RenderEntities(Entity* playerPawn) const
{
    for (Entity* e : m_entities) {
        if (e != nullptr && !e->IsGarbage()) {
            e->Render(playerPawn);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void TileMap::RenderForDebug(Entity* playerPawn) const
{
    if (!g_debugDrawing) {
        return;
    }

    std::vector<Vertex_PCU> verts;
    std::vector<unsigned int> inds;
    for (size_t i = 0; i < m_entities.size(); i++) {
        Entity* entity = m_entities[i];
        if (entity == nullptr || entity->IsGarbage()) {
            continue;
        }

        float radius = entity->GetEntityRadius();
        Transform trans;
        Vec3 entityPos = entity->GetEntityPosition();
        trans.SetPosition(entityPos);
        float height = entity->GetEntityHeight();
        trans.SetScale(Vec3(radius,radius, height));
        DebugDrawEntity(trans.ToMatrix(AXIS__YZ_X));

        Vec3 eyePos = entity->GetEntityEyePosition();
        Vec3 forward = entity->GetEntityForward();
        forward *= .5f;
        Rgba8 transYellow(255,255,0,0);
        DebugAddWorldLine(eyePos, Rgba8::YELLOW, Rgba8::YELLOW, eyePos+forward, transYellow, transYellow);

        Vec3 up, left;
        GetBillboardDirsFromCamAndMethod(playerPawn->GetEntityEyePosition(), playerPawn->GetEntityForward(), entityPos, entity->GetBillboardMode(), up, left);
        Vec2 size = entity->GetSpriteSize();
        Vec3 halfLeft = size.x*.5f*left;
        Vec3 upVec = size.y*up;
        Vec3 bottomLeft = entityPos-halfLeft;
        Vec3 bottomRight = entityPos+halfLeft;
        AppendIndexedVertexesForQuaterPolygon2D(verts, inds, bottomLeft, bottomRight, bottomRight+upVec, bottomLeft+upVec);
    }
    GPUMesh* mesh = new GPUMesh(g_theRenderer);
    mesh->UpdateIndices((unsigned int)inds.size(), &inds[0]);
    mesh->UpdateVertices((unsigned int)verts.size(), &verts[0]);
    g_theRenderer->BindShaderStateByName("data/shaders/states/uvs.shaderstate");
    g_theRenderer->DrawMesh(mesh);
    delete mesh;
}

//////////////////////////////////////////////////////////////////////////
void TileMap::RenderForHealth(Entity* playerPawn) const
{
    std::vector<Vertex_PCU> verts;
    for (Entity* e : m_entities) {
        if (e && !e->IsGarbage() && e != playerPawn && e->GetEntityType()==ENTITY_ACTOR) {
            Vec2 pos = e->GetEntityPosition2D();
            Vec3 headPos(pos, e->GetEntityHeight());
            std::string health = Stringf("%.0f",((Actor*)e)->GetHealth());
            Vec3 up, left;
            GetBillboardDirsFromCamAndMethod(playerPawn->GetEntityEyePosition(), playerPawn->GetEntityForward(), 
                headPos, eBillboardMode::CAM_OPPOSING_XYZ, up, left);
            g_theFont->AddVertsForText3D(verts, headPos, up,left,.1f,
                health.c_str());
        }
    }
    g_theRenderer->BindDiffuseTexture(g_theFont->GetTexture());
    g_theRenderer->DrawVertexArray(verts);
}

//////////////////////////////////////////////////////////////////////////
RaycastResult TileMap::RaycastForTiles(Vec3 const& startPos, Vec3 const& forwardNormal, float maxDist) const
{
    RaycastResult result;
    result.impactDistance = maxDist;
    int tileX = (int)startPos.x;
    int tileY = (int)startPos.y;
    if (IsTileSolid(IntVec2(tileX, tileY))) {
        return result;
    }

    Vec3 rayDisplacement = maxDist*forwardNormal;
    //init x
    float xDeltaT = forwardNormal.x==0.f? FLT_MAX : 1.f/AbsFloat(forwardNormal.x);
    int tileStepX = forwardNormal.x>0.f? 1 : -1;
    int offsetToLeadingEdgeX = (tileStepX+1)/2;
    float firstVerticalIntersectX = (float)(tileX + offsetToLeadingEdgeX);
    float tOfNextXCrossing = AbsFloat(firstVerticalIntersectX-startPos.x) * xDeltaT;
    //init y
    float yDeltaT = forwardNormal.y==0.f?FLT_MAX : 1.f/AbsFloat(forwardNormal.y);
    int tileStepY = forwardNormal.y>0.f?1:-1;
    int offsetToLeadingEdgeY = (tileStepY+1)/2;
    float firstHorizontalIntersectY = (float)(tileY+offsetToLeadingEdgeY);
    float tOfNextYCrossing = AbsFloat(firstHorizontalIntersectY-startPos.y)*yDeltaT;

    while (tOfNextXCrossing <= maxDist || tOfNextYCrossing <= maxDist) {
        if (tOfNextXCrossing < tOfNextYCrossing) {  //cross x
            if(tOfNextXCrossing>maxDist){   //no impact
                return result;
            }

            tileX+=tileStepX;
            if (IsTileSolid(IntVec2(tileX, tileY))) {
                result.didImpact = true;
                result.impactDistance = tOfNextXCrossing;
                result.impactPosition = startPos + result.impactDistance*forwardNormal;
                result.impactSurfaceNormal = Vec3(-(float)tileStepX, 0.f,0.f);
                return result;
            }
            else {
                tOfNextXCrossing += xDeltaT;
            }
        }
        else {  //cross y
            if (tOfNextYCrossing > maxDist) {   //no impact
                return result;
            }

            tileY += tileStepY;
            if (IsTileSolid(IntVec2(tileX, tileY))) {
                result.didImpact = true;
                result.impactDistance = tOfNextYCrossing;
                result.impactPosition = startPos+result.impactDistance*forwardNormal;
                result.impactSurfaceNormal = Vec3(0.f, -(float)tileStepY, 0.f);
                return result;
            }
            else{
                tOfNextYCrossing += yDeltaT;
            }
        }
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////
RaycastResult TileMap::RaycastForCeilingFloor(Vec3 const& startPos, Vec3 const& forwardNormal, float maxDist) const
{
    RaycastResult result;
    float direction = 1.f;
    if (forwardNormal.z > 0.f) {    //test for ceiling
        result.impactDistance = (1.f-startPos.z)/forwardNormal.z;
        direction = -1.f;
    }
    else if(forwardNormal.z<0.f){  //test for floor
        result.impactDistance = startPos.z/(-forwardNormal.z);
    }
    if (result.impactDistance <= maxDist && result.impactDistance > 0.f) {
        result.didImpact = true;
        result.impactSurfaceNormal = Vec3(0.f, 0.f, direction);
        result.impactPosition = startPos + result.impactDistance * forwardNormal;
    }
    else {
        result.impactDistance = maxDist;
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////
TileMap::TileMap(XmlElement const& root)
{
    m_mapSize = ParseXmlAttribute(root, "dimensions", IntVec2(0, 0));
    if (m_mapSize.x < 1 || m_mapSize.y < 1) {
        g_theConsole->PrintError(Stringf("Map %s has invalid dimensions (%i, %i)", m_mapSize.x, m_mapSize.y));
        return;
    }

    //Building legends
    std::vector<sLegend> legends;
    XmlElement const* legendElement = root.FirstChildElement();
    if (legendElement == nullptr) {
        g_theConsole->PrintError("Map has no Legend element");
        return;
    }
    if (strcmp(legendElement->Name(), "Legend") != 0) {
        g_theConsole->PrintError("Legend is not first child element, check format");
        return;
    }

    XmlElement const* tileElement = legendElement->FirstChildElement();
    if (tileElement == nullptr) {
        g_theConsole->PrintError("Map Legend has no Tile element");
        return;
    }
    while (tileElement != nullptr && strcmp(tileElement->Name(), "Tile")==0) {
        char glyph = ParseXmlAttribute(*tileElement, "glyph", '\0');
        if (glyph == '\0') {
            g_theConsole->PrintError("Map Legend has invalid glyph");
            return;
        }

        std::string regionTypeName = ParseXmlAttribute(*tileElement, "regionType", "");
        MapRegionType* regionType = MapRegionType::GetMapRegionTypeForName(regionTypeName);
        if (regionType == nullptr) {
            g_theConsole->PrintError(Stringf("Map Legend has invalid regionType %s", regionTypeName.c_str()));
            return;
        }

        sLegend newLegend;
        newLegend.glyph = glyph;
        newLegend.type = regionType;
        legends.push_back(newLegend);

        tileElement = tileElement->NextSiblingElement();
    }
    if (tileElement != nullptr) {
        g_theConsole->PrintError(Stringf("Legend has unrecognized element %s", tileElement->Name()));
    }

    //building Map
    XmlElement const* mapRowsElement = legendElement->NextSiblingElement();
    if (mapRowsElement == nullptr) {
        g_theConsole->PrintError("Map has no MapRows element");
        return;
    }
    if (strcmp(mapRowsElement->Name(), "MapRows") != 0) {
        g_theConsole->PrintError(Stringf("MapRows is not second element, %s is", mapRowsElement->Name()));
        return;
    }

    XmlElement const* mapRowElement = mapRowsElement->FirstChildElement();
    if (mapRowElement == nullptr) {
        g_theConsole->PrintError("Map MapRows has no MapRow element");
        return;
    }
    int rowIndex = m_mapSize.y - 1;
    while (mapRowElement != nullptr && strcmp(mapRowElement->Name(), "MapRow")==0) {
        if (rowIndex < 0) {
            g_theConsole->PrintError(Stringf("Map has non-compatible number of rows with dimension (%i, %i)", m_mapSize.x, m_mapSize.y));
            return;
        }

        std::string row = ParseXmlAttribute(*mapRowElement, "tiles", "");
        if ((int)row.size() != m_mapSize.x) {
            g_theConsole->PrintError(Stringf("Map MapRow has invalid tiles, non-compatible with dimensions (%i, %i)",
                m_mapSize.x, m_mapSize.y));
            return;
        }

        for (int i = 0; i < m_mapSize.x; i++) {
            char glyph = row[i];
            MapRegionType* type = GetRegionTypeFromGlyphInArray(glyph, legends);
            if (type == nullptr) {
                g_theConsole->PrintError(Stringf("Map MapRow %s has unrecognized glyph '%c'", row.c_str(), glyph));
                return;
            }

            MapTile* newTile = new MapTile(IntVec2(i, rowIndex), type);
            m_tiles.insert(m_tiles.begin() + i, newTile);
        }

        rowIndex--;
        mapRowElement = mapRowElement->NextSiblingElement();
    }
    if (rowIndex != -1) {
        g_theConsole->PrintError(Stringf("Map has non-compatible number of rows with dimension (%i, %i)", m_mapSize.x, m_mapSize.y));
        return;
    }
    if (mapRowElement != nullptr) {
        g_theConsole->PrintError(Stringf("MapRows has unrecognized element %s", mapRowElement->Name()));
    }

    XmlElement const* entityElement = mapRowsElement->NextSiblingElement();
    if (entityElement == nullptr) {
        g_theConsole->PrintError("Map has no Entities element");
    }
    else{
        //parse entity element
        if (strcmp(entityElement->Name(), "Entities")==0) {
            XmlElement const* entity = entityElement->FirstChildElement();
            while (entity != nullptr) {
                if(strcmp(entity->Name(), "PlayerStart")==0){   //player start
                    PlayerStart newStart;
                    newStart.position = ParseXmlAttribute(*entity, "pos", Vec2::ZERO);
                    newStart.yaw = ParseXmlAttribute(*entity, "yaw", 0.f);
                    m_playerStarts.push_back(newStart);
                }
                else {                     //defined in EntityTypes xml
                    std::string entityTypeName = ParseXmlAttribute(*entity, "type", "");
                    EntityDef* newDef = EntityDef::GetEntityDefinitionFromName(entityTypeName);
                    if (newDef == nullptr) {
                        g_theConsole->PrintError(Stringf("Fail to find entity definition of %s", entityTypeName.c_str()));
                    }
                    else{
                        if (newDef->m_type != GetEntityTypeFromText(entity->Name())) {
                            g_theConsole->PrintError(Stringf("Entity %s of type %s not consistent with EntityTypes definition",
                                entityTypeName.c_str(), entity->Name()));
                        }
                        else{
                            Entity* newEntity = SpawnNewEntityOfType(*newDef);
                            if (newEntity != nullptr) {
                                newEntity->SetPosition(ParseXmlAttribute(*entity, "pos", Vec2::ZERO));
                                float yawDegrees= ParseXmlAttribute(*entity, "yaw", 0.f);
                                newEntity->SetPitchYawRollDegrees(Vec3(0.f,yawDegrees,0.f));
                                //TODO add more preset entity settings
                                if (newEntity->GetEntityType() == ENTITY_PORTAL) {
                                    Portal* newPortal = (Portal*)newEntity;
                                    newPortal->m_destMap = ParseXmlAttribute(*entity, "destMap","");
                                    newPortal->m_destPos = ParseXmlAttribute(*entity, "destPos", Vec2::ZERO);
                                    newPortal->m_destYawOffset = ParseXmlAttribute(*entity, "destYawOffset", 0.f);
                                }
                            }
                        }
                    }
                }

                entity = entity->NextSiblingElement();
            }
        }

        entityElement = entityElement->NextSiblingElement();
        if (entityElement != nullptr) {
            g_theConsole->PrintError(Stringf("Map has unrecognized element %s", entityElement->Name()));
        }
    }    

    m_isValid = true;
}

//////////////////////////////////////////////////////////////////////////
TileMap::~TileMap()
{
    delete m_mesh;
    for (size_t i = 0; i < m_tiles.size(); i++) {
        delete m_tiles[i];
    }
    m_tiles.clear();
}

//////////////////////////////////////////////////////////////////////////
void TileMap::UpdateForEntities()
{
    for (Entity* e : m_entities) {
        if(e!=nullptr && !e->IsGarbage()){
            e->Update();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void TileMap::UpdateForEntitiesLocal(float deltaSeconds)
{
    for (Entity* e : m_entities) {
        if (e != nullptr && !e->IsGarbage()) {
            e->UpdateLocal(deltaSeconds);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void TileMap::UpdateForCollisions()
{
    DetectCollisionForEntities();
    DetectCollisionForTilesAndEntities();
}

//////////////////////////////////////////////////////////////////////////
void TileMap::UpdateMeshes()
{
    std::vector<Vertex_PCU> vertexes;
    std::vector<unsigned int> indices;
    size_t maxSize = 3*4*(size_t)m_mapSize.x*(size_t)m_mapSize.y;
    vertexes.reserve(maxSize);
    indices.reserve(maxSize);
    
    int totalNum = m_mapSize.x*m_mapSize.y;
    for (int i = 0; i < totalNum; i++) {
        MapTile* tile = m_tiles[i];
        if (tile->m_type->IsTypeSolid()) {
            AppendForSolidTile(vertexes,indices, tile);
        }
        else {
            AppendForNonSolidTile(vertexes,indices,tile);
        }
    }

    m_mesh->UpdateIndices((unsigned int)indices.size(),&indices[0]);
    m_mesh->UpdateVertices((unsigned int)vertexes.size(), &vertexes[0]);
}

//////////////////////////////////////////////////////////////////////////
void TileMap::UpdateForOutOfMap()
{
    FloatRange mapRange(0.f,1.f);
    AuthoritativeServer* server = (AuthoritativeServer*)g_theServer;
    for (Entity* e : m_entities) {
        if (e != nullptr && !e->IsGarbage()) {
            FloatRange range = e->GetEntityHeightRange();
            if (!range.DoesOverlap(mapRange)) {
                server->DeleteEntity(e);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void TileMap::Render(Entity* playerPawn) const
{   
    g_theRenderer->BindShaderState(nullptr);
    TODO("Assume just one tile texture");
    g_theRenderer->BindDiffuseTexture(m_tiles[0]->m_type->GetTexture());
    g_theRenderer->SetModelMatrix(Mat44::IDENTITY);
    g_theRenderer->DrawMesh(m_mesh);

    RenderEntities(playerPawn);
    RenderForHealth(playerPawn);
    RenderForDebug(playerPawn);
}

//////////////////////////////////////////////////////////////////////////
RaycastResult TileMap::Raycast(Vec3 const& startPos, Vec3 const& forwardNormal, float maxDist) const
{
    RaycastResult entityResult = Map::RaycastForEntities(startPos,forwardNormal,maxDist);
    RaycastResult upDownResult = RaycastForCeilingFloor(startPos,forwardNormal,maxDist);
    RaycastResult tileResult = RaycastForTiles(startPos, forwardNormal, maxDist);
    if (upDownResult.impactDistance < tileResult.impactDistance) {
        if (entityResult.impactDistance < upDownResult.impactDistance) {
            return entityResult;
        }
        else return upDownResult;
    }
    else {
        if (entityResult.impactDistance < tileResult.impactDistance) {
            return entityResult;
        }
        else return tileResult;
    }
}

//////////////////////////////////////////////////////////////////////////
int TileMap::GetIndexFromTileCoords(IntVec2 const& tileCoords) const
{
    return tileCoords.y*m_mapSize.x + tileCoords.x;
}

//////////////////////////////////////////////////////////////////////////
// Out-boundary and solid tile
bool TileMap::IsTileSolid(IntVec2 const& tileCoords) const  
{
    if (tileCoords.x < 0 || tileCoords.x >= m_mapSize.x || tileCoords.y < 0 || tileCoords.y >= m_mapSize.y) {
        return true;
    }

    MapTile* tile = m_tiles[GetIndexFromTileCoords(tileCoords)];
    return tile->m_type->IsTypeSolid();
}

//////////////////////////////////////////////////////////////////////////
IntVec2 TileMap::GetTileCoordsForPosition(Vec2 const& pos) const
{
    return IntVec2(RoundDownToInt(pos.x), RoundDownToInt(pos.y));
}

