#pragma once

#include "../physics/Vec2.h"
#include "../ecs/Entity.h"

struct CameraConfig
{
    int SHAKE_DURATION_SMALL, SHAKE_DURATION_MEDIUM, SHAKE_DURATION_LARGE;
    int SHAKE_INTENSITY_SMALL, SHAKE_INTENSITY_MEDIUM, SHAKE_INTENSITY_LARGE;
};

class Camera {

    bool m_cameraFollow = false;
    int m_cameraZoom = 0;
    Vec2 m_screenSize = {1920, 1080};
    Vec2 m_levelSize;
    Vec2 m_gridSize;
    
    float shakeMagnitude;
    int shakeDuration = 0;
    int shakeTimeElapsed;
    float panSpeed;
    int panTimeElapsed;
    int i;
    bool panInitPause;
    bool m_cameraPause;
    Vec2 panPos = Vec2{0,0};
    Vec2 panStartPos = Vec2{0,0};
public:
    CameraConfig config;
    int panDuration = 0;
    Camera();
    void calibrate(Vec2 screenSize, Vec2 levelSize, Vec2 gridSize);
    Vec2 position = Vec2{0,0};      // Current camera position
    Vec2 originalPosition = Vec2{0,0};  // Original camera position (before shake)
    
    void reset();

    void movement(Vec2 playerPos);

    void startShake(float magnitude, int duration);
    void screenShake();

    void toggleCameraFollow();
    bool getCameraFollow();

    int stepCameraZoom(int zoom, int windowZoom);
    int getCameraZoom();

    bool startPan(float speed, int duration, Vec2 pos, bool pause);
    void panCamera();

    bool update(Vec2 playerPos, bool pause);

};
