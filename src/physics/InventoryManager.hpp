#pragma once
#include "external/json.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

enum class ItemType {
    None,
    Weapon,
    WeaponMelee,
    WeaponRanged,
    WeaponAoE,
    Consumable,
    Quest
};

enum class PickupMode {
    Manual,
    Automatic
};

inline PickupMode pickupModeFromString(const std::string& mode)
{
    if (mode == "Manual") return PickupMode::Manual;
    if (mode == "Automatic") return PickupMode::Automatic;
    throw std::invalid_argument("Unknown pickup mode: " + mode);
}

struct Item {
    int id = -1;
    int index = -1;
    std::string name;
    std::string description;
    std::string iconPath;
    int stack = 0;
    int maxStack = 1;
    PickupMode pickupMode = PickupMode::Manual;

    ItemType type = ItemType::None;
    int damage = 0;
    int healing = 0;
    
    Item() = default;
    ItemType getItemTypeFromString(const std::string& typeStr) {
        if (typeStr == "WeaponMelee") return ItemType::WeaponMelee;
        if (typeStr == "WeaponRanged") return ItemType::WeaponRanged;
        if (typeStr == "WeaponAoE") return ItemType::WeaponAoE;
        if (typeStr == "Consumable") return ItemType::Consumable;
        if (typeStr == "Quest") return ItemType::Quest;
        return ItemType::None;
    }
    Item(const nlohmann::json& j) {
        id          = j.value("id", -1);
        name        = j.value("name", "Unknown");
        description = j.value("description", "");
        iconPath    = j.value("iconPath", "");
        maxStack    = j.value("maxStack", 1);
        pickupMode  = pickupModeFromString(j.value("pickupMode", "Manual"));

        type = getItemTypeFromString(j.value("type", "None"));

        // load stats only if they exist
        if (j.contains("damage")) {
            damage = j["damage"].get<int>();
        }
        if (j.contains("healing")) {
            healing = j["healing"].get<int>();
        }
    }
};


class InventoryManager
{
    public:
    InventoryManager(){}
    InventoryManager(const std::string& path) {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".json") {
                continue;
            }

            std::ifstream file(entry.path());
            if (!file) {
                throw std::runtime_error("Could not load item file: " + entry.path().string());
            }

            nlohmann::json j;
            file >> j;

            const std::string itemName = entry.path().stem().string();
            if (!j.contains(itemName)) {
                throw std::runtime_error("Item file is missing its '" + itemName + "' definition: " + entry.path().string());
            }

            Item item(j[itemName]);
            addItem(item);
        }
    }
    
    void addItem(const Item& item) {
        items[item.id] = item;
    }
    
    const Item& getItem(int id) const {
        return items.at(id);
    }

    const Item* findItem(const std::string& name) const {
        for (const auto& [id, item] : items) {
            if (item.name == name) {
                return &item;
            }
        }
        return nullptr;
    }
    

private:
    std::unordered_map<int, Item> items;
};
