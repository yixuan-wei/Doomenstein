#include "Game/Entity.hpp"
#include "Game/GameCommon.hpp"
#include "Game/NetworkObserver.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/AuthoritativeServer.hpp"
#include "Game/Server.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/GPUMesh.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Core/MeshUtils.hpp"
#include "Engine/Core/DevConsole.hpp"

//////////////////////////////////////////////////////////////////////////
Entity::Entity(Map* map, EntityDef const* definition)
    : m_entityDef(definition)
    , m_theMap(map)
{
    
}

//////////////////////////////////////////////////////////////////////////
void Entity::Update()
{
    UpdateRaycast();
}

//////////////////////////////////////////////////////////////////////////
void Entity::UpdateLocal(float deltaSeconds)
{
    //calculate forward
    Mat44 rotationMat = Mat44::FromRotationDegrees(Vec3(m_rollDegrees, m_pitchDegrees, m_yawDegrees), AXIS__YZ_X);
    m_forward = rotationMat.TransformVector3D(Vec3(1.f, 0.f, 0.f));

    //update anim timer
    m_animTimer += deltaSeconds;
    m_timerAI += deltaSeconds;
    if (m_timerAI > 2.f) {
        m_timerAI-=2.f;
    }
}

//////////////////////////////////////////////////////////////////////////
void Entity::Render(Entity* playerPawn) const
{
    RenderForPosition(playerPawn->GetEntityEyePosition(), playerPawn->GetEntityForward(), m_position);
}

//////////////////////////////////////////////////////////////////////////
FloatRange Entity::GetEntityHeightRange() const
{
    return FloatRange(0.f, m_entityDef->m_height);
}

//////////////////////////////////////////////////////////////////////////
void Entity::MarkAsGarbage()
{
    m_isGarbage = true;
}

//////////////////////////////////////////////////////////////////////////
void Entity::SetIndex(int idx)
{
    m_index = idx;
}

//////////////////////////////////////////////////////////////////////////
void Entity::UpdateMap(Map* newMap)
{
    m_theMap->RemoveEntity(this);
    m_theMap=newMap;
    g_theConsole->PrintString(Rgba8::WHITE,Stringf("Entity idx %i to map %s", m_index, newMap->GetName().c_str()));

    if (g_theServer->m_isAuthoritative) {
        ((AuthoritativeServer*)g_theServer)->AddTeleportMessageToClients(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void Entity::Translate(Vec2 const& translation)
{
    m_position+=translation;
    if(translation!=Vec2::ZERO){
        g_theObserver->AddEntityTransformUpdate(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void Entity::SetPosition(Vec2 const& newPos)
{
    if (newPos != m_position) {
        g_theObserver->AddEntityTransformUpdate(this);
    }
    m_position = newPos;
}

//////////////////////////////////////////////////////////////////////////
void Entity::SetPitchYawRollDegrees(Vec3 const& pitchYawRoll)
{
    float pitch = Clamp(AbsFloat(pitchYawRoll.x), 0.f, 89.9f);
    m_pitchDegrees = GetSmallestSameDegrees(SignFloat(pitchYawRoll.x)*pitch);
    m_yawDegrees = GetSmallestSameDegrees(pitchYawRoll.y);
    m_rollDegrees = GetSmallestSameDegrees(pitchYawRoll.z);
    g_theObserver->AddEntityTransformUpdate(this);
}

//////////////////////////////////////////////////////////////////////////
void Entity::RotateDeltaPitchYawRollDegrees(Vec3 const& deltaDegrees)
{
    if(deltaDegrees!=Vec3::ZERO){
        SetPitchYawRollDegrees(Vec3(m_pitchDegrees, m_yawDegrees, m_rollDegrees)+deltaDegrees);
    }
}

//////////////////////////////////////////////////////////////////////////
void Entity::SetIsControlledByAI(bool isAIControlled)
{
    m_isControlledByAI = isAIControlled;
}

//////////////////////////////////////////////////////////////////////////
bool Entity::SetAnimationName(char const* animName)
{
    m_animName = animName;
    m_animTimer = 0.f;
    return m_entityDef->IsAnimNameExist(animName);
}

//////////////////////////////////////////////////////////////////////////
eBillboardMode Entity::GetBillboardMode() const
{
    return m_entityDef->m_billboardMode;
}

//////////////////////////////////////////////////////////////////////////
eEntityType Entity::GetEntityType() const
{
    return m_entityDef->m_type;
}

//////////////////////////////////////////////////////////////////////////
bool Entity::CanPushedByEntities() const
{
    return m_entityDef->m_canBePushedByEntities;
}

//////////////////////////////////////////////////////////////////////////
bool Entity::CanPushEntities() const
{
    return m_entityDef->m_canPushEntities;
}

//////////////////////////////////////////////////////////////////////////
bool Entity::CanPushedByWalls() const
{
    return m_entityDef->m_canBePushedByWalls;
}

//////////////////////////////////////////////////////////////////////////
float Entity::GetWalkSpeed() const
{
    return m_entityDef->m_walkSpeed;
}

//////////////////////////////////////////////////////////////////////////
float Entity::GetEntityHeight() const
{
    return m_entityDef->m_height;
}

//////////////////////////////////////////////////////////////////////////
float Entity::GetEntityRadius() const
{
    return m_entityDef->m_radius;
}

//////////////////////////////////////////////////////////////////////////
Vec2 Entity::GetSpriteSize() const
{
    return m_entityDef->m_spriteSize;
}

//////////////////////////////////////////////////////////////////////////
Vec3 Entity::GetEntityPitchYawRollDegrees() const
{
    return Vec3(m_pitchDegrees, m_yawDegrees, m_rollDegrees);
}

//////////////////////////////////////////////////////////////////////////
Vec3 Entity::GetEntityPosition() const
{
    return Vec3(m_position);
}

//////////////////////////////////////////////////////////////////////////
Vec3 Entity::GetEntityEyePosition() const
{
    return Vec3(m_position,m_entityDef->m_eyeHeight);
}

//////////////////////////////////////////////////////////////////////////
void Entity::UpdateRaycast()
{
    m_raycast = m_theMap->Raycast(GetEntityEyePosition(), GetEntityForward(), 10.f);
}

//////////////////////////////////////////////////////////////////////////
void Entity::RenderForPosition(Vec3 const& camPos, Vec3 const& camForward, Vec3 const& centerPos) const
{
    Texture const* spriteTex = m_entityDef->GetTexture();
    if (spriteTex == nullptr) {
        return;
    }

    std::vector<Vertex_PCU> verts;
    std::vector<unsigned int> inds;

    Vec3 worldDir = camPos - centerPos;
    Vec2 localDir = GetLocalVector(Vec2(worldDir.x, worldDir.y));
    Vec2 uvMins = Vec2::ZERO;
    Vec2 uvMaxs = Vec2::ONE;
    m_entityDef->GetAnimSpriteUVs(m_animName, m_animTimer, localDir, uvMins, uvMaxs);

    Vec3 up, left;
    GetBillboardDirsFromCamAndMethod(camPos, camForward, centerPos, m_entityDef->m_billboardMode, up, left);
    Vec2 size = GetSpriteSize();
    Vec3 halfLeft = size.x * .5f * left;
    Vec3 upVec = size.y * up;
    Vec3 bottomLeft = centerPos - halfLeft;
    Vec3 bottomRight = centerPos + halfLeft;
    AppendIndexedVertexesForQuaterPolygon2D(verts, inds, bottomLeft, bottomRight, bottomRight + upVec, bottomLeft + upVec,
        Rgba8::WHITE, uvMins, uvMaxs);

    GPUMesh* mesh = new GPUMesh(g_theRenderer);
    mesh->UpdateIndices((unsigned int)inds.size(), &inds[0]);
    mesh->UpdateVertices((unsigned int)verts.size(), &verts[0]);
    g_theRenderer->BindDiffuseTexture(spriteTex);
    g_theRenderer->DrawMesh(mesh);
    delete mesh;
}

//////////////////////////////////////////////////////////////////////////
Vec2 Entity::GetLocalVector(Vec2 const& worldVec) const
{
    Vec2 localI(m_forward.x, m_forward.y);
    Vec2 localJ(-m_forward.y, m_forward.x);
    return Vec2(DotProduct2D(worldVec, localI), DotProduct2D(worldVec, localJ));
}
