#pragma once

#include "Vec2.h"
#include "Entity.h"

class Camera {
    bool m_cameraFollow = false;
    Vec2 m_cameraZoom = {1, 1};
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

    void setCameraZoom(Vec2 zoom);
    Vec2 getCameraZoom();

    bool startPan(float speed, int duration, Vec2 pos, bool pause);
    void panCamera();

    bool update(Vec2 playerPos, bool pause);

};
