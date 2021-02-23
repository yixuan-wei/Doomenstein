#include "Game/GameCommon.hpp"
#include "Game/Entity.hpp"
#include "Game/Server.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Core/DevConsole.hpp"

App* g_theApp = nullptr; 
Server* g_theServer = nullptr;
Client* g_theClient = nullptr;
InputSystem* g_theInput = nullptr;
AudioSystem* g_theAudio = nullptr;
RenderContext* g_theRenderer = nullptr;
NetworkObserver* g_theObserver = nullptr;
RandomNumberGenerator* g_theRNG = nullptr;
BitmapFont* g_theFont = nullptr;
Game* g_theGame = nullptr;

bool g_debugDrawing = false;
float g_sendRatePerSec = 30.f;

//////////////////////////////////////////////////////////////////////////
void GetBillboardDirsFromCamAndMethod(Vec3 const& camPos, Vec3 const& camForward, Vec3 const& entityPos, eBillboardMode method, Vec3& up, Vec3& left)
{
    Vec3 forward;
    switch (method)    {
    case CAM_FACING_XY:    {        
        Vec2 entityToCam(camPos.x-entityPos.x, camPos.y-entityPos.y);
        forward = Vec3(entityToCam).GetNormalized();
        break;
    }
    case CAM_OPPOSING_XY:    {
        forward = Vec3(-camForward.x, -camForward.y, 0.f).GetNormalized();
        break;
    }
    case CAM_FACING_XYZ:    {
        Vec3 entityToCam = camPos-entityPos;
        forward = entityToCam.GetNormalized();
        break;
    }
    case CAM_OPPOSING_XYZ:    {
        forward = -camForward;
        break;
    }
    }

    Vec3 worldUp = GetWorldUpVector(AXIS__YZ_X);
    left = CrossProduct3D(worldUp, forward).GetNormalized();
    if (NearlyEqual(left.GetLengthSquared(), 0.f)) {
        Vec3 worldForward = GetWorldForwardVector(AXIS__YZ_X);
        left = CrossProduct3D(forward, worldForward).GetNormalized();
    }
    up = CrossProduct3D(forward, left).GetNormalized();
}

//////////////////////////////////////////////////////////////////////////
COMMAND(SwitchMap, "switch map, dest=TestMap", eEventFlag::EVENT_CONSOLE)
{
    std::string destName = args.GetValue("dest", "");
    if (destName.empty()) {
        g_theConsole->PrintError(Stringf("Map given no destination, ex. dest=TestRoom"));
        return false;
    }

    g_theServer->SwitchPlayerMap(g_theClient, destName);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool InputInfo::operator==(InputInfo const& other)
{
    return other.playerMove==playerMove && other.mouseMove==mouseMove &&isClosing==other.isClosing;
}
