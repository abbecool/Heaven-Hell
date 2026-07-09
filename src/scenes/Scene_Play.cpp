#include "scenes/Scene_Play.hpp"
#include "scenes/Scene_Menu.hpp"
#include "scenes/Scene_GameOver.hpp"

#include "assets/Assets.hpp"

#include "core/Game.hpp"
#include "core/Action.hpp"

#include "physics/Level_Loader.hpp"
#include "physics/Camera.hpp"
#include "physics/RandomArray.hpp"

#include "ecs/Components.hpp"

#include "external/json.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <queue>

// TODO: fix line count in file

using json = nlohmann::json;

namespace {
constexpr float PHYSICS_DT = 1.0f / 60.0f;
constexpr float SPRINT_MULTIPLIER = 2.0f;
constexpr float STOP_SPEED = 1.0f;

json loadJsonFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not load json file: " + path);
    }

    json data;
    file >> data;
    return data;
}

}

Scene_Play::Scene_Play(Game* game, std::string levelPath, bool newGame)
    : Scene(game), 
    m_levelPath(levelPath), 
    m_collisionManager(&m_ECS, this), 
    m_storyManager(this, "config_files/story.json", "config_files/quests.json"),
    m_levelLoader(this, m_gridSize, game->loadImagePixels(levelPath)),
    m_newGame(newGame),
    m_inventoryManager("config_files/items")
{
    registerAction(InputCode::W, "UP");
    registerAction(InputCode::Up, "UP");
    registerAction(InputCode::S, "DOWN");
    registerAction(InputCode::Down, "DOWN");
    registerAction(InputCode::A, "LEFT");
    registerAction(InputCode::Left, "LEFT");
    registerAction(InputCode::D, "RIGHT");
    registerAction(InputCode::Right, "RIGHT");
    
    registerAction(InputCode::I, "INVENTORY");
    registerAction(InputCode::MouseLeft, "USE");
    registerAction(InputCode::MouseRight, "WRITE POSITION");
    registerAction(InputCode::MouseWheel, "SCROLL");
    registerAction(InputCode::E, "INTERACT");
    registerAction(InputCode::LeftShift, "SHIFT");
    registerAction(InputCode::LeftCtrl, "CTRL");
    registerAction(InputCode::Escape, "ESC");
    registerAction(InputCode::U, "SAVE");
    registerAction(InputCode::R, "RESET");
    registerAction(InputCode::T, "TAKE OVER");

    registerAction(InputCode::X, "CAMERA FOLLOW");
    registerAction(InputCode::Z, "CAMERA PAN");
    registerAction(InputCode::Plus, "ZOOM IN");
    registerAction(InputCode::Minus, "ZOOM OUT");
    registerAction(InputCode::Q, "WRITE QUADTREE");
    registerAction(InputCode::P, "PAUSE");
    registerAction(InputCode::O, "FPS COUNTER");
    registerAction(InputCode::K, "KILL_PLAYER");
    registerAction(InputCode::F3, "TOGGLE_COLLISION");
    registerAction(InputCode::F5, "TOGGLE_TEXTURE");
    registerAction(InputCode::Num1, "Slot1");
    registerAction(InputCode::Num2, "Slot2");
    registerAction(InputCode::Num3, "Slot3");
    registerAction(InputCode::Num7, "TP1");
    registerAction(InputCode::Num8, "TP2");
    registerAction(InputCode::Num9, "TP3");

    const Vec2 worldSize = m_levelLoader.getWorldSize();
    m_collisionManager.setWorldBounds(worldSize / 2.0f, worldSize);
    m_collisionManager.rebuildStaticQuadtree();

    spawnPlayer();
    // mobs have to spawn after player, so they can target the player
    loadMobsNItems("config_files/mobs.json");

    SpawnFromJSON("elf", Vec2{348, 64}*m_gridSize);
    SpawnFromJSON("wizard", Vec2{348, 60}*m_gridSize);
    SpawnFromJSON("knight", Vec2{344, 64}*m_gridSize);
    SpawnFromJSON("dwarf", Vec2{344, 60}*m_gridSize);

    m_camera.calibrate(Vec2{width(), height()}, m_levelLoader.getLevelSize(), m_gridSize);

    SubscribeToStoryEvents();
}

void Scene_Play::SubscribeToStoryEvents(){
    auto& quests = m_storyManager.getQuests();
    for (Quest quest : quests){
        QuestStep step = quest.steps[quest.currentStep];
        Event e = step.asEvent();
        m_eventBus.subscribe(e, [this](const Event& e) {
                m_storyManager.onEvent(e);
        });
    }
}

void Scene_Play::loadMobsNItems(const std::string& path){
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not load mobs/items file: " + path);
    }
    json j;
    file >> j;
    file.close();
    
    Vec2 pos;
    for (auto& [mobType, mobArray] : j["mobs"].items()) {
        for (auto& mob : mobArray) {
            pos = Vec2{mob["x"], mob["y"]};
            EntityID id = Spawn(mobType, pos);
        }
    }
}

// Function to save the game state to a file
void Scene_Play::saveGame() 
{
    Vec2 playerPos = m_ECS.getComponent<CTransform>(m_player).pos;
    int hp = m_ECS.getComponent<CHealth>(m_player).HP;
    int currency = 0;
    if (m_ECS.hasComponent<CCurrency>(m_player)) {
        currency = m_ECS.getComponent<CCurrency>(m_player).value;
    }
    const CInventory& inventory = m_ECS.getComponent<CInventory>(m_player);

    json inventoryItems = json::array();
    for (int i = 0; i < inventory.size(); ++i) {
        const Item& item = inventory.items[i];
        if (item.id == -1) {
            inventoryItems.push_back(nullptr);
            continue;
        }
        inventoryItems.push_back(item.id);
    }

    std::ofstream file("config_files/game_save.json");
    if (!file) {
        throw std::runtime_error("Could not open game save file: config_files/game_save.json");
    }    
    json save = {
        {"player", {
            {"definition", m_playerDefinition},
            {"position", {
                {"x", int(playerPos.x / m_gridSize.x)},
                {"y", int(playerPos.y / m_gridSize.y)}
            }},
            {"hp", hp},
            {"currency", currency},
            {"inventory", {
                {"slots", inventory.size()},
                {"items", inventoryItems},
                {"activeSlot", inventory.activeItem.index}
            }},
            {"progression", json::object()}
        }}
    };
    file << save.dump(4);
    file.close();
}

void Scene_Play::sDoAction(const Action& action){
    if ( action.type() == "START")
    {
        if ( action.name() == "RESET") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, m_levelPath, true), true);
        } else if ( action.name() == "KILL_PLAYER") { 
            m_ECS.getComponent<CHealth>(m_player).HP = 0;
        } else if ( action.name() == "Slot1") { 
            updateActiveItem(0);
        } else if ( action.name() == "Slot2") { 
            updateActiveItem(1);
        } else if ( action.name() == "Slot3") { 
            updateActiveItem(2);
        }
        if ( action.name() == "UP"){m_ECS.getComponent<CInput>(m_player).up = true;}
        if ( action.name() == "DOWN"){m_ECS.getComponent<CInput>(m_player).down = true;}
        if ( action.name() == "LEFT"){m_ECS.getComponent<CInput>(m_player).left = true;}
        if ( action.name() == "RIGHT"){m_ECS.getComponent<CInput>(m_player).right = true;}
        if ( action.name() == "SHIFT"){m_ECS.getComponent<CInput>(m_player).shift = true;}
        if ( action.name() == "CTRL"){m_ECS.getComponent<CInput>(m_player).ctrl = true;}
        if ( action.name() == "INTERACT"){m_ECS.getComponent<CInput>(m_player).interact = true;}
        if ( action.name() == "TAKE OVER"){m_ECS.getComponent<CInput>(m_player).posses = true;}
        if ( action.name() == "USE"){
            auto& input = m_ECS.getComponent<CInput>(m_player);
            input.use = true;
            input.useHeld = true;
        }
    }
    else if ( action.type() == "END")
    {
        if ( action.name() == "TOGGLE_TEXTURE") { m_drawTextures = !m_drawTextures; }
        if ( action.name() == "TOGGLE_COLLISION") { m_drawCollision = !m_drawCollision; }
        if ( action.name() == "PAUSE") { togglePause(); }
        if ( action.name() == "FPS COUNTER") { m_game->toggleRenderFPS(); }
        if ( action.name() == "INVENTORY") { std::cout << "toggle inventory" << std::endl; }
        if ( action.name() == "ZOOM IN"){ m_camera.stepCameraZoom(-1, m_game->getScale()); }
        if ( action.name() == "ZOOM OUT"){ m_camera.stepCameraZoom(1, m_game->getScale()); }
        if ( action.name() == "CAMERA FOLLOW"){ m_camera.toggleCameraFollow(); }
        if ( action.name() == "CAMERA PAN"){ m_pause = m_camera.startPan(2048, 1000, Vec2{0,0}, m_pause); }
        if ( action.name() == "SAVE"){ saveGame(); }
        if ( action.name() == "TP1") { m_ECS.getComponent<CTransform>(m_player).pos = Vec2{460*16, 460*16}; }
        if ( action.name() == "TP2") { m_ECS.getComponent<CTransform>(m_player).pos = Vec2{292*16, 236*16}; }
        if ( action.name() == "TP3") { m_ECS.getComponent<CTransform>(m_player).pos = Vec2{801*16, 181*16}; }
        if ( action.name() == "DOWN") { m_ECS.getComponent<CInput>(m_player).down = false; }
        if ( action.name() == "UP") { m_ECS.getComponent<CInput>(m_player).up = false; }
        if ( action.name() == "LEFT") { m_ECS.getComponent<CInput>(m_player).left = false; }
        if ( action.name() == "RIGHT") { m_ECS.getComponent<CInput>(m_player).right = false; }
        if ( action.name() == "SHIFT") { m_ECS.getComponent<CInput>(m_player).shift = false; }
        if ( action.name() == "CTRL") { m_ECS.getComponent<CInput>(m_player).ctrl = false; }
        if ( action.name() == "INTERACT") { m_ECS.getComponent<CInput>(m_player).interact = false; }
        if ( action.name() == "TAKE OVER") { m_ECS.getComponent<CInput>(m_player).posses = false; }
        if ( action.name() == "USE") {
            auto& input = m_ECS.getComponent<CInput>(m_player);
            input.use = false;
            input.useHeld = false;
        }
        if ( action.name() == "WRITE POSITION") { 
            Vec2 cursorPosition = (m_mousePosition+m_camera.position)/m_gridSize;
            cursorPosition.print("Cursor position");
        }
        if ( action.name() == "ESC") {
            m_game->changeScene("SETTINGS", std::make_shared<Scene_Pause>(m_game), false);
            saveGame();
            m_pause = true;
        }
    }
    else if ( action.name() == "SCROLL"){
        const CInventory& inventory = m_ECS.getComponent<CInventory>(m_player);
        int size = inventory.size();
        const int index = inventory.activeItem.index; 
        int newIndex = (index-getMouseState().scroll+size*10) % size;
        updateActiveItem(newIndex);
    }
    auto& inputs = m_ECS.getComponent<CInput>(m_player);
    inputs.direction = {0, 0};
    if (inputs.up){
        inputs.direction.y--;
    } if (inputs.down){
        inputs.direction.y++;
    } if (inputs.left){
        inputs.direction.x--;
    } if (inputs.right){
        inputs.direction.x++;
    } 
}

const Item* Scene_Play::findItemFromJson(const json& itemRef) const
{
    try {
        if (itemRef.is_number_integer()) {
            return &m_inventoryManager.getItem(itemRef.get<int>());
        }
        if (itemRef.is_string()) {
            return m_inventoryManager.findItem(itemRef.get<std::string>());
        }
    }
    catch (const std::exception& exception) {
        std::cerr << "Invalid inventory item reference: " << exception.what() << std::endl;
    }
    return nullptr;
}

void Scene_Play::loadInventoryFromJson(EntityID entity, const json& inventoryJson)
{
    const json itemRefs = inventoryJson.is_object()
        ? inventoryJson.value("items", json::array())
        : inventoryJson;

    int requestedSlotCount = CInventory::DefaultSlotCount;
    if (inventoryJson.is_object()) {
        requestedSlotCount = inventoryJson.value(
            "slots",
            itemRefs.is_array() && !itemRefs.empty()
                ? static_cast<int>(itemRefs.size())
                : CInventory::DefaultSlotCount
        );
    }
    else if (itemRefs.is_array() && !itemRefs.empty()) {
        requestedSlotCount = static_cast<int>(itemRefs.size());
    }

    if (!m_ECS.hasComponent<CInventory>(entity)) {
        m_ECS.addComponent<CInventory>(entity, requestedSlotCount);
    }

    CInventory& inventory = m_ECS.getComponent<CInventory>(entity);
    inventory.items.assign(std::max(1, requestedSlotCount), Item{});
    for (int i = 0; i < inventory.size(); ++i) {
        inventory.items[i].index = i;
    }

    int activeSlot = inventoryJson.is_object()
        ? inventoryJson.value("activeSlot", inventory.activeItem.index)
        : 0;
    activeSlot = std::clamp(activeSlot, 0, inventory.size() - 1);

    if (itemRefs.is_array()) {
        const int count = std::min(
            inventory.size(),
            static_cast<int>(itemRefs.size())
        );
        for (int i = 0; i < count; ++i) {
            if (itemRefs[i].is_null()) {
                continue;
            }

            const Item* item = findItemFromJson(itemRefs[i]);
            if (!item) {
                std::cerr << "Unknown inventory item: " << itemRefs[i].dump() << std::endl;
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
    if (!m_ECS.hasComponent<CInventory>(entity)) {
        return;
    }

    CInventory& inventory = m_ECS.getComponent<CInventory>(entity);
    if (newIndex < 0 || newIndex >= inventory.size()) {
        return;
    }

    inventory.activeItem = inventory.items[newIndex];
    inventory.activeItem.index = newIndex;

    if (m_ECS.hasComponent<CWeapon>(entity)) {
        m_ECS.removeComponent<CWeapon>(entity);
    }

    if (inventory.activeItem.hasWeaponConfig) {
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
    if (!m_ECS.hasComponent<CInventory>(entity)) {
        return 0.0f;
    }

    const Item& activeItem = m_ECS.getComponent<CInventory>(entity).activeItem;
    if (activeItem.id == -1) {
        return 0.0f;
    }
    if (m_ECS.hasComponent<CWeapon>(entity)) {
        return static_cast<float>(m_ECS.getComponent<CWeapon>(entity).range);
    }
    if (activeItem.type == ItemType::Consumable) {
        return DefaultItemUseRange;
    }
    return 0.0f;
}

bool Scene_Play::useActiveConsumable(EntityID entity)
{
    if (!m_ECS.hasComponent<CInventory>(entity)) {
        return false;
    }

    CInventory& inventory = m_ECS.getComponent<CInventory>(entity);
    const int activeIndex = inventory.activeItem.index;
    if (activeIndex < 0 || activeIndex >= inventory.size()) {
        return false;
    }

    Item& activeItem = inventory.items[activeIndex];
    if (activeItem.type != ItemType::Consumable) {
        return false;
    }

    if (activeItem.healing <= 0 || !m_ECS.hasComponent<CHealth>(entity)) {
        return false;
    }

    CHealth& health = m_ECS.getComponent<CHealth>(entity);
    if (health.HP >= health.HP_max) {
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
    if (!m_ECS.hasComponent<CCurrency>(m_player)) {
        m_ECS.addComponent<CCurrency>(m_player);
    }

    m_ECS.getComponent<CCurrency>(m_player).value += amount;
    return true;
}

bool Scene_Play::addCurrencyToPlayer(const Item& item)
{
    return addCurrencyToPlayer(item.currencyValue);
}

void Scene_Play::update() 
{
    m_pause = m_camera.update(m_ECS.getComponent<CTransform>(m_player).pos, m_pause);
    if (!m_pause) 
    {
        sLoader();
        sAI();
        sAttack();
        sMovement();
        sStatus();
        sCollision();
        sAnimation();
        sAudio();
        m_currentFrame++;
    }
    sRender();
    m_ECS.update();
    m_rendererManager.update();
    
    // Debug: Print ECS entity counts
    // std::cout << "ECS State: " << m_ECS.numberOfEntities() << std::endl;
    
    if (m_restart){
        m_game->changeScene("GAMEOVER", std::make_shared<Scene_GameOver>(m_game), true);
        return;
    }
    if (m_storyManager.isStoryFinished()){
        onFinish();
        return;
    }
}

void Scene_Play::sLoader()
{
    Vec2 playerPosition = m_ECS.getComponent<CTransform>(m_player).pos;
    m_levelLoader.update(playerPosition);
}

void Scene_Play::sAI()
{
    Vec2 playerPos = m_ECS.getComponent<CTransform>(m_player).pos;

    for (auto [e, agent, input, transform] : m_ECS.View<CAIAgent, CInput, CTransform>())
    {
        if (e == m_player) {
            continue;
        }

        Vec2 pos = transform.pos;

        // ── Sight check ────────────────────────────────────────────────
        float distToPlayer = (playerPos - pos).length();
        bool  inRange      = agent.sightRange > 0.0f && distToPlayer <= agent.sightRange;
        agent.canSeePlayer = inRange && hasLineOfSight(pos, playerPos);

        if (agent.canSeePlayer) {
            agent.lastKnownPlayerPos = playerPos;
            agent.memoryTimer        = agent.memoryDuration;
        }

        // ── State transitions ──────────────────────────────────────────
        switch (agent.state)
        {
        case AIStateType::Patrol:
            if (agent.canSeePlayer) {
                agent.state = AIStateType::Chase;
            }
            break;

        case AIStateType::Chase:
            if (!agent.canSeePlayer) {
                agent.state           = AIStateType::Investigate;
                agent.hasPatrolTarget = false;   // clear stale patrol target
            }
            break;

        case AIStateType::Investigate:
            if (agent.canSeePlayer) {
                agent.state = AIStateType::Chase;
            } else {
                agent.memoryTimer--;
                // Reached last known position or memory expired
                bool arrived = (pos - agent.lastKnownPlayerPos).length() < 12.0f;
                if (arrived || agent.memoryTimer <= 0) {
                    agent.state           = AIStateType::Patrol;
                    agent.hasPatrolTarget = false;
                    agent.memoryTimer     = 0;
                }
            }
            break;
        }

        // ── Movement input per state ─────────────────────────────────────
        input.direction = {0, 0};

        switch (agent.state)
        {
        case AIStateType::Chase:
            input.direction = (playerPos - pos).norm();
            input.use = distToPlayer < activeItemUseRange(e);
            input.useHeld = input.use;
            break;

        case AIStateType::Investigate:
            input.direction = (agent.lastKnownPlayerPos - pos).norm();
            input.use = false;
            input.useHeld = false;
            break;

        case AIStateType::Patrol:
            tickPatrol(agent, pos, input);
            input.use = false;
            input.useHeld = false;
            break;
        }
    }
}

void Scene_Play::sMovement() {
    // Convert movement intent into a force. Input itself remains an intent-only component.
    for (auto [e, inputs, velocity, body] : m_ECS.View<CInput, CVelocity, CPhysicsBody>()){
        if (e != m_player && m_ECS.hasComponent<CAttackState>(e)) {
            velocity.vel = {0, 0};
            inputs = CInput();
            continue;
        }

        if (!inputs.direction.isNull()) {
            const float sprintMultiplier = inputs.ctrl ? SPRINT_MULTIPLIER : 1.0f;
            body.accumulatedForce += inputs.direction.norm(body.moveForce * sprintMultiplier);
        }
        if (e != m_player) {
            inputs = CInput(); // reset inputs for NPCs after processing
        }
    }

    // Integrate force and exponential linear damping. This produces a stable terminal speed
    // of moveForce / (mass * linearDamping) while movement input is held.
    for (auto [e, velocity, body] : m_ECS.View<CVelocity, CPhysicsBody>()) {
        const Vec2 acceleration = body.accumulatedForce / body.mass;

        if (body.linearDamping > 0.0f) {
            const float decay = std::exp(-body.linearDamping * PHYSICS_DT);
            velocity.vel = velocity.vel * decay
                         + acceleration * ((1.0f - decay) / body.linearDamping);
        } else {
            velocity.vel += acceleration * PHYSICS_DT;
        }

        float maxSpeed = body.maxSpeed;
        if (m_ECS.hasComponent<CInput>(e) && m_ECS.getComponent<CInput>(e).ctrl) {
            maxSpeed *= SPRINT_MULTIPLIER;
        }
        if (velocity.vel.length() > maxSpeed) {
            velocity.vel = velocity.vel.norm(maxSpeed);
        }
        if (velocity.vel.length() < STOP_SPEED) {
            velocity.vel = {0, 0};
        }

        body.accumulatedForce = {0, 0};
    }

    // Bodies and kinematic projectiles both move from their real world-space velocity.
    for (auto [e, transform, velocity] : m_ECS.View<CTransform, CVelocity>()){
        transform.prevPos = transform.pos;
        transform.pos += velocity.vel * PHYSICS_DT;
    }

    for (auto [e, parent, transform] : m_ECS.View<CParent, CTransform>(ecs::Exclude<CStatic>{}))
    {
        transform.pos = m_ECS.getComponent<CTransform>(parent.parent).pos + parent.relativePos;
    }
}

void Scene_Play::startAttack(EntityID attackerID, Vec2 direction, CWeapon& weapon)
{
    if (direction.isNull()) {
        direction = Vec2{1, 0};
    }

    CAttackState& attackState = m_ECS.addComponent<CAttackState>(
        attackerID,
        direction
    );

    if (!weapon.attackAnimation.empty() && m_ECS.hasComponent<CSprite>(attackerID)) {
        attackState.hasAnimationOverride = true;
        attackState.previousSprite = m_ECS.getComponent<CSprite>(attackerID);
        attackState.hadAnimation = m_ECS.hasComponent<CAnimation>(attackerID);
        if (attackState.hadAnimation) {
            attackState.previousAnimation = m_ECS.getComponent<CAnimation>(attackerID);
        }
        setAnimation(attackerID, weapon.attackAnimation, true);
    }

    if (weapon.attackAnimationRow >= 0 && m_ECS.hasComponent<CAnimation>(attackerID)) {
        CAnimation& animation = m_ECS.getComponent<CAnimation>(attackerID);
        animation.currentFrame = 0;
        animation.currentCol = 0;
        animation.currentRow = weapon.attackAnimationRow;
    }

    if (m_ECS.hasComponent<CAnimation>(attackerID)) {
        CAnimation& animation = m_ECS.getComponent<CAnimation>(attackerID);
        animation.currentFrame = 0;
        animation.currentCol = 0;
        attackState.animationFrameCount = static_cast<int>(animation.frameCount);
        attackState.animationFrameDuration = static_cast<int>(animation.frameDuration);
    }

    attackState.attackHitFrame = std::clamp(
        weapon.attackHitFrame,
        0,
        std::max(0, attackState.animationFrameCount - 1)
    );
}

void Scene_Play::finishAttack(EntityID attackerID, CAttackState& attackState, const CWeapon* weapon)
{
    if (attackState.hasAnimationOverride && m_ECS.hasComponent<CSprite>(attackerID)) {
        m_ECS.getComponent<CSprite>(attackerID) = attackState.previousSprite;
        if (attackState.hadAnimation) {
            m_ECS.getComponent<CAnimation>(attackerID) = attackState.previousAnimation;
        }
        else if (m_ECS.hasComponent<CAnimation>(attackerID)) {
            m_ECS.removeComponent<CAnimation>(attackerID);
        }
    }
    else if (weapon != nullptr &&
        weapon->attackAnimationRow >= 0 &&
        m_ECS.hasComponent<CAnimation>(attackerID)) {
        CAnimation& animation = m_ECS.getComponent<CAnimation>(attackerID);
        animation.currentRow = static_cast<int>(PlayerState::STAND);
        animation.currentFrame = 0;
        animation.currentCol = 0;
        if (attackerID == m_player && m_ECS.hasComponent<CState>(attackerID)) {
            CState& state = m_ECS.getComponent<CState>(attackerID);
            state.state = PlayerState::STAND;
            state.preState = PlayerState::STAND;
            state.changeAnimate = true;
        }
    }

    m_ECS.removeComponent<CAttackState>(attackerID);
}

void Scene_Play::sAttack(){
    ComponentPool<CWeapon>& weaponPool = m_ECS.getOrCreateComponentPool<CWeapon>();

    for (auto [id, inputs, inventory, transform] : m_ECS.View<CInput, CInventory, CTransform>()){
        Item& activeItem = inventory.activeItem;

        if (m_ECS.hasComponent<CAttackState>(id)) {
            CAttackState& attack = m_ECS.getComponent<CAttackState>(id);
            if (!m_ECS.hasComponent<CWeapon>(id)) {
                finishAttack(id, attack, nullptr);
                inputs.use = false;
                continue;
            }

            CWeapon& weapon = weaponPool.getComponent(id);
            if (id != m_player) {
                inputs.direction = {0, 0};
            }
            inputs.use = false;

            int attackGameFrame = ++attack.elapsedFrames;
            if (m_ECS.hasComponent<CAnimation>(id)) {
                attackGameFrame = static_cast<int>(m_ECS.getComponent<CAnimation>(id).currentFrame) + 1;
            }

            if (!attack.hasFired && !inputs.useHeld) {
                finishAttack(id, attack, &weapon);
                continue;
            }

            if (!attack.hasFired && attackGameFrame >= attack.hitGameFrame()) {
                Vec2 attackDirection = attack.direction;
                if (id == m_player && weapon.weaponType == WeaponType::Projectile) {
                    attackDirection = (getMousePosition() - transform.pos + getCameraPosition()).norm();
                    if (attackDirection.isNull()) {
                        attackDirection = attack.direction;
                    }
                    attack.direction = attackDirection;
                }
                switch (weapon.weaponType)
                {
                case WeaponType::Melee:
                    spawnHitbox(id, attackDirection, weapon);
                    break;
                case WeaponType::Projectile:
                    spawnProjectile(id, attackDirection, weapon);
                    break;
                case WeaponType::AoE:
                    spawnHitbox(id, attackDirection, weapon);
                    break;
                }
                attack.hasFired = true;
            }

            if (attackGameFrame >= attack.finishGameFrame()) {
                finishAttack(id, attack, &weapon);
            }
            continue;
        }

        if (m_ECS.hasComponent<CWeapon>(id)) {
            weaponPool.getComponent(id).delay--;
        }

        if (!inputs.use){
            continue;
        }

        switch (activeItem.type)
        {
        case ItemType::Consumable:
            useActiveConsumable(id);
            inputs.use = false;
            continue;
        case ItemType::WeaponMelee:
        case ItemType::WeaponRanged:
        case ItemType::WeaponAoE:
            break;
        case ItemType::None:
        case ItemType::Weapon:
        case ItemType::Quest:
        case ItemType::Currency:
            inputs.use = false;
            continue;
        }

        if (!m_ECS.hasComponent<CWeapon>(id)) {
            updateActiveItem(id, activeItem.index);
        }
        if (!m_ECS.hasComponent<CWeapon>(id)) {
            inputs.use = false;
            continue;
        }

        CWeapon& weapon = weaponPool.getComponent(id);
        if (weapon.delay >= 0){
            continue;
        }
        else {
            weapon.delay = weapon.speed;
        }
        Vec2 position = transform.pos;
        Vec2 direction = {0, 0};
        if (m_ECS.hasComponent<CVelocity>(id)) {
            direction = m_ECS.getComponent<CVelocity>(id).vel;
        }
        if (id == m_player){
            direction = (getMousePosition()-position+getCameraPosition()).norm();
        }
        else if (!inputs.direction.isNull()) {
            direction = inputs.direction.norm();
        }
        if (direction.isNull()) {
            direction = Vec2{1, 0};
        }

        startAttack(id, direction, weapon);
        inputs.use = false;
    }
}

void Scene_Play::sCollision() 
{
    m_collisionManager.doCollisions();
}

void Scene_Play::sStatus() {
    for (auto [entityID, lifespan] : m_ECS.View<CLifespan>())
    {   
        lifespan.lifespan--;
        if (lifespan.lifespan <= 0) {
            m_ECS.queueRemoveEntity(entityID);
        }
    }

    for (auto [entityID, lifetime] : m_ECS.View<CActiveHitboxLifetime>())
    {
        lifetime.framesRemaining--;
        if (lifetime.framesRemaining <= 0) {
            m_ECS.queueRemoveComponent<CCollider>(entityID);
            m_ECS.queueRemoveComponent<CDamage>(entityID);
            m_ECS.queueRemoveComponent<CAttackHitbox>(entityID);
            m_ECS.queueRemoveComponent<CActiveHitboxLifetime>(entityID);
        }
    }

    for (auto [entityID, flash] : m_ECS.View<CDamageFlash>())
    {
        if (flash.framesRemaining > 0) {
            flash.framesRemaining--;
        }
    }

    for (auto [entityID, health] : m_ECS.View<CHealth>())
    {
        if (health.HP > 0)
        {
            continue;
        }
        auto& transform = m_ECS.getComponent<CTransform>(entityID);
        if ( m_player == entityID ){
            std::cout << "Player has died!" << std::endl;
            m_restart = true;
            continue;
        }
        const Vec2 gridPos{
            std::floor(transform.pos.x / m_gridSize.x),
            std::floor(transform.pos.y / m_gridSize.y)
        };
        Spawn("coin", gridPos);
        m_ECS.queueRemoveEntity(entityID);
        // m_ECS.addComponent<CAudio>(entityID, "enemy_death_ida");
        m_game->playAudio("enemy_death_ida");
        Emit(Event{EventType::EntityKilled, m_ECS.getComponent<CName>(entityID).name});
    }
}

void Scene_Play::sAnimation() {
    for (auto [e, state, animation, velocity] : m_ECS.View<CState, CAnimation, CVelocity>()){
        bool useStateAnimation = true;
        if (m_ECS.hasComponent<CAttackState>(e)) {
            CAttackState& attackState = m_ECS.getComponent<CAttackState>(e);
            if (attackState.hasAnimationOverride) {
                useStateAnimation = false;
            } else if (m_ECS.hasComponent<CWeapon>(e) &&
                m_ECS.getComponent<CWeapon>(e).attackAnimationRow >= 0) {
                animation.currentRow = m_ECS.getComponent<CWeapon>(e).attackAnimationRow;
                useStateAnimation = false;
            } else {
                Vec2 attackDirection = m_ECS.getComponent<CAttackState>(e).direction;
                if (attackDirection.mainDir().x > 0) {
                    changePlayerState(e, PlayerState::RUN_RIGHT);
                } else if (attackDirection.mainDir().x < 0) {
                    changePlayerState(e, PlayerState::RUN_LEFT);
                } else if (attackDirection.mainDir().y > 0) {
                    changePlayerState(e, PlayerState::RUN_DOWN);
                } else if (attackDirection.mainDir().y < 0) {
                    changePlayerState(e, PlayerState::RUN_UP);
                } else {
                    changePlayerState(e, PlayerState::STAND);
                }
            }
        } else if( velocity.vel.isNull() ) {
            changePlayerState(e, PlayerState::STAND);
        } else if( velocity.vel.mainDir().x > 0 ) {
            changePlayerState(e, PlayerState::RUN_RIGHT);
        } else if(velocity.vel.mainDir().x < 0) {
            changePlayerState(e, PlayerState::RUN_LEFT);
        } else if(velocity.vel.mainDir().y > 0) {
            changePlayerState(e, PlayerState::RUN_DOWN);
        } else if(velocity.vel.mainDir().y < 0) {
            changePlayerState(e, PlayerState::RUN_UP);
        }
        // change player animation
        if (useStateAnimation && state.changeAnimate) {
            animation.currentRow = static_cast<int>(state.state);
        }
    }

    auto& projectilePool = m_ECS.getComponentPool<CProjectile>();
    for (auto [e, projectileState, animation] : m_ECS.View<CProjectileState, CAnimation>()) {
        if (!m_ECS.hasComponent<CProjectile>(e)) {
            continue;
        }

        if (projectileState.phase != ProjectilePhase::Flying) {
            continue;
        }

        auto& projectile = projectilePool.getComponent(e);
        projectile.flightLifetime--;
        if (projectile.flightLifetime <= 0) {
            destroyProjectile(e);
        }
    }

    updateAnimations();
}

void Scene_Play::sRenderHealth() {
    int windowScale = m_game->getScale();

    const SpriteDefinition& heartsSprite = getSprite("hearts");

    auto& playerHealth = m_ECS.getComponent<CHealth>(m_player);
    const float playerHearts = static_cast<float>(playerHealth.HP) / 2.0f;
    const float playerMaxHearts = static_cast<float>(playerHealth.HP_max) / 2.0f;
    const Vec2 playerHeartFrameSize = heartsSprite.frameSize();
    const Vec2 playerHeartSize = playerHeartFrameSize * windowScale;
    const bool playerHasHalfHeart = playerHearts != std::floor(playerHearts);
    const float playerVisibleHeartSlots = std::ceil(playerHearts);
    const RectF playerHeartSource = heartsSprite.sourceRegion();
    const RectF playerSrc = {
        playerHeartSource.x + (10.0f - playerVisibleHeartSlots) * playerHeartFrameSize.x,
        playerHeartSource.y + playerHeartFrameSize.y * static_cast<float>(playerHasHalfHeart),
        playerHeartFrameSize.x * std::ceil(playerMaxHearts),
        playerHeartFrameSize.y
    };
    const RectF playerDst = {
        0.0f,
        0.0f,
        playerHeartSize.x * std::ceil(playerMaxHearts),
        playerHeartSize.y
    };
    drawSprite(heartsSprite, playerSrc, playerDst);

    const RenderView view = worldRenderView();
    const float viewScale = view.scale > 0.0f ? view.scale : 1.0f;
    for (auto [entityID, health, transform] : m_ECS.constView<CHealth, CTransform>())
    {
        if (entityID == m_player) { continue; }
        // if (static_cast<int>(m_currentFrame - health.damage_frame) >= health.i_frames)
        // {
        //     continue;
        // }

        const float hearts = static_cast<float>(health.HP) / 2.0f;
        const float maxHearts = static_cast<float>(health.HP_max) / 2.0f;
        
        const Vec2 heartFrameSize = heartsSprite.frameSize();
        const Vec2 heartSize = heartFrameSize * transform.scale * (static_cast<float>(windowScale) / viewScale);
        const bool hasHalfHeart = hearts != std::floor(hearts);
        const float visibleHeartSlots = std::ceil(hearts);
        const RectF heartSource = heartsSprite.sourceRegion();
        const RectF src = {
            heartSource.x + (10.0f - visibleHeartSlots) * heartFrameSize.x,
            heartSource.y + heartFrameSize.y * static_cast<float>(hasHalfHeart),
            heartFrameSize.x * std::ceil(maxHearts),
            heartFrameSize.y
        };
        const CSprite& entitySprite = m_ECS.getComponent<CSprite>(entityID);
        const float entityVisualHeight = entitySprite.size().y * transform.scale.y;
        const RectF dst = {
            transform.pos.x - std::ceil(maxHearts) * heartSize.x / 2.0f,
            transform.pos.y - entityVisualHeight / 2.0f - heartSize.y / 2.0f,
            heartSize.x * std::ceil(maxHearts),
            heartSize.y
        };
        drawWorldSprite(heartsSprite, src, dst);
    }
}

void Scene_Play::sRenderCurrency() {
    int windowScale = m_game->getScale();

    const SpriteDefinition& coinSprite = getSprite("coin");
    const int currency = m_ECS.hasComponent<CCurrency>(m_player)
        ? m_ECS.getComponent<CCurrency>(m_player).value
        : 0;
    const std::string currencyText = std::to_string(currency);
    const Vec2 coinSize = coinSprite.frameSize() * windowScale;
    const float margin = 4.0f * windowScale;
    const float gap = 3.0f * windowScale;
    const float textHeight = std::max(8.0f * windowScale, coinSize.y * 0.55f);
    const float textWidth = std::max(
        8.0f * windowScale,
        static_cast<float>(currencyText.length()) * textHeight * 0.5f
    );
    const float currencyWidth = coinSize.x + gap + textWidth;
    const float currencyX = static_cast<float>(width() * windowScale) - currencyWidth - margin;
    drawSprite(coinSprite, RectF{
        currencyX,
        margin,
        coinSize.x,
        coinSize.y
    });
    m_game->render().drawText(TextDrawCommand{
        currencyText,
        "Minecraft",
        RectF{
            currencyX + coinSize.x + gap,
            margin + (coinSize.y - textHeight) / 2.0f,
            textWidth,
            textHeight
        },
        {255, 255, 255, 255}
    });
}

void Scene_Play::sRenderInventory() {
    int windowScale = m_game->getScale();

    // render player inventory
    const SpriteDefinition& inventorySprite = getSprite("inventory");
    Vec2 inventorySize = inventorySprite.frameSize() * windowScale;
    drawSprite(inventorySprite, RectF{
        width()/2.0f*windowScale - inventorySize.x/2,
        0.0f,
        inventorySize.x,
        inventorySize.y
    });
    auto& inventory = m_ECS.getComponent<CInventory>(m_player);
    auto& items = inventory.items;
    auto activeItemIndex = inventory.activeItem.index;
    int slotIndex = -1;
    for (Item& item: items){
        slotIndex++;
        if (slotIndex >= inventory.size()) {
            break;
        }
        if (item.index==activeItemIndex){
            const SpriteDefinition& activeItemSprite = getSprite("activeItemInventory");
            Vec2 activeSize = activeItemSprite.frameSize() * windowScale;
            drawSprite(activeItemSprite, RectF{
                (width()/2.0f + slotIndex*32.0f)*windowScale - inventorySize.x/2,
                0.0f,
                activeSize.x,
                activeSize.y
            });
        }
        if (item.id==-1){continue;}
        const SpriteDefinition& itemSprite = getSprite(item.iconPath);
        Vec2 itemSize = itemSprite.frameSize() * windowScale;
        drawSprite(
            itemSprite,
            itemSprite.firstFrame(),
            RectF{
                (width()/2.0f + slotIndex*32.0f)*windowScale - inventorySize.x/2,
                0.0f,
                itemSize.x,
                itemSize.y
            }
        );
    }
}

void Scene_Play::sRenderUI() {
    sRenderHealth();
    sRenderCurrency();
    sRenderInventory();
}

void Scene_Play::sRender() {    
    sRenderBasic();
    sRenderUI();
    
    if (m_drawCollision)
    {
        m_levelLoader.renderChunkGrid(m_game->render());
        m_collisionManager.renderQuadtree(m_game->render());
    }

}

void Scene_Play::sAudio()
{
    for (auto [id, audio] : m_ECS.constView<CAudio>()){
        m_game->playAudio(audio.audioName);
        m_ECS.queueRemoveComponent<CAudio>(id);
    }
}

EntityID Scene_Play::SpawnFromJSON(std::string name, Vec2 pos)
{
    const Item* item = m_inventoryManager.findItem(name);
    std::ifstream file;
    std::string definitionName = name;

    for (const std::string& directory : {"config_files/mobs", "config_files/entities"}) {
        file.open(directory + "/" + name + ".json");
        if (file.is_open()) {
            break;
        }
        file.clear();
    }

    if (!file.is_open() && item) {
        file.open("config_files/entities/item.json");
        definitionName = "item";
    }

    if (!file.is_open()) {
        std::cerr << "Could not load entity spawn json file for: " << name << std::endl;
        return -1;
    }

    json j;
    file >> j;
    file.close();
    if (!j.contains(definitionName) || !j[definitionName].contains("components")) {
        std::cerr << "Invalid entity spawn json file for: " << name << std::endl;
        return -1;
    }

    const json& definition = j[definitionName];
    json c = definition["components"];
    if (item && definitionName == "item") {
        c["CName"] = item->name;
        c["CItem"]["itemID"] = item->id;
        c["CAnimation"]["animation"] = item->iconPath;
    }

    bool hasEvent = false;
    Event event;
    if (definition.contains("event")) {
        const json& eventConfig = definition["event"];
        if (!eventConfig.is_object() || !eventConfig.contains("type") || !eventConfig.contains("subject")) {
            std::cerr << "Invalid event directive for: " << name << std::endl;
            return -1;
        }

        try {
            event = Event{
                m_storyManager.getEventTypeFromString(eventConfig.at("type").get<std::string>()),
                eventConfig.at("subject").get<std::string>()
            };
            hasEvent = true;
        }
        catch (const std::exception& exception) {
            std::cerr << "Invalid event directive for " << name << ": " << exception.what() << std::endl;
            return -1;
        }
    }
    
    EntityID id = m_ECS.addEntity();
    
    m_ECS.addComponent<CName>(id, c.value("CName", name));
    if (c.contains("CAnimation")){
        addVisual(
            id,
            c["CAnimation"]["animation"].get<std::string>(),
            renderLayerFromJson(c["CAnimation"]["layer"])
        );
    }
    if (c.contains("CTransform")){

        pos = gridToMidPixel(pos, id);
        c["CTransform"]["pos"] = {{"x", pos.x}, {"y", pos.y}};
        m_ECS.addComponent<CTransform>(id, c["CTransform"]);
    }
    if (c.contains("CPhysicsBody")) {
        m_ECS.addComponent<CPhysicsBody>(id, c["CPhysicsBody"]);
        m_ECS.addComponent<CVelocity>(id);
    }
    if (c.contains("CItem")) {
        m_ECS.addComponent<CItem>(id, c["CItem"].at("itemID").get<int>());
    }
    if (c.contains("CCollider")){
        m_ECS.addComponent<CCollider>(id, c["CCollider"]);
    }
    if (c.contains("CState")){
        m_ECS.addComponent<CState>(id);
    }
    if (c.contains("CHealth")){
        m_ECS.addComponent<CHealth>(id, c["CHealth"]);
    }
    if (c.contains("CPossesLevel")){
        m_ECS.addComponent<CPossesLevel>(id, c["CPossesLevel"]);
    }
    if (c.contains("CInput")){
        m_ECS.addComponent<CInput>(id);
    }
    if (c.contains("CInventory")) {
        loadInventoryFromJson(id, c["CInventory"]);
    }
    if (c.contains("CCurrency")) {
        m_ECS.addComponent<CCurrency>(id, c["CCurrency"]);
    }
    if (c.contains("CAIAgent")) {
        if (!m_ECS.hasComponent<CInput>(id)) {
            m_ECS.addComponent<CInput>(id);
        }
        m_ECS.addComponent<CAIAgent>(id, c["CAIAgent"]);
        // Record spawn position for patrol anchor
        m_ECS.getComponent<CAIAgent>(id).spawnPos = pos;
    }
    if (hasEvent) {
        m_ECS.addComponent<CEvent>(id, event);
    }
    if (m_ECS.hasComponent<CTransform>(id) && m_ECS.hasComponent<CSprite>(id)) {
        spawnShadow(id);
    }
    return id;
}

EntityID Scene_Play::Spawn(std::string name, Vec2 pos)
{
    return SpawnFromJSON(name, pos * m_gridSize);
}

EntityID Scene_Play::DropItem(const Item& item, Vec2 position)
{
    if (item.id == -1) {
        return static_cast<EntityID>(-1);
    }

    EntityID droppedID = SpawnFromJSON(item.name, position);
    if (droppedID == static_cast<EntityID>(-1)) {
        return droppedID;
    }

    if (m_ECS.hasComponent<CTransform>(droppedID)) {
        CTransform& transform = m_ECS.getComponent<CTransform>(droppedID);
        transform.pos = position;
        transform.prevPos = position;
    }

    if (m_ECS.hasComponent<CItem>(droppedID)) {
        CItem& droppedItem = m_ECS.getComponent<CItem>(droppedID);
        droppedItem.hasPickupModeOverride = true;
        droppedItem.pickupModeOverride = PickupMode::Manual;
    }

    return droppedID;
}

EntityID Scene_Play::spawnPlayer()
{
    json save;
    json playerSave;
    m_playerDefinition = "player";

    if (!m_newGame){
        save = loadJsonFile("config_files/game_save.json");
        playerSave = save.at("player");
        m_playerDefinition = playerSave.value("definition", "player");
    }

    const std::string definitionPath = "config_files/entities/" + m_playerDefinition + ".json";
    json playerDefinitionJson = loadJsonFile(definitionPath);
    const json& playerDefinition = playerDefinitionJson.at(m_playerDefinition);
    Vec2 spawnGrid = playerDefinition.at("spawn");
    if (playerSave.contains("position")) {
        spawnGrid = playerSave.at("position");
    }

    EntityID entityID = Spawn(m_playerDefinition, spawnGrid);
    if (entityID == static_cast<EntityID>(-1)) {
        throw std::runtime_error("Could not spawn player from definition: " + m_playerDefinition);
    }
    m_player = entityID;

    if (playerSave.contains("hp") && m_ECS.hasComponent<CHealth>(entityID)) {
        m_ECS.getComponent<CHealth>(entityID).HP = playerSave.at("hp").get<int>();
    }

    if (!m_ECS.hasComponent<CCurrency>(entityID)) {
        m_ECS.addComponent<CCurrency>(entityID);
    }
    m_ECS.getComponent<CCurrency>(entityID).value = playerSave.contains("currency")
        ? playerSave.at("currency").get<int>()
        : 0;

    if (!m_ECS.hasComponent<CInventory>(entityID)) {
        m_ECS.addComponent<CInventory>(entityID);
    }

    if (playerSave.contains("inventory")) {
        const json& inventorySave = playerSave.at("inventory");
        loadInventoryFromJson(entityID, inventorySave);
    }
    else if (save.contains("inventory")) {
        loadInventoryFromJson(entityID, save.at("inventory"));
    }

    const auto& inventory = m_ECS.getComponent<CInventory>(entityID);
    if (inventory.activeItem.index >= 0 &&
        inventory.activeItem.index < inventory.size()) {
        updateActiveItem(entityID, inventory.activeItem.index);
    }

    return entityID;
}

EntityID Scene_Play::spawnShadow(EntityID parentID){
    const CTransform& parentTransform = m_ECS.getComponent<CTransform>(parentID);
    const CSprite& parentSprite = m_ECS.getComponent<CSprite>(parentID);
    const SpriteDefinition& shadowSprite = getSprite("shadow");

    const Vec2 parentSize = parentSprite.size() * parentTransform.scale;
    const Vec2 shadowSize = shadowSprite.frameSize();
    const float shadowScale = parentSize.x / shadowSize.x;
    const Vec2 scaledShadowSize = shadowSize * shadowScale;
    const Vec2 relativePos{0, parentSize.y / 2.0f - scaledShadowSize.y / 2.0f};

    auto shadowID = m_ECS.addEntity();
    m_ECS.addComponent<CTransform>(shadowID);
    m_ECS.getComponent<CTransform>(shadowID).scale = {shadowScale, shadowScale};
    addSprite(shadowID, "shadow", parentSprite.layer - 1);
    m_ECS.attachChild(parentID, shadowID, relativePos);
    return shadowID;
}

EntityID Scene_Play::spawnProjectile(EntityID attackerID, Vec2 vel, const CWeapon& weapon)
{
    auto id = m_ECS.addEntity();
    const int layer = RenderLayer::Projectile;
    const float speed = 200.0f;
    const int flightLifetime = 60;
    const float createOffset = 12.0f;
    const Vec2 startPos = m_ECS.getComponent<CTransform>(attackerID).pos;

    Vec2 direction = vel.norm();
    if (direction.isNull()) {
        direction = Vec2{1, 0};
    }

    addVisual(id, "fireball", layer, true);
    m_ECS.addComponent<CTransform>(id, startPos + direction * createOffset, direction.angle());
    m_ECS.addComponent<CProjectile>(
        id,
        attackerID,
        direction,
        speed,
        flightLifetime,
        createOffset,
        weapon.targetMask
    );
    m_ECS.addComponent<CProjectileState>(id, ProjectilePhase::Flying);
    m_ECS.addComponent<CDamage>(id, weapon.damage);
    m_ECS.getComponent<CDamage>(id).damageType = {"Fire", "Explosive"};
    m_ECS.addComponent<CVelocity>(id, direction.norm(speed));
    m_ECS.addComponent<CCollider>(
        id,
        Vec2{6, 6},
        PROJECTILE_LAYER,
        weapon.targetMask | OBSTACLE_LAYER
    );
    m_ECS.addComponent<CAudio>(id, "fireball_shot");
    m_camera.startShake(2, 60);
    return id;
}

void Scene_Play::destroyProjectile(EntityID projectileID)
{
    if (!m_ECS.hasComponent<CProjectileState>(projectileID)) {
        return;
    }

    auto& projectileState = m_ECS.getComponent<CProjectileState>(projectileID);
    if (projectileState.phase == ProjectilePhase::Destroying) {
        return;
    }

    projectileState.phase = ProjectilePhase::Destroying;
    if (m_ECS.hasComponent<CParent>(projectileID)) {
        m_ECS.detachChild(projectileID);
    }
    if (m_ECS.hasComponent<CVelocity>(projectileID)) {
        m_ECS.getComponent<CVelocity>(projectileID).vel = Vec2{0, 0};
    }
    if (m_ECS.hasComponent<CCollider>(projectileID)) {
        m_ECS.queueRemoveComponent<CCollider>(projectileID);
    }

    setAnimation(projectileID, "fireball_explode", false);
    m_ECS.addComponent<CAudio>(projectileID, "fireball_destroy");
}

EntityID Scene_Play::spawnHitbox(EntityID attackerID, Vec2 direction, const CWeapon& weapon)
{
    auto id = m_ECS.addEntity();
    const int renderLayer = RenderLayer::AttackEffect;
    if (direction.isNull()) {
        direction = Vec2{1, 0};
    }

    const Vec2 attackerPosition = m_ECS.getComponent<CTransform>(attackerID).pos;
    const Vec2 hitboxSize = Vec2{weapon.range, weapon.range};
    Vec2 hitboxPosition = attackerPosition;

    if (!weapon.hitboxAnimation.empty()) {
        addVisual(id, weapon.hitboxAnimation, renderLayer, false);
    }
    else {
        addSprite(id, weapon.hitboxSprite, renderLayer);
        hitboxPosition += direction.norm((float)weapon.range / 2.0f);
    }

    m_ECS.addComponent<CTransform>(id, hitboxPosition);
    m_ECS.addComponent<CDamage>(id, weapon.damage);
    m_ECS.addComponent<CAttackHitbox>(id, attackerID);
    m_ECS.addComponent<CCollider>(
        id,
        hitboxSize,
        DAMAGE_LAYER,
        weapon.targetMask,
        Color{0, 0, 255, 255},
        true
    );

    if (weapon.hitboxAnimation.empty()) {
        m_ECS.addComponent<CLifespan>(id, std::max(1, weapon.hitboxActiveFrames));
    }
    else {
        m_ECS.addComponent<CActiveHitboxLifetime>(id, std::max(1, weapon.hitboxActiveFrames));
    }
    return id;
}

std::vector<EntityID> Scene_Play::spawnDualTiles(const Vec2 pos, std::array<int, 5> tileTextures)
{   
    std::vector<EntityID> entityIDs;
    for (int i = 0; i < tileTextures.size(); ++i) {
        TileType tileKey = static_cast<TileType>(i);
        int textureIndex = tileTextures[i];
        if (textureIndex == 0) {
            continue;
        }
        int layer = RenderLayer::TerrainTilesLow;
        std::string tile_name = "grass";
        
        if (tileKey == TileType::WATER) {
            tile_name = "water";
        } else if (tileKey == TileType::DIRT) {
            layer = RenderLayer::TerrainTilesHigh;
            tile_name = "dirt";
        } else if (tileKey == TileType::OBSTACLE) {
            layer = RenderLayer::TerrainTilesHigh;
            tile_name = "mountain";
        }

        EntityID entity = m_ECS.addEntity();
        entityIDs.push_back(entity);
        const std::string spriteName = tile_name + "_dual_sheet";
        CSprite& sprite = addSprite(entity, spriteName, layer);
        Vec2 tilePosition = Vec2{
            static_cast<float>(textureIndex % 4),
            static_cast<float>(textureIndex / 4)
        };
        sprite.src = getSprite(spriteName).frameRect(
            static_cast<int>(tilePosition.x),
            static_cast<int>(tilePosition.y)
        );
        if (tileKey == TileType::WATER) {
            CAnimation& animation = m_ECS.addComponent<CAnimation>(entity, getSprite(spriteName), true);
            animation.currentCol = static_cast<int>(tilePosition.x);
            animation.currentRow = static_cast<int>(tilePosition.y);
        }
        Vec2 midGrid = gridToMidPixel(pos, entity);
        m_ECS.addComponent<CTransform>(entity, midGrid);
    }
    return entityIDs;
}

void Scene_Play::changePlayerState(EntityID entity, PlayerState s) {
    auto& prev = m_ECS.getComponent<CState>(entity).preState; 
    if (prev != s) {
        prev = m_ECS.getComponent<CState>(entity).state;
        m_ECS.getComponent<CState>(entity).state = s; 
        m_ECS.getComponent<CState>(entity).changeAnimate = true;
    }
    else { 
        m_ECS.getComponent<CState>(entity).changeAnimate = false;
    }
}

void Scene_Play::onEnd() {
    m_game->changeScene("MAIN_MENU", std::make_shared<Scene_Menu>(m_game));
}

void Scene_Play::onFinish() {
    std::cout << "Warning, removing scene_play instance" << std::endl;
    m_game->changeScene("FINISH", std::make_shared<Scene_Finish>(m_game), true);
}

void Scene_Play::OnPlayerDeath() {
    std::cout << "Warning, removing scene_play instance" << std::endl;
    m_game->changeScene("GAMEOVER", std::make_shared<Scene_GameOver>(m_game), true);
}

void Scene_Play::setPaused(bool pause) {
    m_pause = pause;
}

void Scene_Play::togglePause() {
    m_pause = !m_pause;
}

Vec2 Scene_Play::getCameraPosition() {
    return m_camera.position;
}

EntityID Scene_Play::changePlayerID(EntityID otherID) {
    m_player = otherID;
    return m_player;
}

bool Scene_Play::rayIntersectsAABB(
    Vec2 origin, Vec2 dir, float maxDist,
    Vec2 boxMin, Vec2 boxMax)
{
    float tMin = 0.0f;
    float tMax = maxDist;

    // Test X slab
    if (std::abs(dir.x) < 1e-6f) {
        if (origin.x < boxMin.x || origin.x > boxMax.x) return false;
    } else {
        float t1 = (boxMin.x - origin.x) / dir.x;
        float t2 = (boxMax.x - origin.x) / dir.x;
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) return false;
    }

    // Test Y slab
    if (std::abs(dir.y) < 1e-6f) {
        if (origin.y < boxMin.y || origin.y > boxMax.y) return false;
    } else {
        float t1 = (boxMin.y - origin.y) / dir.y;
        float t2 = (boxMax.y - origin.y) / dir.y;
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) return false;
    }

    return tMin <= tMax;
}

bool Scene_Play::hasLineOfSight(Vec2 origin, Vec2 target)
{
    Vec2  delta   = target - origin;
    float dist    = delta.length();
    if (dist <= 0.0001f) {
        return true;
    }
    Vec2  dir     = delta / dist;          // normalized

    for (auto [obstacle, collider, transform, _] : m_ECS.constView<CCollider, CTransform, CStatic>()) {
        for (const auto& shape : collider.shapes) {
            if (shape.isTrigger || (shape.layer & OBSTACLE_LAYER) == 0) {
                continue;
            }

            Vec2 pos = transform.pos + shape.offset;
            Vec2 boxMin = pos - shape.halfSize;
            Vec2 boxMax = pos + shape.halfSize;

            if (rayIntersectsAABB(origin, dir, dist, boxMin, boxMax)) {
                return false;
            }
        }
    }
    return true;
}

void Scene_Play::tickPatrol(CAIAgent& agent, Vec2 pos, CInput& input)
{
    // Waiting at current patrol point
    if (agent.patrolWaitTimer > 0) {
        agent.patrolWaitTimer--;
        input.direction = {0, 0};
        return;
    }

    // Pick a new random patrol target if we don't have one
    if (!agent.hasPatrolTarget) {
        // Random point in a circle around spawn using rejection sampling
        Vec2 offset;
        do {
            float rx = ((rand() % 200) - 100) / 100.0f; // -1 to 1
            float ry = ((rand() % 200) - 100) / 100.0f;
            offset = Vec2{rx, ry} * agent.patrolRadius;
        } while (offset.length() > agent.patrolRadius);

        agent.patrolTarget    = agent.spawnPos + offset;
        agent.hasPatrolTarget = true;
    }

    // Move toward patrol target
    Vec2  toTarget = agent.patrolTarget - pos;
    float dist     = toTarget.length();

    if (dist < 8.0f) {
        // Arrived — wait before picking next point
        agent.hasPatrolTarget = false;
        agent.patrolWaitTimer = agent.patrolWaitDuration;
        input.direction  = {0, 0};
    } else {
        input.direction = toTarget.norm();
    }
}
