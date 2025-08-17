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
    void removeEntityFromLayer(EntityID id, uint8_t layer) {
        if (layer >= layers.size()) {
            return; // layer does not exist
        }
        auto& l = layers[layer];
        l.erase(std::remove(l.begin(), l.end(), id), l.end());
    }

    void queueRemoveEntityFromLayer(EntityID entityID, uint8_t layer) {
        entitiesToRemove.resize(layers.size());
        entitiesToRemove[layer].push_back(entityID);
    }

    void update() {
        for (int layer = 0; layer < entitiesToRemove.size(); layer++) {
        // for (auto& layer : entitiesToRemove) {
            for (auto entityID : entitiesToRemove[layer]) {
                removeEntityFromLayer(entityID, layer);
            }
        }
        entitiesToRemove.clear();
    }

    std::vector<EntityID> getEntities() {
        std::vector<EntityID> entities;
        for (const auto& layer : layers) {
            for (const auto& entityID : layer) {
                entities.push_back(entityID);
            }
        }
        return entities;
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
    std::vector<std::vector<EntityID>> entitiesToRemove; // Entities to be removed in the next update
};

#endif // RENDERER_HPP