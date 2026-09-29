#include "scenes/Scene_Play.hpp"

#include "external/json.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace
{
json placementSignature(const WorldLayout& layout)
{
    json placements = json::array();
    for (const LayoutPlacement& placement : layout.placements)
        placements.push_back({{"definition", placement.definition},
                              {"x", placement.x}, {"y", placement.y}});
    return placements;
}

json loadJsonFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
    {
        throw std::runtime_error("Could not load json file: " + path);
    }

    json data;
    file >> data;
    return data;
}
} // namespace

void Scene_Play::loadActiveLayout()
{
    LayoutRepository layouts;
    layouts.load();
    const LayoutInfo& info = layouts.activeLayout();
    const WorldLayout layout = layouts.loadLayout(info);

    for (size_t index = 0; index < layout.placements.size(); ++index)
    {
        if (m_removedPlacements.contains(index) || index == m_playerPlacement)
            continue;
        const LayoutPlacement& placement = layout.placements[index];
        const EntityID id = Spawn(placement.definition,
                                  Vec2{placement.x, placement.y});
        if (id == static_cast<EntityID>(-1))
        {
            std::cerr << "Could not spawn layout entity '"
                      << placement.definition << "' from layout " << info.id
                      << std::endl;
        }
        else
        {
            m_placedEntities.emplace(id, index);
            const auto state = m_restoredEntities.find(index);
            if (state != m_restoredEntities.end())
            {
                const json& saved = state->second;
                if (saved.contains("hp") && m_ECS.hasComponent<CHealth>(id))
                    m_ECS.getComponent<CHealth>(id).HP =
                        saved.at("hp").get<int>();
                if (saved.contains("position") &&
                    m_ECS.hasComponent<CTransform>(id))
                {
                    const Vec2 pos = Vec2(saved.at("position")) * m_gridSize;
                    auto& transform = m_ECS.getComponent<CTransform>(id);
                    transform.pos = pos;
                    transform.prevPos = pos;
                }
            }
        }
    }
    if (!m_newGame)
    {
        for (const json& item : m_loadedSave.at("world").at("dynamic_items"))
        {
            const std::string name = item.at("name").get<std::string>();
            if (!m_inventoryManager.findItem(name))
                throw std::runtime_error("Unknown saved world item: " + name);
            const EntityID id = Spawn(name, Vec2(item.at("position")));
            if (id == static_cast<EntityID>(-1))
                throw std::runtime_error("Could not restore world item: " + name);
            const Vec2 position = Vec2(item.at("position")) * m_gridSize;
            auto& transform = m_ECS.getComponent<CTransform>(id);
            transform.pos = position;
            transform.prevPos = position;
            if (item.at("manual_pickup").get<bool>())
            {
                auto& component = m_ECS.getComponent<CItem>(id);
                component.hasPickupModeOverride = true;
                component.pickupModeOverride = PickupMode::Manual;
            }
        }
    }
}

void Scene_Play::restoreSavedWorld()
{
    m_loadedSave = m_saveRepository.load();
    LayoutRepository layouts;
    layouts.load();
    const json& world = m_loadedSave.at("world");
    if (world.at("layout") != layouts.activeLayout().id)
        throw std::runtime_error("Saved world uses a different level layout");
    const WorldLayout layout = layouts.loadLayout(layouts.activeLayout());
    if (world.at("placements") != placementSignature(layout))
        throw std::runtime_error("Level placements changed since save");
    for (const json& indexJson : world.at("removed_placements"))
    {
        const size_t index = indexJson.get<size_t>();
        if (index >= layout.placements.size() ||
            !m_removedPlacements.insert(index).second)
            throw std::runtime_error("Invalid removed placement in save");
    }
    if (!world.at("player_placement").is_null())
    {
        m_playerPlacement = world.at("player_placement").get<size_t>();
        if (m_playerPlacement >= layout.placements.size() ||
            m_removedPlacements.contains(m_playerPlacement) ||
            layout.placements[m_playerPlacement].definition !=
                m_loadedSave.at("player").at("definition").get<std::string>())
            throw std::runtime_error("Invalid player host in save");
    }
    for (const json& state : world.at("entities"))
    {
        const size_t index = state.at("index").get<size_t>();
        if (index >= layout.placements.size() ||
            m_removedPlacements.contains(index) || index == m_playerPlacement ||
            !m_restoredEntities.emplace(index, state).second)
            throw std::runtime_error("Invalid placed entity state in save");
    }
    for (const json& item : world.at("dynamic_items"))
    {
        if (!item.is_object() || !item.at("name").is_string() ||
            !item.at("position").is_object() ||
            !item.at("manual_pickup").is_boolean())
            throw std::runtime_error("Invalid saved world item");
    }
    m_storyManager.loadState(m_loadedSave.at("story"));
}

void Scene_Play::saveGame()
{
    Vec2 playerPos = m_ECS.getComponent<CTransform>(m_player).pos;
    int hp = m_ECS.getComponent<CHealth>(m_player).HP;
    int currency = 0;
    if (m_ECS.hasComponent<CCurrency>(m_player))
    {
        currency = m_ECS.getComponent<CCurrency>(m_player).value;
    }
    const CInventory& inventory = m_ECS.getComponent<CInventory>(m_player);

    json inventoryItems = json::array();
    for (int i = 0; i < inventory.size(); ++i)
    {
        const Item& item = inventory.items[i];
        if (item.id == -1)
        {
            inventoryItems.push_back(nullptr);
            continue;
        }
        inventoryItems.push_back(item.id);
    }

    json removed = json::array();
    std::vector<size_t> indices(m_removedPlacements.begin(),
                                m_removedPlacements.end());
    std::sort(indices.begin(), indices.end());
    for (size_t index : indices)
        removed.push_back(index);
    json entities = json::array();
    for (const auto& [id, index] : m_placedEntities)
    {
        if (!m_ECS.isAlive(id))
            continue;
        json state = {{"index", index}};
        if (m_ECS.hasComponent<CHealth>(id))
            state["hp"] = m_ECS.getComponent<CHealth>(id).HP;
        if (m_ECS.hasComponent<CTransform>(id))
        {
            const Vec2 pos = m_ECS.getComponent<CTransform>(id).pos / m_gridSize;
            state["position"] = {{"x", pos.x}, {"y", pos.y}};
        }
        entities.push_back(std::move(state));
    }
    json dynamicItems = json::array();
    for (auto [id, item, transform, name] :
         m_ECS.View<CItem, CTransform, CName>())
    {
        if (m_placedEntities.contains(id))
            continue;
        const Vec2 position = transform.pos / m_gridSize;
        dynamicItems.push_back({{"name", name.name},
                                {"position", {{"x", position.x}, {"y", position.y}}},
                                {"manual_pickup", item.hasPickupModeOverride &&
                                      item.pickupModeOverride == PickupMode::Manual}});
    }
    LayoutRepository layouts;
    layouts.load();
    const WorldLayout layout = layouts.loadLayout(layouts.activeLayout());
    json save = {{"version", 1}, {"player",
                  {{"definition", m_playerDefinition},
                   {"position",
                    {{"x", playerPos.x / m_gridSize.x},
                     {"y", playerPos.y / m_gridSize.y}}},
                   {"hp", hp},
                   {"hp_max", m_ECS.getComponent<CHealth>(m_player).HP_max},
                   {"currency", currency},
                   {"inventory",
                    {{"slots", inventory.size()},
                     {"items", inventoryItems},
                     {"activeSlot", inventory.activeItem.index}}}}},
                 {"story", m_storyManager.saveState()},
                 {"world", {{"layout", layouts.activeLayout().id},
                             {"placements", placementSignature(layout)},
                             {"removed_placements", removed},
                             {"entities", entities},
                             {"dynamic_items", dynamicItems},
                             {"player_placement", m_playerPlacement == static_cast<size_t>(-1)
                                  ? json(nullptr) : json(m_playerPlacement)}}}};
    m_saveRepository.write(save);
}

const Item* Scene_Play::findItemFromJson(const json& itemRef) const
{
    try
    {
        if (itemRef.is_number_integer())
        {
            return &m_inventoryManager.getItem(itemRef.get<int>());
        }
        if (itemRef.is_string())
        {
            return m_inventoryManager.findItem(itemRef.get<std::string>());
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Invalid inventory item reference: " << exception.what()
                  << std::endl;
    }
    return nullptr;
}

void Scene_Play::loadInventoryFromJson(EntityID entity,
                                       const json& inventoryJson)
{
    const json itemRefs = inventoryJson.is_object()
                              ? inventoryJson.value("items", json::array())
                              : inventoryJson;

    int requestedSlotCount = CInventory::DefaultSlotCount;
    if (inventoryJson.is_object())
    {
        requestedSlotCount = inventoryJson.value(
            "slots", itemRefs.is_array() && !itemRefs.empty()
                         ? static_cast<int>(itemRefs.size())
                         : CInventory::DefaultSlotCount);
    }
    else if (itemRefs.is_array() && !itemRefs.empty())
    {
        requestedSlotCount = static_cast<int>(itemRefs.size());
    }

    if (!m_ECS.hasComponent<CInventory>(entity))
    {
        m_ECS.addComponent<CInventory>(entity, requestedSlotCount);
    }

    CInventory& inventory = m_ECS.getComponent<CInventory>(entity);
    inventory.items.assign(std::max(1, requestedSlotCount), Item{});
    for (int i = 0; i < inventory.size(); ++i)
    {
        inventory.items[i].index = i;
    }

    int activeSlot =
        inventoryJson.is_object()
            ? inventoryJson.value("activeSlot", inventory.activeItem.index)
            : 0;
    activeSlot = std::clamp(activeSlot, 0, inventory.size() - 1);

    if (itemRefs.is_array())
    {
        const int count =
            std::min(inventory.size(), static_cast<int>(itemRefs.size()));
        for (int i = 0; i < count; ++i)
        {
            if (itemRefs[i].is_null())
            {
                continue;
            }

            const Item* item = findItemFromJson(itemRefs[i]);
            if (!item)
            {
                std::cerr << "Unknown inventory item: " << itemRefs[i].dump()
                          << std::endl;
                continue;
            }

            inventory.items[i] = *item;
            inventory.items[i].index = i;
        }
    }

    updateActiveItem(entity, activeSlot);
}

void Scene_Play::updateActiveItem(EntityID entity, int newIndex)
{
    if (!m_ECS.hasComponent<CInventory>(entity))
    {
        return;
    }

    CInventory& inventory = m_ECS.getComponent<CInventory>(entity);
    if (newIndex < 0 || newIndex >= inventory.size())
    {
        return;
    }

    inventory.activeItem = inventory.items[newIndex];
    inventory.activeItem.index = newIndex;

    if (m_ECS.hasComponent<CWeapon>(entity))
    {
        m_ECS.removeComponent<CWeapon>(entity);
    }

    if (inventory.activeItem.hasWeaponConfig)
    {
        m_ECS.addComponent<CWeapon>(entity, inventory.activeItem.weaponConfig);
    }
}

void Scene_Play::updateActiveItem(int newIndex)
{
    updateActiveItem(m_player, newIndex);
}

float Scene_Play::activeItemUseRange(EntityID entity)
{
    constexpr float DefaultItemUseRange = 48.0f;
    if (!m_ECS.hasComponent<CInventory>(entity))
    {
        return 0.0f;
    }

    const Item& activeItem = m_ECS.getComponent<CInventory>(entity).activeItem;
    if (activeItem.id == -1)
    {
        return 0.0f;
    }
    if (m_ECS.hasComponent<CWeapon>(entity))
    {
        return static_cast<float>(m_ECS.getComponent<CWeapon>(entity).range);
    }
    if (activeItem.type == ItemType::Consumable)
    {
        return DefaultItemUseRange;
    }
    return 0.0f;
}

bool Scene_Play::useActiveConsumable(EntityID entity)
{
    if (!m_ECS.hasComponent<CInventory>(entity))
    {
        return false;
    }

    CInventory& inventory = m_ECS.getComponent<CInventory>(entity);
    const int activeIndex = inventory.activeItem.index;
    if (activeIndex < 0 || activeIndex >= inventory.size())
    {
        return false;
    }

    Item& activeItem = inventory.items[activeIndex];
    if (activeItem.type != ItemType::Consumable)
    {
        return false;
    }

    if (activeItem.healing <= 0 || !m_ECS.hasComponent<CHealth>(entity))
    {
        return false;
    }

    CHealth& health = m_ECS.getComponent<CHealth>(entity);
    if (health.HP >= health.HP_max)
    {
        return true;
    }

    health.HP = std::min(health.HP + activeItem.healing, health.HP_max);

    Item emptySlot;
    emptySlot.index = activeIndex;
    inventory.items[activeIndex] = emptySlot;
    updateActiveItem(entity, activeIndex);
    return true;
}

bool Scene_Play::addCurrencyToPlayer(int amount)
{
    if (!m_ECS.hasComponent<CCurrency>(m_player))
    {
        m_ECS.addComponent<CCurrency>(m_player);
    }

    m_ECS.getComponent<CCurrency>(m_player).value += amount;
    return true;
}

bool Scene_Play::addCurrencyToPlayer(const Item& item)
{
    return addCurrencyToPlayer(item.currencyValue);
}

EntityID Scene_Play::SpawnFromJSON(std::string name, Vec2 pos)
{
    const Item* item = m_inventoryManager.findItem(name);
    std::ifstream file;
    std::string definitionName = name;

    for (const char* directory : {"config_files/mobs", "config_files/entities"})
    {
        file.open(std::string(directory) + "/" + name + ".json");
        if (file.is_open())
        {
            break;
        }
        file.clear();
    }

    if (!file.is_open() && item)
    {
        file.open("config_files/entities/item.json");
        definitionName = "item";
    }

    if (!file.is_open())
    {
        std::cerr << "Could not load entity spawn json file for: " << name
                  << std::endl;
        return -1;
    }

    json j;
    file >> j;
    file.close();
    if (!j.contains(definitionName) ||
        !j[definitionName].contains("components"))
    {
        std::cerr << "Invalid entity spawn json file for: " << name
                  << std::endl;
        return -1;
    }

    const json& definition = j[definitionName];
    json c = definition["components"];
    if (item && definitionName == "item")
    {
        c["CName"] = item->name;
        c["CItem"]["itemID"] = item->id;
        c["CAnimation"]["animation"] = item->iconPath;
        if (item->hasShadowConfig)
        {
            c["CShadow"] = item->shadowConfig;
        }
    }

    bool hasEvent = false;
    Event event;
    if (definition.contains("event"))
    {
        const json& eventConfig = definition["event"];
        if (!eventConfig.is_object() || !eventConfig.contains("type") ||
            !eventConfig.contains("subject"))
        {
            std::cerr << "Invalid event directive for: " << name << std::endl;
            return -1;
        }

        try
        {
            event = Event{m_storyManager.getEventTypeFromString(
                              eventConfig.at("type").get<std::string>()),
                          eventConfig.at("subject").get<std::string>()};
            hasEvent = true;
        }
        catch (const std::exception& exception)
        {
            std::cerr << "Invalid event directive for " << name << ": "
                      << exception.what() << std::endl;
            return -1;
        }
    }

    EntityID id = m_ECS.addEntity();

    m_ECS.addComponent<CName>(id, c.value("CName", name));
    if (c.contains("CAllegiance"))
    {
        m_ECS.addComponent<CAllegiance>(id, c["CAllegiance"]);
    }
    if (c.contains("CAnimation"))
    {
        addVisual(id, c["CAnimation"]["animation"].get<std::string>(),
                  renderLayerFromJson(c["CAnimation"]["layer"]));
    }
    if (c.contains("CTransform"))
    {
        pos = gridToMidPixel(pos, id);
        c["CTransform"]["pos"] = {{"x", pos.x}, {"y", pos.y}};
        m_ECS.addComponent<CTransform>(id, c["CTransform"]);
    }
    if (c.contains("CShadow"))
    {
        m_ECS.addComponent<CShadow>(id, c["CShadow"]);
    }
    if (c.contains("CPhysicsBody"))
    {
        m_ECS.addComponent<CPhysicsBody>(id, c["CPhysicsBody"]);
        m_ECS.addComponent<CVelocity>(id);
    }
    if (c.contains("CItem"))
    {
        m_ECS.addComponent<CItem>(id, c["CItem"].at("itemID").get<int>());
    }
    if (c.contains("CCollider"))
    {
        m_ECS.addComponent<CCollider>(id, c["CCollider"]);
    }
    if (c.contains("CState"))
    {
        m_ECS.addComponent<CState>(id);
    }
    if (c.contains("CHealth"))
    {
        m_ECS.addComponent<CHealth>(id, c["CHealth"]);
    }
    if (c.contains("CPossessable"))
    {
        m_ECS.addComponent<CPossessable>(id, c["CPossessable"]);
    }
    if (c.contains("CInput"))
    {
        m_ECS.addComponent<CInput>(id);
    }
    if (c.contains("CInventory"))
    {
        loadInventoryFromJson(id, c["CInventory"]);
    }
    if (c.contains("CCurrency"))
    {
        m_ECS.addComponent<CCurrency>(id, c["CCurrency"]);
    }
    if (c.contains("CAIAgent"))
    {
        if (!m_ECS.hasComponent<CInput>(id))
        {
            m_ECS.addComponent<CInput>(id);
        }
        if (!m_ECS.hasComponent<CState>(id))
        {
            m_ECS.addComponent<CState>(id);
        }
        m_ECS.addComponent<CAIAgent>(id, c["CAIAgent"]);
        m_ECS.getComponent<CAIAgent>(id).spawnPos = pos;
    }
    if (hasEvent)
    {
        m_ECS.addComponent<CEvent>(id, event);
    }
    if (m_ECS.hasComponent<CTransform>(id) && m_ECS.hasComponent<CSprite>(id))
    {
        const CShadow shadowConfig = m_ECS.hasComponent<CShadow>(id)
                                         ? m_ECS.getComponent<CShadow>(id)
                                         : CShadow{};
        spawnShadow(id, shadowConfig);
    }
    return id;
}

EntityID Scene_Play::Spawn(std::string name, Vec2 pos)
{
    return SpawnFromJSON(name, pos * m_gridSize);
}

EntityID Scene_Play::DropItem(const Item& item, Vec2 position)
{
    if (item.id == -1)
    {
        return static_cast<EntityID>(-1);
    }

    EntityID droppedID = SpawnFromJSON(item.name, position);
    if (droppedID == static_cast<EntityID>(-1))
    {
        return droppedID;
    }

    if (m_ECS.hasComponent<CTransform>(droppedID))
    {
        CTransform& transform = m_ECS.getComponent<CTransform>(droppedID);
        transform.pos = position;
        transform.prevPos = position;
    }

    if (m_ECS.hasComponent<CItem>(droppedID))
    {
        CItem& droppedItem = m_ECS.getComponent<CItem>(droppedID);
        droppedItem.hasPickupModeOverride = true;
        droppedItem.pickupModeOverride = PickupMode::Manual;
    }

    return droppedID;
}

EntityID Scene_Play::spawnPlayer()
{
    json playerSave;
    m_playerDefinition = "player";

    if (!m_newGame)
    {
        playerSave = m_loadedSave.at("player");
        m_playerDefinition = playerSave.value("definition", "player");
    }

    const json playerTemplate = loadJsonFile("config_files/entities/player.json");
    Vec2 spawnGrid = playerTemplate.at("player").at("spawn");
    if (playerSave.contains("position"))
    {
        spawnGrid = playerSave.at("position");
    }

    EntityID entityID = Spawn(m_playerDefinition, spawnGrid);
    if (entityID == static_cast<EntityID>(-1))
    {
        throw std::runtime_error("Could not spawn player from definition: " +
                                 m_playerDefinition);
    }
    m_player = entityID;
    if (playerSave.contains("position"))
    {
        const Vec2 position = Vec2(playerSave.at("position")) * m_gridSize;
        auto& transform = m_ECS.getComponent<CTransform>(entityID);
        transform.pos = position;
        transform.prevPos = position;
    }
    if (m_playerDefinition != "player")
    {
        m_ECS.addComponent<CCollider>(
            entityID, playerTemplate.at("player").at("components").at("CCollider"));
        if (!m_ECS.hasComponent<CInput>(entityID))
            m_ECS.addComponent<CInput>(entityID);
        if (m_ECS.hasComponent<CAIAgent>(entityID))
            m_ECS.removeComponent<CAIAgent>(entityID);
        if (m_ECS.hasComponent<CPossessable>(entityID))
            m_ECS.removeComponent<CPossessable>(entityID);
        if (m_ECS.hasComponent<CLifespan>(entityID))
            m_ECS.removeComponent<CLifespan>(entityID);
    }

    if (playerSave.contains("hp") && m_ECS.hasComponent<CHealth>(entityID))
    {
        CHealth& health = m_ECS.getComponent<CHealth>(entityID);
        health.HP = playerSave.at("hp").get<int>();
        health.HP_max = playerSave.at("hp_max").get<int>();
    }

    if (!m_ECS.hasComponent<CCurrency>(entityID))
    {
        m_ECS.addComponent<CCurrency>(entityID);
    }
    m_ECS.getComponent<CCurrency>(entityID).value =
        playerSave.contains("currency") ? playerSave.at("currency").get<int>()
                                        : 0;

    if (!m_ECS.hasComponent<CInventory>(entityID))
    {
        m_ECS.addComponent<CInventory>(entityID);
    }

    if (playerSave.contains("inventory"))
    {
        const json& inventorySave = playerSave.at("inventory");
        loadInventoryFromJson(entityID, inventorySave);
    }

    auto& inventory = m_ECS.getComponent<CInventory>(entityID);
    for (Item& item : inventory.items)
    {
        if (item.hasWeaponConfig)
            item.weaponConfig["mask"] = json::array({"ENEMY_LAYER"});
    }
    if (inventory.activeItem.index >= 0 &&
        inventory.activeItem.index < inventory.size())
    {
        updateActiveItem(entityID, inventory.activeItem.index);
    }

    return entityID;
}
