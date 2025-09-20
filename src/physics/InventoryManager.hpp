#pragma once
#include "ecs/Components.h"

#include <fstream>
#include <iostream>

struct Item {
    int id = -1;
    int index = -1;
    std::string name;
    std::string description;
    std::string iconPath;
    int stack;
    int maxStack = 1;
    Item(){}
    Item(const json j){
        id = j["id"];
        name = j["name"];
        description = j["description"];
        iconPath = j["iconPath"];
        maxStack = j["maxStack"];
    }
};

class InventoryManager
{
public:
    InventoryManager(){}
    InventoryManager(const std::string& path) {
        std::vector<std::string> names = {"coin", "staff"};
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