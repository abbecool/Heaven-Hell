#include "Camera.h"
#include "Vec2.h"
#include <random>
#include "Entity.h"
#include "Scene_Play.h"


// Random offset generation for screen shake
// Vec2 Camera::getShakeOffset(float intensity) {
//     static std::default_random_engine generator;
//     std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
    
//     float offsetX = distribution(generator) * intensity;
//     float offsetY = distribution(generator) * intensity;
    
//     return Vec2(offsetX, offsetY);
// }

void Camera::reset() {
        position = originalPosition;
    }

void Camera::update() {
    
}

Vec2 Camera::movement(std::shared_ptr<Entity> player, Scene_Play scene){
    auto height = scene.height();
    auto width = scene.width();
    auto levelSize = scene.levelSize();
    auto gridSize = scene.gridSize();
    
    if (m_cameraFollow){
        position = player->getComponent<CTransform>().pos - Vec2(width / 2, height / 2);
        if (position.x + (float)width > gridSize.x*levelSize.x){ position.x = gridSize.x*levelSize.x - (float)width;}     // right wall
        if (position.x < 0){position.x = 0;}      // left wall 
        if (position.y + (float)height > gridSize.y*levelSize.y){ position.y = gridSize.y*levelSize.y - (float)height;}     // bottom wall
        if (position.y < 0){ position.y = 0;}     // top wall
    } else{
        position = Vec2{   gridSize.x*30*(int)((int)(player->getComponent<CTransform>().pos.x)/(30*gridSize.x)),
                            gridSize.y*17*(int)((int)(player->getComponent<CTransform>().pos.y)/(17*gridSize.y))};
    }
    return position;
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

// Update function to apply screen shake
// void Camera::update(float deltaTime, ScreenShakeComponent* screenShake) {
//     if (screenShake && screenShake->duration > 0) {
//         Vec2 shakeOffset = getShakeOffset(screenShake->intensity);
        
//         // Apply the shake offset to the camera position
//         Vec2 originalPosition = position;
//         position = originalPosition + shakeOffset;

//         // Decay the shake intensity over time
//         screenShake->intensity -= screenShake->decay * deltaTime;
//         screenShake->duration -= deltaTime;

//         // Ensure intensity doesn't go negative
//         if (screenShake->intensity < 0) {
//             screenShake->intensity = 0;
//         }

//         // Reset the camera position after the shake effect
//         if (screenShake->duration <= 0) {
//             position = originalPosition;  // Return camera to its original position
//         }
//     }
// }
