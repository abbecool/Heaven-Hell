#include "Camera.h"
#include "Vec2.h"

#include <iostream>
#include <random>
#include <cmath>

// Random offset generation for screen shake
// Vec2 Camera::getShakeOffset(float intensity) {
//     static std::default_random_engine generator;
//     std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
    
//     float offsetX = distribution(generator) * intensity;
//     float offsetY = distribution(generator) * intensity;
    
//     return Vec2(offsetX, offsetY);
// }
Camera::Camera(){}

void Camera::calibrate(Vec2 screenSize, Vec2 levelSize, Vec2 gridSize){
    m_screenSize = screenSize;
    m_levelSize = levelSize;
    m_gridSize = gridSize;
}

void Camera::reset() {
        position = originalPosition;
    }

void Camera::update() {
    
}

void Camera::movement(Vec2 playerPos){
    auto width = m_screenSize.x;
    auto height = m_screenSize.y;
    auto levelX = m_levelSize.x;
    auto levelY = m_levelSize.y;
    auto gridX = m_gridSize.x;
    auto gridY = m_gridSize.y;

    if (m_cameraFollow){
        position = playerPos - Vec2(width / 2, height / 2);
        if (position.x + (float)width > gridX*levelX){ position.x = gridX*levelX - (float)width;}     // right wall
        if (position.x < 0){position.x = 0;}      // left wall 
        if (position.y + (float)height > gridY*levelY){ position.y = gridY*levelY - (float)height;}     // bottom wall
        if (position.y < 0){ position.y = 0;}     // top wall
    } else{
        position = Vec2{   gridX*30*(int)((int)(playerPos.x)/(30*gridX)),
                            gridY*17*(int)((int)(playerPos.y)/(17*gridY))};
    }
    originalPosition = position;
    screenShake();
}

// Start screen shake with a magnitude and duration
void Camera::startShake(float magnitude, int duration) {
    shakeMagnitude = magnitude;
    shakeDuration = duration;
    shakeTimeElapsed = 0;  // Reset elapsed time
}

// Function to apply screen shake
void Camera::screenShake() {
    if (shakeDuration > 0) {
        shakeTimeElapsed += 16; // Assuming 60 FPS, increase time (16ms per frame)
        if (shakeTimeElapsed < shakeDuration) {
            // Generate random offset
            float randomX = ((std::rand() % 2001) / 1000.0f - 1.0f) * shakeMagnitude;
            float randomY = ((std::rand() % 2001) / 1000.0f - 1.0f) * shakeMagnitude;

            // Apply random offset to the camera position
            position.x = originalPosition.x + randomX;
            position.y = originalPosition.y + randomY;

            // Optionally reduce the magnitude over time (decay effect)
            shakeMagnitude *= 0.9f; // Reduce the magnitude to create a decay effect
        } else {
            // Reset the shake when the duration is over
            shakeDuration = 0;
            position = originalPosition;
        }
    }
}

void Camera::toggleCameraFollow(){
    m_cameraFollow = !m_cameraFollow;
}

bool Camera::getCameraFollow(){
    return m_cameraFollow;
}

void Camera::setCameraZoom(Vec2 zoom){
    m_cameraZoom = zoom*m_cameraZoom;
}

Vec2 Camera::getCameraZoom(){
    return m_cameraZoom;
}

void Camera::update(Vec2 playerPos) {
    // Usual camera movement logic
    movement(playerPos); // Example player position

    // Apply screen shake effect if it's active
    screenShake();
}
