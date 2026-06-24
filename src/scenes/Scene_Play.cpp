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
}

Scene_Play::Scene_Play(Game* game, std::string levelPath, bool newGame)
    : Scene(game), 
    m_levelPath(levelPath), 
    m_collisionManager(&m_ECS, this), 
    m_interactionManager(&m_ECS, this), 
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
    registerAction(InputCode::MouseLeft, "ATTACK");
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
    registerAction(InputCode::F4, "TOGGLE_INTERACTION");
    registerAction(InputCode::F5, "TOGGLE_TEXTURE");
    registerAction(InputCode::Num1, "Slot1");
    registerAction(InputCode::Num2, "Slot2");
    registerAction(InputCode::Num3, "Slot3");
    registerAction(InputCode::Num7, "TP1");
    registerAction(InputCode::Num8, "TP2");
    registerAction(InputCode::Num9, "TP3");

    loadConfig("config_files/config.txt");
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

void Scene_Play::loadConfig(const std::string& confPath){
    // TODO: line count fix be using json
    std::ifstream file(confPath);
    if (!file) {
        throw std::runtime_error("Could not load config file: " + confPath);
    }
    std::string head;
    while (file >> head) {
        if (head == "Player") {
            file >> m_playerConfig.x >> m_playerConfig.y
                 >> m_playerConfig.moveForce >> m_playerConfig.maxSpeed
                 >> m_playerConfig.mass >> m_playerConfig.linearDamping
                 >> m_playerConfig.HP >> m_playerConfig.DAMAGE;
        }
        else if (head == "Rooter") {
            float unusedSpeed;
            file >> unusedSpeed >> m_rooterConfig.ATTACK_SPEED >> m_rooterConfig.HP >> m_rooterConfig.DAMAGE;
        }
        else if (head == "Goblin") {
            float unusedSpeed;
            file >> unusedSpeed >> m_goblinConfig.ATTACK_SPEED >> m_goblinConfig.HP >> m_goblinConfig.DAMAGE;
        }
        else if (head == "Camera") {
            file >> m_camera.config.SHAKE_DURATION_SMALL >> m_camera.config.SHAKE_INTENSITY_SMALL;
        }
        else {
            throw std::runtime_error("Invalid config entry: " + head + ". Config file format is incorrect.");
        }
    }
}

// Function to save the game state to a file
void Scene_Play::saveGame(const std::string& filename) 
{
    Vec2 playerPos = m_ECS.getComponent<CTransform>(m_player).pos;
    int hp = m_ECS.getComponent<CHealth>(m_player).HP;

    std::ofstream file("config_files/game_save.json");
    if (!file) {
        throw std::runtime_error("Could not open game save file: config_files/game_save.json");
    }    
    json save = {
        {"player", {
            {"position", {
                {"x", int(playerPos.x / m_gridSize.x)},
                {"y", int(playerPos.y / m_gridSize.y)}
            }},
            {"hp", hp}
        }},
        {"inventory", {1, 2}}
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
        if ( action.name() == "ATTACK"){m_ECS.getComponent<CInput>(m_player).attack = true;}
    }
    else if ( action.type() == "END")
    {
        if ( action.name() == "TOGGLE_TEXTURE") { m_drawTextures = !m_drawTextures; }
        if ( action.name() == "TOGGLE_COLLISION") { m_drawCollision = !m_drawCollision; }
        if ( action.name() == "TOGGLE_INTERACTION") { m_drawInteraction = !m_drawInteraction; }
        if ( action.name() == "PAUSE") { togglePause(); }
        if ( action.name() == "FPS COUNTER") { m_game->toggleRenderFPS(); }
        if ( action.name() == "INVENTORY") { std::cout << "toggle inventory" << std::endl; }
        if ( action.name() == "ZOOM IN"){ m_camera.stepCameraZoom(-1, m_game->getScale()); }
        if ( action.name() == "ZOOM OUT"){ m_camera.stepCameraZoom(1, m_game->getScale()); }
        if ( action.name() == "CAMERA FOLLOW"){ m_camera.toggleCameraFollow(); }
        if ( action.name() == "CAMERA PAN"){ m_pause = m_camera.startPan(2048, 1000, Vec2{0,0}, m_pause); }
        if ( action.name() == "SAVE"){ saveGame("config_files/game_save.txt"); }
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
        if ( action.name() == "ATTACK") { m_ECS.getComponent<CInput>(m_player).attack = false; }
        if ( action.name() == "WRITE POSITION") { 
            Vec2 cursorPosition = (m_mousePosition+m_camera.position)/m_gridSize;
            cursorPosition.print("Cursor position");
        }
        if ( action.name() == "ESC") {
            m_game->changeScene("SETTINGS", std::make_shared<Scene_Pause>(m_game), false);
            saveGame("config_files/game_save.txt");
            m_pause = true;
        }
    }
    else if ( action.name() == "SCROLL"){
        const CInventory& inventory = m_ECS.getComponent<CInventory>(m_player);
        int size = inventory.items.size();
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

void Scene_Play::updateActiveItem(int newIndex){
    CInventory& inventory = m_ECS.getComponent<CInventory>(m_player);
    inventory.activeItem = inventory.items[newIndex];
    Item& activeItem = inventory.activeItem;

    m_ECS.removeComponent<CWeapon>(m_player);

    switch (activeItem.type)
    {
    case ItemType::WeaponMelee:
        m_ECS.addComponent<CWeapon>(m_player, activeItem.damage, 60, 180, WeaponType::Melee);
        break;
    case ItemType::WeaponRanged:
        m_ECS.addComponent<CWeapon>(m_player, activeItem.damage, 60, 180, WeaponType::Projectile);
        break;
    case ItemType::Consumable:
        std::cout << "Consumable" << std::endl;
        // useConsumable(activeItem);
        break;
    }
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
        sInteraction();
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
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& inputPool = m_ECS.getComponentPool<CInput>();
    auto& agentPool = m_ECS.getComponentPool<CAIAgent>();
    auto  namePool = m_ECS.getComponentPool<CName>();

    Vec2 playerPos = transformPool.getComponent(m_player).pos;

    auto viewAgents = m_ECS.View<CAIAgent, CInput, CTransform>();
    for (EntityID e : viewAgents)
    {
        auto& agent = agentPool.getComponent(e);
        auto& input = inputPool.getComponent(e);
        Vec2  pos = transformPool.getComponent(e).pos;

        // ── Sight check ────────────────────────────────────────────────
        float distToPlayer = (playerPos - pos).length();
        bool  inRange      = distToPlayer <= agent.sightRange;
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
            input.attack        = distToPlayer < 32.0f;   // melee range
            break;

        case AIStateType::Investigate:
            input.direction = (agent.lastKnownPlayerPos - pos).norm();
            break;

        case AIStateType::Patrol:
            tickPatrol(agent, pos, input);
            break;
        }
    }
}

void Scene_Play::sMovement() {
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& velocityPool = m_ECS.getComponentPool<CVelocity>();
    auto& inputPool = m_ECS.getComponentPool<CInput>();
    auto& bodyPool = m_ECS.getComponentPool<CPhysicsBody>();

    // Convert movement intent into a force. Input itself remains an intent-only component.
    const auto viewInputs = m_ECS.View<CInput, CVelocity, CPhysicsBody>();
    for (auto e : viewInputs){
        auto& inputs = inputPool.getComponent(e);
        auto& body = bodyPool.getComponent(e);

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
    const auto viewBodies = m_ECS.View<CVelocity, CPhysicsBody>();
    for (auto e : viewBodies) {
        auto& velocity = velocityPool.getComponent(e);
        auto& body = bodyPool.getComponent(e);
        const Vec2 acceleration = body.accumulatedForce / body.mass;

        if (body.linearDamping > 0.0f) {
            const float decay = std::exp(-body.linearDamping * PHYSICS_DT);
            velocity.vel = velocity.vel * decay
                         + acceleration * ((1.0f - decay) / body.linearDamping);
        } else {
            velocity.vel += acceleration * PHYSICS_DT;
        }

        float maxSpeed = body.maxSpeed;
        if (m_ECS.hasComponent<CInput>(e) && inputPool.getComponent(e).ctrl) {
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
    const auto viewTransform = m_ECS.View<CTransform, CVelocity>();
    for (auto e : viewTransform){
        auto &transform = transformPool.getComponent(e);
        auto &velocity = velocityPool.getComponent(e);

        transform.prevPos = transform.pos;
        transform.pos += velocity.vel * PHYSICS_DT;
    }

    auto viewParent = m_ECS.View<CParent, CTransform>();
    auto& parentPool = m_ECS.getOrCreateComponentPool<CParent>();
    for (auto e : viewParent)
    {
        auto& transform = transformPool.getComponent(e);
        auto& parent = parentPool.getComponent(e);
        transform.pos = transformPool.getComponent(parent.parent).pos + parent.relativePos;
    }
}

void Scene_Play::sAttack(){
    ComponentPool<CTransform>& transformPool = m_ECS.getComponentPool<CTransform>();
    ComponentPool<CVelocity>& velocityPool = m_ECS.getComponentPool<CVelocity>();
    ComponentPool<CInput>& inputPool = m_ECS.getComponentPool<CInput>();
    ComponentPool<CWeapon>& weaponPool = m_ECS.getComponentPool<CWeapon>();

    std::vector<EntityID> viewAttack = m_ECS.View<CInput, CWeapon, CVelocity, CTransform>();
    for (EntityID id : viewAttack){
        CTransform& transform = transformPool.getComponent(id);
        CVelocity& velocity = velocityPool.getComponent(id);
        CInput& inputs = inputPool.getComponent(id);
        CWeapon& weapon = weaponPool.getComponent(id);

        weapon.delay--;
        if (!inputs.attack){
            continue;
        }
        if (weapon.delay >= 0){
            continue;
        }
        else {
            weapon.delay = weapon.speed;
        }
        Vec2 position = transformPool.getComponent(m_player).pos;
        Vec2 direction = velocityPool.getComponent(m_player).vel;
        if (id == m_player){
            direction = (getMousePosition()-position+getCameraPosition()).norm();
        }
        switch (weapon.weaponType)
        {
        case WeaponType::Melee:
            spawnHitbox(position, direction, PROJECTILE_LAYER, ENEMY_LAYER);
            break;
        case WeaponType::Projectile:
            spawnProjectile(position, direction);
            break;
        case WeaponType::AoE:
            spawnHitbox(position, direction, PROJECTILE_LAYER, ENEMY_LAYER);
            break;
        }
        inputs.attack = false;
    }
}

void Scene_Play::sInteraction()
{
    auto screenSize = Vec2{(float)width(), (float)height()};
    Vec2 treePos = m_camera.position + screenSize/2 - Vec2{32, 32};
    Vec2 treeSize = Vec2{1048, 1048};

    m_interactionManager.doInteractions(treePos, treeSize);
}

void Scene_Play::sCollision() 
{
    auto screenSize = Vec2{(float)width(), (float)height()};
    Vec2 treePos = m_camera.position + screenSize/2 - Vec2{32, 32};
    Vec2 treeSize = Vec2{1048, 1048};

    m_collisionManager.doCollisions(treePos, treeSize);
    
}

void Scene_Play::sStatus() {
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
  
    auto& lifespanPool = m_ECS.getComponentPool<CLifespan>();
    auto viewLifespan = m_ECS.View<CLifespan>();
    for ( auto entityID : viewLifespan)
    {   
        auto& lifespan = lifespanPool.getComponent(entityID).lifespan;
        lifespan--;
        if (lifespan <= 0) {
            m_ECS.queueRemoveEntity(entityID);
        }
    }

    auto viewHealth = m_ECS.View<CHealth>();
    auto& healthPool = m_ECS.getComponentPool<CHealth>();
    for ( auto entityID : viewHealth)
    {
        auto& health = healthPool.getComponent(entityID);
        if (health.HP > 0)
        {
            continue;
        }
        auto& transform = transformPool.getComponent(entityID);
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

    auto view = m_ECS.View<CState, CAnimation, CVelocity>();

    auto& velocityPool = m_ECS.getComponentPool<CVelocity>();
    auto& statePool = m_ECS.getComponentPool<CState>();
    auto& animationPool = m_ECS.getComponentPool<CAnimation>();
    for ( auto e : view ){
        auto& velocity = velocityPool.getComponent(e);
        auto& state = statePool.getComponent(e);
        auto& animation = animationPool.getComponent(e);
        if( velocity.vel.isNull() ) {
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
        // // change player animation
        if (state.changeAnimate) {
            animation.currentRow = (int)state.state;
        }
    }

    auto viewProjectileState = m_ECS.View<CProjectileState, CAnimation>();
    auto& projectileStatePool = m_ECS.getComponentPool<CProjectileState>();
    for ( auto e : viewProjectileState ) {
        auto& animation = animationPool.getComponent(e);
        auto& projectileState = projectileStatePool.getComponent(e);
        if ( projectileState.state == "Create" ) {
            if ( animation.hasEnded() ) {
                projectileState.state = "Ready";
                setAnimation(e, "fireball", true);
                m_camera.startShake(50, 100);
            }
        }
    }

    updateAnimations();
}

void Scene_Play::sRenderHealth() {
    int windowScale = m_game->getScale();

    const SpriteDefinition& heartsSprite = getSprite("hearts");
    const SpriteDefinition& heart_empty = getSprite("heart_empty");

    const RenderView view = worldRenderView();
    const float viewScale = view.scale > 0.0f ? view.scale : 1.0f;
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& healthPool = m_ECS.getComponentPool<CHealth>();
    auto viewHealth = m_ECS.View<CHealth, CTransform>();
    for (auto entityID : viewHealth)
    {
        // if (entityID == m_player) { continue; }
        auto& health = healthPool.getComponent(entityID);
        // if ((int)(m_currentFrame - health.damage_frame) >= health.i_frames)
        // {
        //     continue;
        // }
        auto& transform = transformPool.getComponent(entityID);

        float hearts = health.HP / 2;
        
        Vec2 heartSize = heartsSprite.frameSize() * transform.scale * (static_cast<float>(windowScale) / viewScale);
        const RectF src = {
            (10-ceil(hearts)) * heartSize.y * (int)(hearts != floor(hearts)),
            // 0.0f,
            heartSize.y * (int)(hearts != floor(hearts)),
            heartSize.x * (float)ceil(hearts),
            heartSize.y
        };
        const CSprite& entitySprite = m_ECS.getComponent<CSprite>(entityID);
        const float entityVisualHeight = entitySprite.size().y * transform.scale.y;
        const RectF dst = {
            transform.pos.x - ceil(hearts) * heartSize.x / 2,
            transform.pos.y - entityVisualHeight / 2 - heartSize.y / 2,
            heartSize.x * ceil(hearts),
            heartSize.y
        };
        drawWorldSprite(heartsSprite, src, dst);
    }
}

void Scene_Play::sRenderUI() {
    int windowScale = m_game->getScale();

    auto hearts = float(m_ECS.getComponent<CHealth>(m_player).HP) / 2;
    const SpriteDefinition& heart_full = getSprite("heart_full");
    const SpriteDefinition& heart_half = getSprite("heart_half");
    const SpriteDefinition& heart_empty = getSprite("heart_empty");
    
    for (int i = 1; i <= m_ECS.getComponent<CHealth>(m_player).HP_max / 2; i++)
    {
        if (hearts >= i)
        {
            const Vec2 heartSize = heart_full.frameSize() * windowScale;
            drawSprite(heart_full, RectF{
                (float)(i - 1) * heart_full.frameSize().x * windowScale * windowScale,
                0.0f,
                heartSize.x,
                heartSize.y
            });
        }
        else if (i - hearts == 0.5f)
        {
            const Vec2 heartSize = heart_half.frameSize() * windowScale;
            drawSprite(heart_half, RectF{
                (float)(i - 1) * heart_half.frameSize().x * windowScale * windowScale,
                0.0f,
                heartSize.x,
                heartSize.y
            });
        }
        else
        {
            const Vec2 heartSize = heart_empty.frameSize() * windowScale;
            drawSprite(heart_empty, RectF{
                (float)(i - 1) * heart_empty.frameSize().x * windowScale * windowScale,
                0.0f,
                heartSize.x,
                heartSize.y
            });
        }
    }

    // render player inventory
    const SpriteDefinition& inventorySprite = getSprite("inventory");
    Vec2 inventorySize = inventorySprite.frameSize() * windowScale;
    drawSprite(inventorySprite, RectF{
        width()/2.0f*windowScale - inventorySize.x/2,
        0.0f,
        inventorySize.x,
        inventorySize.y
    });
    auto& items = m_ECS.getComponent<CInventory>(m_player).items;
    auto activeItemIndex = m_ECS.getComponent<CInventory>(m_player).activeItem.index;
    int slotIndex = -1;
    for (Item& item: items){
        slotIndex++;
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
        drawSprite(itemSprite, RectF{
            (width()/2.0f + slotIndex*32.0f)*windowScale - inventorySize.x/2,
            0.0f,
            itemSize.x,
            itemSize.y
        });
    }
}

void Scene_Play::sRender() {    
    sRenderBasic();
    sRenderHealth();
    sRenderUI();
    
    int windowScale = m_game->getScale();

    if (m_drawCollision)
    {
        m_collisionManager.renderQuadtree(m_game->render());
    }

    if (m_drawInteraction)
    {
        m_interactionManager.renderQuadtree(m_game->render());
    }
    
}

void Scene_Play::sAudio()
{
    auto& audioPool = m_ECS.getComponentPool<CAudio>();
    auto audioView = m_ECS.View<CAudio>();
    for (EntityID id : audioView){
        CAudio& audio = audioPool.getComponent(id);
        m_game->playAudio(audio.audioName);
        m_ECS.removeComponent<CAudio>(id);
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
            c["CAnimation"]["layer"]
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
    if (c.contains("CInteractionBox")){
        m_ECS.addComponent<CInteractionBox>(id, c["CInteractionBox"]);
    }
    if (c.contains("CItem")) {
        m_ECS.addComponent<CItem>(id, c["CItem"].at("itemID").get<int>());
    }
    if (c.contains("CCollisionBox")){
        m_ECS.addComponent<CCollisionBox>(id, c["CCollisionBox"]);
    }
    if (c.contains("CState")){
        m_ECS.addComponent<CState>(id);
    }
    if (c.contains("CHealth")){
        m_ECS.addComponent<CHealth>(id, c["CHealth"]);
    }
    if (c.contains("CAttack")){
        m_ECS.addComponent<CAttack>(id, c["CAttack"]);
    }
    if (c.contains("CPossesLevel")){
        m_ECS.addComponent<CPossesLevel>(id, c["CPossesLevel"]);
    }
    if (c.contains("CWeapon")){
        m_ECS.addComponent<CWeapon>(id, c["CWeapon"]);
    }
    if (c.contains("CInput")){
        m_ECS.addComponent<CInput>(id);
    }
    if (c.contains("CAIAgent")) {
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

EntityID Scene_Play::spawnPlayer()
{
    uint8_t layer = 9;
    auto entityID = m_ECS.addEntity();
    m_player = entityID;

    int pos_x = m_playerConfig.x;
    int pos_y = m_playerConfig.y;
    int hp = m_playerConfig.HP;
    
    json j;
    if (!m_newGame){
        std::ifstream file_json("config_files/game_save.json");
        if (!file_json) {
            throw std::runtime_error("Could not load game save file: config_files/game_save.json");
        }

        file_json >> j;
        file_json.close();
        Vec2 pos = j["player"]["position"];
        hp = j["player"]["hp"];        
    }
    
    Vec2 pos = Vec2{16*(float)pos_x, 16*(float)pos_y};
    Vec2 midGrid = gridToMidPixel(pos, entityID);
    
    m_ECS.addComponent<CTransform>(entityID, midGrid);
    m_ECS.addComponent<CVelocity>(entityID);
    m_ECS.addComponent<CPhysicsBody>(
        entityID,
        m_playerConfig.mass,
        m_playerConfig.moveForce,
        m_playerConfig.maxSpeed,
        m_playerConfig.linearDamping
    );
    CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER | FRIENDLY_LAYER;
    m_ECS.addComponent<CCollisionBox>(entityID, Vec2 {8, 8}, PLAYER_LAYER, collisionMask);
    CollisionMask interactionMask = ENEMY_LAYER | FRIENDLY_LAYER | LOOT_LAYER | AREA_LAYER;
    m_ECS.addComponent<CInteractionBox>(entityID, Vec2 {4, 4}, PLAYER_LAYER, interactionMask);
    m_ECS.addComponent<CName>(entityID, "demon");
    addVisual(entityID, "demon-sheet", layer);
    spawnShadow(entityID);
    m_ECS.addComponent<CInput>(entityID);
    m_ECS.addComponent<CState>(entityID, PlayerState::STAND);
    m_ECS.addComponent<CHealth>(entityID, hp, m_playerConfig.HP, 60);
    m_ECS.addComponent<CInventory>(entityID);
    
    if (j.contains("inventory")){
        auto& inventory = m_ECS.getComponent<CInventory>(entityID);
        int i = 0;
        for (int itemID : j["inventory"])
        {
            inventory.items[i] = m_inventoryManager.getItem(itemID);
            inventory.items[i].index = i;
            i++;
        }
        inventory.activeItem = inventory.items[0];
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
    m_ECS.addComponent<CParent>(shadowID, parentID, relativePos);
    addSprite(shadowID, "shadow", parentSprite.layer - 1);
    m_ECS.addComponent<CChild>(parentID, shadowID);
    return shadowID;
}

EntityID Scene_Play::spawnObstacle(const Vec2 pos, bool movable, const int frame){
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0.5f,0.5f});
    CollisionMask collisionMask = ENEMY_LAYER | FRIENDLY_LAYER | PLAYER_LAYER | PROJECTILE_LAYER;
    m_ECS.addComponent<CCollisionBox>(entity, Vec2 {16, 16}, OBSTACLE_LAYER, collisionMask);
    return entity;
}

EntityID Scene_Play::spawnWater(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid);
    m_ECS.addComponent<CWater>(entity, CWater{false});
    m_ECS.addComponent<CCollisionBox>(entity, Vec2{16, 16});
    return entity;
}

EntityID Scene_Play::spawnProjectile(Vec2 startPos, Vec2 vel)
{
    auto id = m_ECS.addEntity();
    int layer = 8;
    addVisual(id, "fireball", layer);
    m_ECS.addComponent<CTransform>(id, startPos, vel.angle());
    m_ECS.addComponent<CVelocity>(id, vel.norm(200.0f));
    m_ECS.addComponent<CDamage>(id, 1);
    m_ECS.getComponent<CDamage>(id).damageType = {"Fire", "Explosive"};
    CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER;
    m_ECS.addComponent<CCollisionBox>(id, Vec2{6, 6}, PROJECTILE_LAYER, collisionMask);
    m_ECS.addComponent<CLifespan>(id, 60);
    m_ECS.addComponent<CAudio>(id, "fireball_shot");
    return id;
}
EntityID Scene_Play::spawnHitbox(Vec2 position, Vec2 direction, CollisionMask layer, CollisionMask mask)
{
    auto id = m_ECS.addEntity();
    int render_layer = 9;
    addSprite(id, "sword", render_layer);
    m_ECS.addComponent<CTransform>(id, position+direction*8, direction.angle());
    m_ECS.addComponent<CDamage>(id, 1);
    m_ECS.addComponent<CInteractionBox>(id, Vec2{24, 24}, layer, mask);
    m_ECS.addComponent<CLifespan>(id, 15);
    return id;
}

std::vector<EntityID> Scene_Play::spawnDualTiles(const Vec2 pos, std::array<int, 5> tileTextures)
{   
    std::vector<EntityID> entityIDs;
    // for (const auto& [tileKey, textureIndex] : tileTextures) {
    for (int i = 0; i < tileTextures.size(); ++i) {
        TileType tileKey = static_cast<TileType>(i);
        int textureIndex = tileTextures[i];
        if (textureIndex == 0) {
            continue;
        }
        uint8_t layer = 3;
        std::string tile_name = "grass";
        
        if (tileKey == TileType::WATER) {
            layer = layer + 1;
            tile_name = "water";
        } else if (tileKey == TileType::DIRT) {
            layer = layer + 1;
            tile_name = "dirt";
        } else if (tileKey == TileType::OBSTACLE) {
            layer = layer + 1;
            tile_name = "mountain";
        }

        EntityID entity = m_ECS.addEntity();
        entityIDs.push_back(entity);
        const std::string spriteName = tile_name + "_dual_sheet";
        CSprite& sprite = addSprite(entity, spriteName, layer);
        Vec2 tilePosition = Vec2{   (float)(textureIndex % 4), 
                                    (float)(int)(textureIndex / 4)};
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
    Vec2  dir     = delta / dist;          // normalized

    auto& transformPool  = m_ECS.getComponentPool<CTransform>();
    auto& collisionPool  = m_ECS.getComponentPool<CCollisionBox>();

    for (EntityID obstacle : m_ECS.View<CCollisionBox, CTransform>()) {
        auto& box = collisionPool.getComponent(obstacle);
        if ((box.layer & OBSTACLE_LAYER) == 0) continue;   // only solid obstacles

        Vec2 pos    = transformPool.getComponent(obstacle).pos;
        Vec2 boxMin = pos - box.halfSize;
        Vec2 boxMax = pos + box.halfSize;

        if (rayIntersectsAABB(origin, dir, dist, boxMin, boxMax)) {
            return false;   // something is in the way
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
