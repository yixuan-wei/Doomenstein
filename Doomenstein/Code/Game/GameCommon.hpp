#pragma once
#include "Engine/Core/EngineCommon.hpp"

class App;
class Server;
class Client;
class InputSystem;
class AudioSystem;
class RenderContext;
class NetworkObserver;
class BitmapFont;
class RandomNumberGenerator;
class Game;
struct Vec3;

constexpr float LINE_THICKNESS = .05f;
constexpr float CAMERA_MOVE_SPEED = 2.f;
constexpr float CAMERA_ROTATE_SPEED = 1000.f;
constexpr float SEND_RATE_CHANGE_RATE = 5.f;

extern App* g_theApp;
extern Server* g_theServer;
extern Client* g_theClient;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern RenderContext* g_theRenderer;
extern NetworkObserver* g_theObserver;
extern RandomNumberGenerator* g_theRNG;
extern BitmapFont* g_theFont;
extern Game* g_theGame;

extern bool g_debugDrawing;
extern float g_sendRatePerSec;

//////////////////////////////////////////////////////////////////////////
enum eBillboardMode
{
    CAM_FACING_XY,
    CAM_OPPOSING_XY,
    CAM_FACING_XYZ,
    CAM_OPPOSING_XYZ
};

void GetBillboardDirsFromCamAndMethod(Vec3 const& camPos, Vec3 const& camForward, Vec3 const& entityPos, eBillboardMode method, Vec3& up, Vec3& left);

//////////////////////////////////////////////////////////////////////////
struct InputInfo
{
    Vec2 playerMove;
    Vec2 mouseMove;

    bool isFiring = false;
    bool isClosing = false;

    bool operator==(InputInfo const& other);
};