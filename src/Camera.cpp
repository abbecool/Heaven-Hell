#include "Camera.h"
#include "Vec2.h"
#include <random>

// Random offset generation for screen shake
Vec2 Camera::getShakeOffset(float intensity) {
    static std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
    
    float offsetX = distribution(generator) * intensity;
    float offsetY = distribution(generator) * intensity;
    
    return Vec2(offsetX, offsetY);
}

void Camera::reset() {
        position = originalPosition;
    }

// Update function to apply screen shake
void Camera::update(float deltaTime, ScreenShakeComponent* screenShake) {
    if (screenShake && screenShake->duration > 0) {
        Vec2 shakeOffset = getShakeOffset(screenShake->intensity);
        
        // Apply the shake offset to the camera position
        Vec2 originalPosition = position;
        position = originalPosition + shakeOffset;

        // Decay the shake intensity over time
        screenShake->intensity -= screenShake->decay * deltaTime;
        screenShake->duration -= deltaTime;

        // Ensure intensity doesn't go negative
        if (screenShake->intensity < 0) {
            screenShake->intensity = 0;
        }

        // Reset the camera position after the shake effect
        if (screenShake->duration <= 0) {
            position = originalPosition;  // Return camera to its original position
        }
    }
}
