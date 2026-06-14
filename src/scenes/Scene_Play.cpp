#include "scenes/Scene_Play.h"
#include "scenes/Scene_Menu.h"
#include "scenes/Scene_GameOver.h"

#include "assets/Sprite.h"
#include "assets/Assets.h"

#include "core/Game.h"
#include "core/Action.h"

#include "physics/Level_Loader.h"
#include "physics/Camera.h"
#include "physics/RandomArray.h"

#include "ecs/Components.h"

#include "external/json.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <queue>

// TODO: fix line count in file

using json = nlohmann::json;

Scene_Play::Scene_Play(Game* game, std::string levelPath, bool newGame)
    : Scene(game), 
    m_levelPath(levelPath), 
    m_collisionManager(&m_ECS, this), 
    m_interactionManager(&m_ECS, this), 
    m_storyManager(this, "config_files/story.json", "config_files/quests.json"),
    m_levelLoader(this, m_gridSize, levelPath),
    m_newGame(newGame),
    m_inventoryManager("config_files/items")
{
    registerAction(SDLK_W, "UP");
    registerAction(SDLK_UP, "UP");
    registerAction(SDLK_S, "DOWN");
    registerAction(SDLK_DOWN, "DOWN");
    registerAction(SDLK_A, "LEFT");
    registerAction(SDLK_LEFT, "LEFT");
    registerAction(SDLK_D, "RIGHT");
    registerAction(SDLK_RIGHT, "RIGHT");
    
    registerAction(SDLK_I, "INVENTORY");
    registerAction(SDL_BUTTON_LEFT , "ATTACK");
    registerAction(SDL_BUTTON_RIGHT , "WRITE POSITION");
    registerAction(SDL_MOUSEWHEEL_NORMAL , "SCROLL");
    registerAction(SDLK_E, "INTERACT");
    registerAction(SDLK_LSHIFT, "SHIFT");
    registerAction(SDLK_LCTRL, "CTRL");
    registerAction(SDLK_ESCAPE, "ESC");
    registerAction(SDLK_U, "SAVE");
    registerAction(SDLK_R, "RESET");
    registerAction(SDLK_T, "TAKE OVER");

    registerAction(SDLK_X, "CAMERA FOLLOW");
    registerAction(SDLK_Z, "CAMERA PAN");
    registerAction(SDLK_PLUS, "ZOOM IN");
    registerAction(SDLK_MINUS, "ZOOM OUT");
    registerAction(SDLK_Q, "WRITE QUADTREE");
    registerAction(SDLK_P, "PAUSE");
    registerAction(SDLK_O, "FPS COUNTER");
    registerAction(SDLK_K, "KILL_PLAYER");
    registerAction(SDLK_F3, "TOGGLE_COLLISION");
    registerAction(SDLK_F4, "TOGGLE_INTERACTION");
    registerAction(SDLK_F5, "TOGGLE_TEXTURE");
    registerAction(SDLK_1, "Slot1");
    registerAction(SDLK_2, "Slot2");
    registerAction(SDLK_3, "Slot3");
    registerAction(SDLK_7, "TP1");
    registerAction(SDLK_8, "TP2");
    registerAction(SDLK_9, "TP3");

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
            file >> m_playerConfig.x >> m_playerConfig.y >> m_playerConfig.SPEED >> m_playerConfig.MAXSPEED >> m_playerConfig.HP >> m_playerConfig.DAMAGE; // long line to be replaced when saving to json      
        }
        else if (head == "Rooter") {
            file >> m_rooterConfig.SPEED >> m_rooterConfig.ATTACK_SPEED >> m_rooterConfig.HP >> m_rooterConfig.DAMAGE;
        }
        else if (head == "Goblin") {
            file >> m_goblinConfig.SPEED >> m_goblinConfig.ATTACK_SPEED >> m_goblinConfig.HP >> m_goblinConfig.DAMAGE;
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
        sAttack();
        sAI();
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
    auto viewInputs = m_ECS.View<CInput, CTransform, CVelocity>();
    for (auto e : viewInputs){
        auto &transform = transformPool.getComponent(e);
        auto &velocity = velocityPool.getComponent(e);
        auto &inputs = inputPool.getComponent(e);

        velocity.vel = inputs.direction;
        velocity.tempo = inputs.ctrl ? 2.0f : 1.0f;
        if (e != m_player) {
            inputs = CInput(); // reset inputs for NPCs after processing
        }
    }

    // auto viewKnockback = m_ECS.View<CKnockback, CTransform>();
    // auto& knockbackPool = m_ECS.getComponentPool<CKnockback>();
    // for (auto entityKnockback : viewKnockback){    
    //     auto &transform = transformPool.getComponent(entityKnockback);
    //     auto& knockback = knockbackPool.getComponent(entityKnockback);
    //     transform.pos += m_physics.knockback(knockback);
    //     if (knockback.duration <= 0){
    //         m_ECS.queueRemoveComponent<CKnockback>(entityKnockback);
    //     }
    // }       

    auto viewTransform = m_ECS.View<CTransform, CVelocity>();
    for (auto e : viewTransform){
        auto &transform = transformPool.getComponent(e);
        auto &velocity = velocityPool.getComponent(e);
        
        // Update position
        transform.prevPos = transform.pos;
        if (!velocity.vel.isNull()){
            transform.pos += velocity.vel.norm(velocity.tempo*velocity.speed/m_game->framerate());
        }
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
        spawnCoin(transform.pos, 6);
        m_ECS.queueRemoveEntity(entityID);
        m_ECS.addComponent<CAudio>(entityID, "enemy_death");
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
        auto& animation = animationPool.getComponent(e).animation;
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
            animation.setRow((int)state.state);
        }
    }

    auto viewProjectileState = m_ECS.View<CProjectileState, CAnimation>();
    auto& projectileStatePool = m_ECS.getComponentPool<CProjectileState>();
    for ( auto e : viewProjectileState ) {
        auto& animation = animationPool.getComponent(e);
        auto& projectileState = projectileStatePool.getComponent(e);
        if ( projectileState.state == "Create" ) {
            if ( animation.animation.hasEnded() ) {
                projectileState.state = "Ready";
                animation.animation = getAnimation("fireball");
                animation.repeat = true;
                m_camera.startShake(50, 100);
            }
        }
    }

    auto view2 = m_ECS.View<CAnimation>();
    for ( auto e : view2 ){
        auto& animation = animationPool.getComponent(e);
        if (animation.animation.hasEnded() && !animation.repeat) {
            m_ECS.queueRemoveEntity(e);
        } else{
            animation.animation.update(m_currentFrame);
        }
    }
}

void Scene_Play::sRender() {
    // Clear the screen with black
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 255);
    
    // Debug: Print renderer layer sizes
    // const auto& layers = m_rendererManager.getLayers();
    // std::string totalEntitiesInLayers;
    // for (const auto& layer : layers) {
    //     totalEntitiesInLayers += ", " + std::to_string(layer.size());
    // }
    // std::cout << "Renderer Total Entities: " << totalEntitiesInLayers << " | Layers: " << layers.size() << std::endl;
    
    sRenderBasic();
    
    int windowScale = m_game->getScale();

    Animation animation;
    auto hearts = float(m_ECS.getComponent<CHealth>(m_player).HP) / 2;
    const Animation& heart_full = getAnimation("heart_full");
    const Animation& heart_half = getAnimation("heart_half");
    const Animation& heart_empty = getAnimation("heart_empty");
    
    for (int i = 1; i <= m_ECS.getComponent<CHealth>(m_player).HP_max / 2; i++)
    {   
        if (hearts >= i)
        {
            animation = heart_full;
        }
        else if (i - hearts == 0.5f)
        {
            animation = heart_half;
        }
        else
        {
            animation = heart_empty;
        }
        animation.setScale(Vec2{1, 1}*windowScale);
        animation.setDestRect(Vec2{(float)(i - 1) * animation.getSize().x * animation.getScale().x, 0.0f}*windowScale);
        spriteRender(animation);
    }

    Vec2 screenCenter = Vec2{(float)width(), (float)height()}/2;
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& healthPool = m_ECS.getComponentPool<CHealth>();
    auto viewHealth = m_ECS.View<CHealth>();
    for (auto entityID : viewHealth)
    {   
        if (entityID == m_player) { continue; }
        auto& health = healthPool.getComponent(entityID);
        if ((int)(m_currentFrame - health.damage_frame) >= health.i_frames)
        {
            continue;
        }
        auto& transform = transformPool.getComponent(entityID);

        Vec2 adjustedPosition = (transform.pos - m_camera.position) * (windowScale - m_camera.getCameraZoom());
        adjustedPosition += screenCenter*m_camera.getCameraZoom();

        Animation animation;
        auto hearts = float(health.HP) / 2;

        for (int i = 1; i <= health.HP_max / 2; i++)
        {   
            if (hearts >= i)
            {
                animation = heart_full;
            }
            else if (i - hearts == 0.5f)
            {
                animation = heart_half;
            }
            else
            {
                animation = heart_empty;
            }

            animation.setScale(transform.scale * windowScale);
            animation.setDestRect(Vec2{
                adjustedPosition.x + (float)(i - 1 - (float)health.HP_max / 4) * animation.getSize().x * animation.getScale().x, 
                adjustedPosition.y - m_ECS.getComponent<CAnimation>(entityID).animation.getSize().y * m_ECS.getComponent<CAnimation>(entityID).animation.getScale().y / 2
            });
            spriteRender(animation);
        }
    }

    if (m_drawCollision)
    {
        m_collisionManager.renderQuadtree(
            m_game->renderer(), 
            windowScale - m_camera.getCameraZoom(), 
            screenCenter * m_camera.getCameraZoom(), 
            m_camera.position
        );
    }

    if (m_drawInteraction)
    {
        m_interactionManager.renderQuadtree(
            m_game->renderer(), 
            windowScale - m_camera.getCameraZoom(), 
            screenCenter * m_camera.getCameraZoom(), 
            m_camera.position
        );
    }
    
    // render player inventory
    Animation invAni = getAnimation("inventory");
        invAni.setScale(Vec2{1, 1} * windowScale);
        invAni.setDestRect(Vec2{width()/2*windowScale-invAni.getDestSize().x/2, 0});
        spriteRender(invAni);
    auto& items = m_ECS.getComponent<CInventory>(m_player).items;
    auto activeItemIndex = m_ECS.getComponent<CInventory>(m_player).activeItem.index;
    int slotIndex = -1;
    for (Item& item: items){
        slotIndex++;
        if (item.index==activeItemIndex){
            Animation activeItemAni = getAnimation("activeItemInventory");
            activeItemAni.setScale(Vec2{1, 1} * windowScale);
            activeItemAni.setDestRect(Vec2{(width()/2+slotIndex*32)*windowScale-invAni.getDestSize().x/2, 0});
            spriteRender(activeItemAni);
        }
        if (item.id==-1){continue;}
        Animation itemAnim = getAnimation(item.iconPath);
        itemAnim.setScale(Vec2{1, 1} * windowScale);
        itemAnim.setDestRect(Vec2{(width()/2+slotIndex*32)*windowScale-invAni.getDestSize().x/2, 0});
        spriteRender(itemAnim);
    }
}

void Scene_Play::sAudio()
{
    auto& audioPool = m_ECS.getComponentPool<CAudio>();
    auto audioView = m_ECS.View<CAudio>();
    for (EntityID id : audioView){
        CAudio& audio = audioPool.getComponent(id);
        m_game->assets().playAudio(audio.audioName);
        m_ECS.removeComponent<CAudio>(id);
    }
}

EntityID Scene_Play::SpawnFromJSON(std::string name, Vec2 pos)
{
    std::ifstream file("config_files/mobs/"+name+".json");
    if (!file){
        std::cerr << "Could not load entity spawn json file for: " << name << std::endl;
        return -1;
    }

    json j;
    file >> j;
    file.close();
    json c = j[name]["components"];
    
    EntityID id = m_ECS.addEntity();
    
    m_ECS.addComponent<CName>(id, name);
    if (c.contains("CTransform")){

        c["CTransform"]["pos"] = {{"x", pos.x}, {"y", pos.y}};
        m_ECS.addComponent<CTransform>(id, c["CTransform"]);
    }
    if (c.contains("CAnimation")){
        m_ECS.addComponent<CAnimation>(id, 
            getAnimation(c["CAnimation"]["animation"]), 
            c["CAnimation"]["animation"].get<std::string>(),
            c["CAnimation"]["layer"]
        );
        m_rendererManager.addEntityToLayer(id, c["CAnimation"]["layer"]);
    }
    // if (c.contains("CFollow")){
    //     m_ECS.addComponent<CFollow>(id, c["CFollow"]);
    // }
    if (c.contains("CVelocity")){
        m_ECS.addComponent<CVelocity>(id, c["CVelocity"]);
    }
    if (c.contains("CInteractionBox")){
        m_ECS.addComponent<CInteractionBox>(id, c["CInteractionBox"]);
    }
    if (c.contains("CCollisionBox")){
        m_ECS.addComponent<CCollisionBox>(id, c["CCollisionBox"]);
    }
    if (c.contains("CState")){
        m_ECS.addComponent<CState>(id);
    }
    // if (c.contains("CPath")){
    //     m_ECS.addComponent<CPath>(id, c["CPath"]);
    // }
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
    return id;
}

EntityID Scene_Play::Spawn(std::string name, Vec2 pos)
{
    pos = pos*m_gridSize;
    if (name == "copper_staff"){
        return spawnWeapon(pos, name);
    }
    if (name == "emblem"){
        return spawnEmblem(pos, 6);
    }
    if (name == "coin"){
        return spawnCoin(pos, 6);
    }
    if (name == "tree"){
        return spawnDecoration(pos, Vec2 {6, 8}, 6, "tree");
    }
    if (name == "house"){
        EntityID id = spawnDecoration(pos, Vec2 {56, 44}, 6, name);
        m_ECS.addComponent<CEvent>(id, Event{EventType::EnteredArea, name});
        m_ECS.addComponent<CInteractionBox>(id, Vec2{16, 64}, AREA_LAYER, PLAYER_LAYER);
        return id;
    }
    if (name == "campfire"){
        return spawnCampfire(pos, 6);
    }
    if (name == "sword"){
        return spawnSword(pos);
    }
    auto i = SpawnFromJSON(name, pos);
    std::cout << "Spawned entity: " << name << " with ID: " << i << std::endl;
    return i;
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
    m_ECS.addComponent<CVelocity>(entityID, m_playerConfig.SPEED);
    CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER | FRIENDLY_LAYER;
    m_ECS.addComponent<CCollisionBox>(entityID, Vec2 {8, 8}, PLAYER_LAYER, collisionMask);
    CollisionMask interactionMask = ENEMY_LAYER | FRIENDLY_LAYER | LOOT_LAYER | AREA_LAYER;
    m_ECS.addComponent<CInteractionBox>(entityID, Vec2 {4, 4}, PLAYER_LAYER, interactionMask);
    m_ECS.addComponent<CName>(entityID, "demon");
    m_ECS.addComponent<CAnimation>(entityID, getAnimation("demon-sheet"), layer);
    m_rendererManager.addEntityToLayer(entityID, layer);
    spawnShadow(entityID, Vec2{0,0}, 1, layer-1);
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

EntityID Scene_Play::spawnShadow(EntityID parentID, Vec2 relPos, int size, int layer){
    auto shadowID = m_ECS.addEntity();
    m_ECS.addComponent<CTransform>(shadowID);
    m_ECS.getComponent<CTransform>(shadowID).scale *= size;
    m_ECS.addComponent<CParent>(shadowID, parentID, relPos);
    m_ECS.addComponent<CAnimation>(shadowID, getAnimation("shadow"), layer);
    m_rendererManager.addEntityToLayer(shadowID, layer);
    m_ECS.addComponent<CChild>(parentID, shadowID);
    return shadowID;
    // return 0;
}

EntityID Scene_Play::spawnWeapon(Vec2 pos, std::string weaponName){
    auto entityID = m_ECS.addEntity();
    int layer = 7;
    Vec2 midGrid = gridToMidPixel(pos, entityID);
    m_ECS.addComponent<CTransform>(entityID, midGrid);
    m_ECS.addComponent<CVelocity>(entityID);
    m_ECS.addComponent<CItem>(entityID, 1);
    
    CollisionMask interactionMask = PLAYER_LAYER;
    m_ECS.addComponent<CInteractionBox>(entityID, Vec2 {6, 6}, LOOT_LAYER, interactionMask);
    m_ECS.addComponent<CName>(entityID, weaponName);
    m_ECS.addComponent<CAnimation>(entityID, getAnimation("staff"), layer);
    m_rendererManager.addEntityToLayer(entityID, layer);
    m_ECS.addComponent<CWeapon>(entityID);
    spawnShadow(entityID, Vec2{0,0}, 1, layer-1);
    return entityID;
}

EntityID Scene_Play::spawnSword(Vec2 pos, std::string weaponName){
    auto id = m_ECS.addEntity();
    int layer = 7;
    Vec2 midGrid = gridToMidPixel(pos, id);
    m_ECS.addComponent<CTransform>(id, midGrid);
    m_ECS.addComponent<CVelocity>(id, m_playerConfig.SPEED);
    m_ECS.addComponent<CItem>(id, 2);
    
    CollisionMask interactionMask = PLAYER_LAYER;
    m_ECS.addComponent<CInteractionBox>(id, Vec2 {6, 6}, LOOT_LAYER, interactionMask);
    
    m_ECS.addComponent<CName>(id, "sword");
    m_ECS.addComponent<CAnimation>(id, getAnimation("sword"), layer);
    m_rendererManager.addEntityToLayer(id, layer);
    // m_ECS.addComponent<CDamage>(id, 1, 180, std::unordered_set<std::string> {"Fire", "Explosive"});
    m_ECS.addComponent<CWeapon>(id);
    spawnShadow(id, Vec2{0,0}, 1, layer-1);
    return id;
}

EntityID Scene_Play::spawnDecoration(Vec2 pos, Vec2 collisionBox, const size_t layer, std::string animationName){
    EntityID id = m_ECS.addEntity();
    
    Vec2 midGrid = gridToMidPixel(pos, id);
    m_ECS.addComponent<CTransform>(id, midGrid);
    m_ECS.addComponent<CAnimation>(id, getAnimation(animationName), layer);
    CollisionMask collisionMask = ENEMY_LAYER | FRIENDLY_LAYER | PLAYER_LAYER;
    m_ECS.addComponent<CCollisionBox>(id, collisionBox, OBSTACLE_LAYER, collisionMask);
    m_ECS.addComponent<CName>(id, animationName);
    m_rendererManager.addEntityToLayer(id, layer);
    m_ECS.addComponent<CImmovable>(id);
    spawnShadow(id, Vec2{0,-4}, 3, layer-1);
    return id;
}

EntityID Scene_Play::spawnObstacle(const Vec2 pos, bool movable, const int frame){
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0.5f,0.5f});
    CollisionMask collisionMask = ENEMY_LAYER | FRIENDLY_LAYER | PLAYER_LAYER | PROJECTILE_LAYER;
    m_ECS.addComponent<CCollisionBox>(entity, Vec2 {16, 16}, OBSTACLE_LAYER, collisionMask);

    m_ECS.addComponent<CImmovable>(entity); // remove when new collision system is implemented
    return entity;
}

EntityID Scene_Play::spawnCampfire(const Vec2 pos, int layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity,getAnimation("campfire"), layer);
    m_rendererManager.addEntityToLayer(entity, layer);
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid);
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

EntityID Scene_Play::spawnEmblem(Vec2 pos, const size_t layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CName>(entity, "emblem");
    m_ECS.addComponent<CAnimation>(entity, getAnimation("emblem"), layer);
    m_rendererManager.addEntityToLayer(entity, layer);
    m_ECS.addComponent<CTransform>(entity, pos);
    m_ECS.addComponent<CItem>(entity, 0); // TODO: fix correct item here
    CollisionMask interactionMask = PLAYER_LAYER;
    m_ECS.addComponent<CInteractionBox>(entity, Vec2 {8, 8}, LOOT_LAYER, interactionMask);
    spawnShadow(entity, Vec2{0,0}, 1, layer-1);
    return entity;
}

EntityID Scene_Play::spawnCoin(Vec2 pos, const size_t layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, getAnimation("coin"), layer);
    m_rendererManager.addEntityToLayer(entity, layer);
    m_ECS.addComponent<CTransform>(entity, pos);
    m_ECS.addComponent<CItem>(entity, 0);
    CollisionMask interactionMask = PLAYER_LAYER;
    m_ECS.addComponent<CInteractionBox>(entity, Vec2 {8, 8}, LOOT_LAYER, interactionMask);
    spawnShadow(entity, Vec2{0,0}, 1, layer-1);
    return entity;
}

EntityID Scene_Play::spawnProjectile(Vec2 startPos, Vec2 vel)
{
    auto id = m_ECS.addEntity();
    int layer = 8;
    m_ECS.addComponent<CAnimation>(id, m_game->assets().getAnimation("fireball"), true, layer);
    m_rendererManager.addEntityToLayer(id, layer);
    m_ECS.addComponent<CTransform>(id, startPos, vel.angle());
    m_ECS.addComponent<CVelocity>(id, vel, 200.0f);
    m_ECS.addComponent<CDamage>(id, 1);
    m_ECS.getComponent<CDamage>(id).damageType = {"Fire", "Explosive"};
    CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER;
    m_ECS.addComponent<CCollisionBox>(id, Vec2{6, 6}, PROJECTILE_LAYER, collisionMask);
    m_ECS.addComponent<CLifespan>(id, 60);
    // spawnShadow(id, Vec2{0,0}, 1, layer-1);
    return id;
}
EntityID Scene_Play::spawnHitbox(Vec2 position, Vec2 direction, CollisionMask layer, CollisionMask mask)
{
    auto id = m_ECS.addEntity();
    int render_layer = 9;
    m_ECS.addComponent<CAnimation>(id, m_game->assets().getAnimation("sword"), true, render_layer);
    m_rendererManager.addEntityToLayer(id, render_layer);
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
        m_ECS.addComponent<CAnimation>(entity, getAnimation(tile_name + "_dual_sheet"), layer);
        Vec2 tilePosition = Vec2{   (float)(textureIndex % 4), 
                                    (float)(int)(textureIndex / 4)};
        m_ECS.getComponent<CAnimation>(entity).animation.setTile(tilePosition); 
        m_rendererManager.addEntityToLayer(entity, layer);
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

    for (EntityID obstacle : m_ECS.View<CCollisionBox, CTransform, CImmovable>()) {
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
