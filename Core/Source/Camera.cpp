#include "Camera.h"
#include "Vec2.h"

#include <iostream>
#include <random>
#include <cmath>

Camera::Camera(){}

void Camera::calibrate(Vec2 screenSize, Vec2 levelSize, Vec2 gridSize){
    m_screenSize = screenSize;
    m_levelSize = levelSize;
    m_gridSize = gridSize;
    position = Vec2{0,0};
}

void Camera::reset() {
        position = originalPosition;
    }

void Camera::movement(Vec2 playerPos){
    auto width = m_screenSize.x;
    auto height = m_screenSize.y;
    auto levelX = m_levelSize.x;
    auto levelY = m_levelSize.y;
    auto gridX = m_gridSize.x;
    auto gridY = m_gridSize.y;

    if (!m_cameraFollow){
        originalPosition = playerPos - Vec2(width / 2, height / 2);
        if (originalPosition.x + (float)width > gridX*levelX){ originalPosition.x = gridX*levelX - (float)width;}     // right wall
        if (originalPosition.x < 0){originalPosition.x = 0;}      // left wall 
        if (originalPosition.y + (float)height > gridY*levelY){ originalPosition.y = gridY*levelY - (float)height;}     // bottom wall
        if (originalPosition.y < 0){ originalPosition.y = 0;}     // top wall
    } else{
        originalPosition = Vec2{   gridX*30*(int)((int)(playerPos.x)/(30*gridX)),
                            gridY*17*(int)((int)(playerPos.y)/(17*gridY))};
    }
    position = originalPosition;
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

// Start screen shake with a magnitude and duration
bool Camera::startPan(float speed, int duration, Vec2 pos, bool pause) {
    panSpeed = speed;
    panDuration = duration;
    panTimeElapsed = 0;  // Reset elapsed time
    panPos = pos;
    panStartPos = originalPosition;
    panInitPause = pause;
    i = 0;
    return true;
}

void Camera::panCamera(){
    Vec2 panVelocity = (panPos - panStartPos).norm()*(float)i* panSpeed / 60;
    if (panDuration > 0) {
        if ( ( (panPos-(panStartPos + panVelocity)).length() > 32) && !(panTimeElapsed >= panDuration) ) {
            i++;
        } else {
            panTimeElapsed += 16; // Assuming 60 FPS, increase time (16ms per frame)
        }
        if ( panTimeElapsed >= panDuration ) {
            if ( ( panStartPos-(panStartPos + panVelocity) ).length() > 32 ) {
                i--;
            } else {
                panDuration = 0;
                m_cameraPause = panInitPause;
            }
        }
        position = panStartPos + panVelocity;
    }
}

bool Camera::update(Vec2 playerPos, bool pause) {
    m_cameraPause = pause;
    // Usual camera movement logic
    movement(playerPos); // Example player position
    
    // Apply screen shake effect if it's active
    screenShake();
    
    if (panDuration > 0) {
        panCamera();
    }

    return m_cameraPause;
}

#include "ScriptableEntity.h"
#include <iostream>

class CameraController : public ScriptableEntity 
{
public:
    void OnCreateFunction()
    {
        // std::cout << "create script entity: OnCreate" << std::endl;
    }

    void OnDestroyFunction()
    {
        // std::cout << "destoy script entity: OnDestroy" << std::endl;
    }

    void OnUpdateFunction()
    {
        // auto& pos = getComponent<CTransform>().pos;
        // pos.x -= 1;
    }
};

