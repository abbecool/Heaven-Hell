#pragma once

#include "Vec2.h"
#include "Entity.h"
#include "Scene_Play.h"

class Camera {
    bool m_cameraFollow;
    Vec2 m_cameraZoom;
public:
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

    Vec2 movement(std::shared_ptr<Entity> player, Scene_Play scene);

    void toggleCameraFollow();
    bool getCameraFollow();

    void setCameraZoom(Vec2 zoom);
    Vec2 getCameraZoom();

    void update();
    // void update(float deltaTime, ScreenShakeComponent* screenShake);

};
