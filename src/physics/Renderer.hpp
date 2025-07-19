#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>

class RendererManager {
public:
    using EntityID = uint32_t;

    // Adds an entity ID to the specified layer
    void addEntityToLayer(EntityID entityID, uint8_t layer) {
        if (layer >= layers.size()) {
            layers.resize(layer + 1);
        }
        layers[layer].push_back(entityID);
    }

    // Removes an entity ID from the specified layer
    void removeEntityFromLayer(EntityID entityID, uint8_t layer) {
        if (layer < layers.size()) {
            auto& entities = layers[layer];
            entities.erase(std::remove(entities.begin(), entities.end(), entityID), entities.end());
        }
    }

    // Gets the entities in the specified layer
    const std::vector<EntityID>* getEntitiesInLayer(uint8_t layer) const {
        if (layer < layers.size()) {
            return &layers[layer];
        }
        return nullptr;
    }
    
    // Returns a pointer to the layers so they can be for-looped through in another class
    const std::vector<std::vector<EntityID>> getLayers() const {
        return layers;
    }

private:
    std::vector<std::vector<EntityID>> layers;
};

#endif // RENDERER_HPP