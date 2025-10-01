#pragma once
#include "ecs/Components.h"

#include <fstream>
#include <iostream>

enum class ItemType {
    None,
    Weapon,
    WeaponMelee,
    WeaponRanged,
    WeaponAoE,
    Consumable,
    Quest
};

struct Item {
    int id = -1;
    int index = -1;
    std::string name;
    std::string description;
    std::string iconPath;
    int stack;
    int maxStack = 1;

    ItemType type = ItemType::None;
    int damage;
    int healing;
    
    Item() = default;
    ItemType getItemTypeFromString(const std::string& typeStr) {
        if (typeStr == "WeaponMelee") return ItemType::WeaponMelee;
        if (typeStr == "WeaponRanged") return ItemType::WeaponRanged;
        if (typeStr == "WeaponAoE") return ItemType::WeaponAoE;
        if (typeStr == "Consumable") return ItemType::Consumable;
        if (typeStr == "Quest") return ItemType::Quest;
        return ItemType::None;
    }
    Item(const json& j) {
        id          = j.value("id", -1);
        name        = j.value("name", "Unknown");
        description = j.value("description", "");
        iconPath    = j.value("iconPath", "");
        maxStack    = j.value("maxStack", 1);

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
        std::vector<std::string> names = {"coin", "staff", "sword"};
        for (std::string name: names){
            std::ifstream file("config_files/items/"+name+".json");
            if (!file) {
                std::cerr << "Could not load item file!\n";
                exit(-1);
            }
            json j;
            file >> j;
            file.close();
            Item item(j[name]);
            addItem(item);
        }
    }
    
    void addItem(const Item& item) {
        items[item.id] = item;
    }
    
    const Item& getItem(int id) const {
        return items.at(id);
    }
    

private:
    std::unordered_map<int, Item> items;
};