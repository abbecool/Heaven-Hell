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
    void addEntityToLayer(EntityID entityID, uint8_t layerIndex) {
        if (layerIndex >= layers.size()) {
            layers.resize(layerIndex + 1);
        }
        layers[layerIndex].push_back(entityID);
    }

    void removeEntityFromLayer(EntityID id, uint8_t layerIndex) {
        if (layerIndex >= layers.size()) {
            std::cout << "ERROR: Layer " << (int)layerIndex << " doesn't exist! (max: " 
                      << layers.size() << ")" << std::endl;
            return;
        }
        auto& layer = layers[layerIndex];
        size_t before = layer.size();
        layer.erase(std::remove(layer.begin(), layer.end(), id), layer.end());
        if (before == layer.size()) {
            std::cout << "WARNING: Entity " << id << " not found in Layer " << (int)layerIndex << std::endl;
            // Check if it exists in any other layer
            for (size_t i = 0; i < layers.size(); ++i) {
                if (i == layerIndex) continue;
                auto it = std::find(layers[i].begin(), layers[i].end(), id);
                if (it != layers[i].end()) {
                    std::cout << "  -> Entity " << id << " found in Layer " << i << std::endl;
                }
            }
        }
    }

    void queueRemoveEntityFromLayer(EntityID entityID, uint8_t layerIndex) {
        entitiesToRemove.resize(layers.size());
        entitiesToRemove[layerIndex].push_back(entityID);
    }

    void update() {
        entitiesToRemove.resize(layers.size());
        
        for (int layerIndex = 0; layerIndex < entitiesToRemove.size(); layerIndex++) {
            for (auto entityID : entitiesToRemove[layerIndex]) {
                removeEntityFromLayer(entityID, layerIndex);
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
    const std::vector<EntityID>* getEntitiesInLayer(uint8_t layerIndex) const {
        if (layerIndex < layers.size()) {
            return &layers[layerIndex];
        }
        return nullptr;
    }
    
    const std::vector<std::vector<EntityID>>& getLayers() const {
        return layers;
    }

private:
    std::vector<std::vector<EntityID>> layers;
    std::vector<std::vector<EntityID>> entitiesToRemove; // Entities to be removed in the next update
};

#endif // RENDERER_HPP
