#pragma once

#include "Vec2.h"
#include "Entity.h"

class Camera {
    bool m_cameraFollow = false;
    Vec2 m_cameraZoom = {1, 1};
    Vec2 m_screenSize = {1920, 1080};
    Vec2 m_levelSize;
    Vec2 m_gridSize;
public:
    Camera();
    void calibrate(Vec2 screenSize, Vec2 levelSize, Vec2 gridSize);
    Vec2 position;      // Current camera position
    Vec2 originalPosition;  // Original camera position (before shake)

    // struct ScreenShakeComponent {
    // float intensity;      // Maximum displacement of the camera
    // float duration;       // Total duration of the screen shake
    // float elapsedTime;    // Time passed since the screen shake started
    // float decay;          // Time passed since the screen shake started

    // ScreenShakeComponent(float shakeIntensity, float shakeDuration)
    //     : intensity(shakeIntensity), duration(shakeDuration), elapsedTime(0) {}
    // };

    // Reset camera to the original position after shaking
    // Vec2 getShakeOffset(float intensity);
    void reset();

    void movement(Vec2 playerPos);

    void startShake(float magnitude, int duration);
    void screenShake();

    void toggleCameraFollow();
    bool getCameraFollow();

    void setCameraZoom(Vec2 zoom);
    Vec2 getCameraZoom();

    void update();
    
    float shakeMagnitude;
    int shakeDuration;
    int shakeTimeElapsed;

    void update(Vec2 playerPos);

};
