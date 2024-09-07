#include "Vec2.h"

class Camera {
public:
    Vec2 position;      // Current camera position
    Vec2 originalPosition;  // Original camera position (before shake)

    struct ScreenShakeComponent {
    float intensity;      // Maximum displacement of the camera
    float duration;       // Total duration of the screen shake
    float elapsedTime;    // Time passed since the screen shake started
    };

    ScreenShakeComponent(float shakeIntensity, float shakeDuration)
        : intensity(shakeIntensity), duration(shakeDuration), elapsedTime(0) {}


    Camera(float x, float y) : position(x, y), originalPosition(x, y) {}

    // Reset camera to the original position after shaking
    void reset();
    Vec2 Camera::getShakeOffset(float intensity);

    void Camera::update(float deltaTime, ScreenShakeComponent* screenShake);

};
