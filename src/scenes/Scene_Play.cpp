#include "scenes/Scene_Play.h"
#include "scenes/Scene_Menu.h"
#include "scenes/Scene_GameOver.h"
#include "scenes/Scene_Inventory.h"
#include "assets/Sprite.h"
#include "assets/Assets.h"
#include "core/Game.h"
#include "ecs/Components.h"
#include "core/Action.h"
#include "physics/Level_Loader.h"
#include "physics/Camera.h"
#include "ecs/ScriptableEntity.h"
#include "scripts/player.cpp"
#include "scripts/npc.cpp"
#include "scripts/house.cpp"
#include "scripts/rooter.cpp"
#include "scripts/weapon.cpp"
#include "scripts/projectile.cpp"
#include "scripts/coin.cpp"
#include "physics/RandomArray.h"
#include "external/json.hpp"

#include <SDL_image.h>
#include <SDL_mixer.h>

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <queue>

// TODO: fix line count in file

using json = nlohmann::json;

// inline std::unordered_map<std::string, ScriptInitFn> ScriptRegistry = {
//     {"NPCController", [](Scene_Play* scene, CScript& sc, EntityID id){ 
//         scene->InitiateScript<NPCController>(sc, id); 
//     }},
//     {"PlayerController", [](Scene_Play* scene, CScript& sc, EntityID id){ 
//         scene->InitiateScript<PlayerController>(sc, id); 
//     }},
//     // etc.
// };

Scene_Play::Scene_Play(Game* game, std::string levelPath, bool newGame)
    : Scene(game), 
    m_levelPath(levelPath), 
    m_collisionManager(&m_ECS, this), 
    m_interactionManager(&m_ECS, this), 
    m_storyManager(this, "config_files/story.json"),
    m_levelLoader(this, m_gridSize, levelPath),
    m_newGame(newGame)
{
    registerAction(SDLK_w, "UP");
    registerAction(SDLK_UP, "UP");
    registerAction(SDLK_s, "DOWN");
    registerAction(SDLK_DOWN, "DOWN");
    registerAction(SDLK_a, "LEFT");
    registerAction(SDLK_LEFT, "LEFT");
    registerAction(SDLK_d, "RIGHT");
    registerAction(SDLK_RIGHT, "RIGHT");
    
    registerAction(SDLK_e, "INVENTORY");
    registerAction(SDL_BUTTON_LEFT , "ATTACK");
    registerAction(SDL_MOUSEWHEEL , "ATTACK");
    registerAction(SDL_MOUSEWHEEL_NORMAL , "SCROLL");
    registerAction(SDLK_f, "INTERACT");
    registerAction(SDLK_LSHIFT, "SHIFT");
    registerAction(SDLK_LCTRL, "CTRL");
    registerAction(SDLK_ESCAPE, "ESC");
    registerAction(SDLK_u, "SAVE");
    registerAction(SDLK_r, "RESET");

    registerAction(SDLK_x, "CAMERA FOLLOW");
    registerAction(SDLK_z, "CAMERA PAN");
    registerAction(SDLK_PLUS, "ZOOM IN");
    registerAction(SDLK_MINUS, "ZOOM OUT");
    registerAction(SDLK_q, "WRITE QUADTREE");
    registerAction(SDLK_p, "PAUSE");
    registerAction(SDLK_k, "KILL_PLAYER");
    registerAction(SDLK_F3, "TOGGLE_COLLISION");
    registerAction(SDLK_F4, "TOGGLE_INTERACTION");
    registerAction(SDLK_F5, "TOGGLE_TEXTURE");
    registerAction(SDLK_1, "TP1");
    registerAction(SDLK_2, "TP2");
    registerAction(SDLK_3, "TP3");

    loadConfig("config_files/config.txt");
    spawnPlayer();
    // mobs have to spawn after player, so they can target the player
    loadMobsNItems("config_files/mobs.json");
    SpawnFromJSON("wizard", Vec2{348, 65}*m_gridSize);

    m_camera.calibrate(Vec2{width(), height()}, m_levelLoader.getLevelSize(), m_gridSize);
    m_inventory_scene = std::make_shared<Scene_Inventory>(m_game);

    // StoryManager listens to events
    m_eventBus.subscribe([this](const Event& e) {
        m_storyManager.onEvent(e);
    });
}

void Scene_Play::loadMobsNItems(const std::string& path){
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Could not load assets file!\n";
        exit(-1);
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
        std::cerr << "Could not load config.txt file!\n";
        exit(-1);
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
            std::cerr << "head to " << head << "\n";
            std::cerr << "The config file format is incorrect!\n";
            exit(-1);
        }
    }
}

// Function to save the game state to a file
void Scene_Play::saveGame(const std::string& filename) 
{
    std::ofstream saveFile(filename);
    if ( !saveFile.is_open() ) 
    {
        std::cerr << "Unable to open file for saving!" << std::endl;
        return;
    }
    Vec2 playerPos = m_ECS.getComponent<CTransform>(m_player).pos;
    saveFile << "Player_pos " << (int)(playerPos.x/m_gridSize.x) << " " << (int)(playerPos.y/m_gridSize.y) << std::endl; // TODO: long line to be replaced when saving to json
    saveFile << "Player_hp " << m_ECS.getComponent<CHealth>(m_player).HP << std::endl;
    saveFile.close();
}

void Scene_Play::sDoAction(const Action& action) {
    if ( action.type() == "START")
    {
        if ( action.name() == "TOGGLE_TEXTURE") {
            m_drawTextures = !m_drawTextures; 
        } else if ( action.name() == "TOGGLE_COLLISION") { 
            m_drawCollision = !m_drawCollision; 
        } else if ( action.name() == "TOGGLE_INTERACTION") { 
            m_drawInteraction = !m_drawInteraction; 
        } else if ( action.name() == "PAUSE") { 
            setPaused( !m_pause || m_inventoryOpen );
        } else if ( action.name() == "INVENTORY") { 
            m_inventoryOpen = !m_inventoryOpen;
            m_inventory_scene->toggleInventory();
            setPaused( !m_pause || m_inventoryOpen );
        } else if ( action.name() == "SCROLL"){
            if ( m_inventoryOpen )
            {
                m_inventory_scene->Scroll(m_mouseState.scroll);
            }
        } else if ( action.name() == "ZOOM IN"){
            m_camera.stepCameraZoom(-1, m_game->getScale());
        } else if ( action.name() == "ZOOM OUT"){
            m_camera.stepCameraZoom(1, m_game->getScale());
        } else if ( action.name() == "CAMERA FOLLOW"){
            m_camera.toggleCameraFollow();
        } else if ( action.name() == "CAMERA PAN"){
            m_pause = m_camera.startPan(2048, 1000, Vec2{0,0}, m_pause);
        } else if ( action.name() == "SAVE"){
            saveGame("config_files/game_save.txt");
        } else if ( action.name() == "TP1") { 
            m_ECS.getComponent<CTransform>(m_player).pos = Vec2{460*16, 460*16};
        } else if ( action.name() == "TP2") { 
            m_ECS.getComponent<CTransform>(m_player).pos = Vec2{292*16, 236*16};
        }else if ( action.name() == "TP3") {
            m_ECS.getComponent<CTransform>(m_player).pos = Vec2{801*16, 181*16};
        } else if ( action.name() == "RESET") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, m_levelPath, true), true);
        } else if ( action.name() == "KILL_PLAYER") { 
            m_ECS.getComponent<CHealth>(m_player).HP = 0;
        }
        if ( action.name() == "UP") { m_ECS.getComponent<CInputs>(m_player).up = true; }
        if ( action.name() == "DOWN") { m_ECS.getComponent<CInputs>(m_player).down = true;}
        if ( action.name() == "LEFT") { m_ECS.getComponent<CInputs>(m_player).left = true; }
        if ( action.name() == "RIGHT") { m_ECS.getComponent<CInputs>(m_player).right = true; }
        if ( action.name() == "SHIFT") { m_ECS.getComponent<CInputs>(m_player).shift = true; }
        if ( action.name() == "CTRL") { m_ECS.getComponent<CInputs>(m_player).ctrl = true; }
        if ( action.name() == "INTERACT" ) { m_ECS.getComponent<CInputs>(m_player).interact = true; }

        if ( action.name() == "ATTACK"){
            m_ECS.getComponent<CScript>(m_player).Instance->OnAttackFunction();
        }
    }
    else if ( action.type() == "END")
    {
        if ( action.name() == "DOWN") { m_ECS.getComponent<CInputs>(m_player).down = false; }
        if ( action.name() == "UP") { m_ECS.getComponent<CInputs>(m_player).up = false; }
        if ( action.name() == "LEFT") { m_ECS.getComponent<CInputs>(m_player).left = false; }
        if ( action.name() == "RIGHT") { m_ECS.getComponent<CInputs>(m_player).right = false; }
        if ( action.name() == "SHIFT") { m_ECS.getComponent<CInputs>(m_player).shift = false; }
        if ( action.name() == "CTRL") { m_ECS.getComponent<CInputs>(m_player).ctrl = false; }
        if ( action.name() == "INTERACT") { m_ECS.getComponent<CInputs>(m_player).interact = false; }
        if ( action.name() == "ESC") {
            m_game->changeScene("SETTINGS", std::make_shared<Scene_Pause>(m_game), false);
            saveGame("config_files/game_save.txt");
            m_pause = true;
        }
        if ( action.name() == "WRITE QUADTREE")
        {
            m_physics.m_quadRoot->printTree("", "");
        }
    }
}

void Scene_Play::update() 
{
    m_pause = m_camera.update(m_ECS.getComponent<CTransform>(m_player).pos, m_pause);
    if (!m_pause) 
    {
        sLoader();
        sScripting();
        sMovement();
        sStatus();
        sCollision();
        sInteraction();
        sAnimation();
        sAudio();
        m_currentFrame++;
    }
    sRender();
    m_inventory_scene->update();
    m_ECS.update();
    // m_storyManager.update();
    m_rendererManager.update();
    if (m_restart) {
        m_game->changeScene("GAMEOVER", std::make_shared<Scene_GameOver>(m_game), true);
        return;
    }
    if (m_storyManager.IsStoryFinished()){
        onFinish();
    }
}

void Scene_Play::sLoader()
{
    Vec2 playerPosition = m_ECS.getComponent<CTransform>(m_player).pos;
    m_levelLoader.update(playerPosition);
}

void Scene_Play::sScripting() 
{
    auto view = m_ECS.View<CScript>();
    auto& scriptPool = m_ECS.getComponentPool<CScript>();
    for ( auto e : view ) 
    {
        auto& sc = scriptPool.getComponent(e);
        sc.Instance->OnUpdateFunction();
        // memory leak, destroy
    }
}

void Scene_Play::sMovement() {
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& velocityPool = m_ECS.getComponentPool<CVelocity>();

    auto& pathfindPool = m_ECS.getComponentPool<CPathfind>();
    auto viewPathfind = m_ECS.View<CPathfind, CTransform, CVelocity>();
    for (auto e : viewPathfind)
    {
        auto& transform = transformPool.getComponent(e);
        auto& velocity = velocityPool.getComponent(e);
        auto& pathfind = pathfindPool.getComponent(e);
        if ((pathfind.target - transform.pos).length() < 16*2) {
            velocity.vel = pathfind.target - transform.pos;
        } else {
            velocity.vel = Vec2 {0,0};
        }
        pathfind.target = transformPool.getComponent(m_player).pos;
    }

    auto& inputPool = m_ECS.getComponentPool<CInputs>();
    auto viewInputs = m_ECS.View<CInputs, CTransform, CVelocity>();
    for (auto e : viewInputs){
        auto &transform = transformPool.getComponent(e);
        auto &velocity = velocityPool.getComponent(e);
        auto &inputs = inputPool.getComponent(e);

        velocity.vel = { 0,0 };
        if (inputs.up){
            velocity.vel.y--;
        } if (inputs.down){
            velocity.vel.y++;
        } if (inputs.left){
            velocity.vel.x--;
        } if (inputs.right){
            velocity.vel.x++;
        } if (inputs.shift){
            velocity.tempo = 0.5f;
        } else if (inputs.ctrl){
            velocity.tempo = 2.0f;
        } else{
            velocity.tempo = 1.0f;
        }

        if (m_ECS.hasComponent<CSwimming>(e)){
            velocity.tempo *= m_ECS.getComponent<CSwimming>(e).swimSpeedMultiplier;
        }
    }

    auto viewKnockback = m_ECS.View<CKnockback, CTransform>();
    auto& knockbackPool = m_ECS.getComponentPool<CKnockback>();
    for (auto entityKnockback : viewKnockback){    
        auto &transform = transformPool.getComponent(entityKnockback);
        auto& knockback = knockbackPool.getComponent(entityKnockback);
        transform.pos += m_physics.knockback(knockback);
        if (knockback.duration <= 0){
            m_ECS.queueRemoveComponent<CKnockback>(entityKnockback);
        }
    }       

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
        } else {
            spawnCoin(transform.pos, 6);
            m_ECS.queueRemoveEntity(entityID);
        }
        Mix_PlayChannel(-1, m_game->assets().getAudio("enemy_death"), 0);
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
            changePlayerStateTo(e, PlayerState::STAND);
        } else if( velocity.vel.mainDir().x > 0 ) {
            changePlayerStateTo(e, PlayerState::RUN_RIGHT);
        } else if(velocity.vel.mainDir().x < 0) {
            changePlayerStateTo(e, PlayerState::RUN_LEFT);
        } else if(velocity.vel.mainDir().y > 0) {
            changePlayerStateTo(e, PlayerState::RUN_DOWN);
        } else if(velocity.vel.mainDir().y < 0) {
            changePlayerStateTo(e, PlayerState::RUN_UP);
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

    auto view3 = m_ECS.View<CSwimming, CAnimation>();
    for ( auto e : view3 ){
        auto& animation = animationPool.getComponent(e);
        SDL_Rect* rect = animation.animation.getSrcRect();
        animation.animation.setSrcRect(rect->x, rect->y-4, rect->w, rect->h);
    }
}

void Scene_Play::sRender() {
    // Clear the screen with black
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 255);
    SDL_RenderClear(m_game->renderer());
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
}

void Scene_Play::sAudio()
{
    if( Mix_PlayingMusic() == 0 )
    {
        Mix_PlayMusic(m_game->assets().getMusic("AbbeGameTrack1ogg"), -1);
    }
}

EntityID Scene_Play::SpawnFromJSON(std::string name, Vec2 pos)
{
    std::ifstream file("config_files/mobs/"+name+".json");
    assert(file && "Could not load file!");
    json j;
    file >> j;
    file.close();
    json components = j[name]["components"];
    
    EntityID id = m_ECS.addEntity();
    
    m_ECS.addComponent<CName>(id, name);
    if (components.contains("CTransform")){
        m_ECS.addComponent<CTransform>(id, pos);
    }
    if (components.contains("CVelocity")){
        m_ECS.addComponent<CVelocity>(id, components["CVelocity"]);
    }
    if (components.contains("CInteractionBox")){
        m_ECS.addComponent<CInteractionBox>(id, components["CInteractionBox"]);
    }
    if (components.contains("CCollisionBox")){
        m_ECS.addComponent<CCollisionBox>(id, components["CCollisionBox"]);
    }
    if (components.contains("CAnimation")){
        m_ECS.addComponent<CAnimation>(id, 
            getAnimation(components["CAnimation"]["animation"]), 
            components["CAnimation"]["animation"].get<std::string>()
        );
        m_rendererManager.addEntityToLayer(id, components["CAnimation"]["layer"]);
    }
    if (components.contains("CScript")){
        auto& sc = m_ECS.addComponent<CScript>(id);
        std::string controllerType = components["CScript"].get<std::string>();
        if        (controllerType == "NPCController"){
            InitiateScript<NPCController>(sc, id);
        } else if (controllerType == "WeaponController"){
            InitiateScript<WeaponController>(sc, id);
        } else if (controllerType == "CoinController"){
            InitiateScript<CoinController>(sc, id);
        } else if (controllerType == "PlayerController"){
            InitiateScript<PlayerController>(sc, id);
        } else if (controllerType == "RooterController"){
            InitiateScript<RooterController>(sc, id);
        } else if (controllerType == "ProjectileController"){
            InitiateScript<ProjectileController>(sc, id);
        }
    }
    if (components.contains("CState")){
        m_ECS.addComponent<CState>(id);
    }
    if (components.contains("CPathfind")){
        Vec2 playerPos = m_ECS.getComponent<CTransform>(m_player).pos; 
        m_ECS.addComponent<CPathfind>(id, playerPos);
    }
    if (components.contains("CHealth")){
        m_ECS.addComponent<CHealth>(id, components["CHealth"]);
    }
    if (components.contains("CAttack")){
        m_ECS.addComponent<CAttack>(id, components["CAttack"]);
    }
    return id;
}

EntityID Scene_Play::Spawn(std::string name, Vec2 pos)
{
    pos = pos*m_gridSize;
    if (name == "copper_staff") {
        return spawnWeapon(pos, name);
    }
    else if (name == "rooter") {
        return SpawnFromJSON(name, pos);
    }
    else if (name == "goblin") {
        return SpawnFromJSON(name, pos);
    }
    else if (name == "coin") {
        return spawnCoin(pos, 6);
    }
    else if (name == "tree") {
        return spawnDecoration(pos, Vec2 {6, 8}, 6, "tree");
    }
    else if (name == "house") {
        EntityID id = spawnDecoration(pos, Vec2 {56, 44}, 6, "house");
        m_ECS.addComponent<CInteractionBox>(id, Vec2{64, 64}, LOOT_LAYER, PLAYER_LAYER);
        return id;
    }
    else if (name == "campfire") {
        return spawnCampfire(pos, 6);
    }
    else if (name == "sword") {
        return spawnSword(pos);
    }
    return 0; // Return 0 if the entity type is not recognized
}

EntityID Scene_Play::SpawnDialog(
    std::string dialog, 
    int size, 
    std::string font, 
    EntityID parentID
)
{
    auto id = m_ECS.addEntity();
    m_ECS.addComponent<CTransform>(id);
    m_ECS.addComponent<CChild>(parentID, id);
    Vec2 relativePosition = {0, -2*m_gridSize.y};
    m_ECS.addComponent<CParent>(id, parentID, relativePosition);
    CAnimation& animation =  m_ECS.addComponent<CAnimation>(id, getAnimation("button_unpressed"));
    Vec2 animationSize = animation.animation.getSize();
    CText& text = m_ECS.addComponent<CText>(id, dialog, animationSize.y*0.9f, font);
    animation.animation.setScale(Vec2{2*animationSize.x, animationSize.y});
    m_rendererManager.addEntityToLayer(id, 8);
    m_ECS.addComponent<CLifespan>(id, 60);
    return id;
}

EntityID Scene_Play::spawnPlayer()
{
    uint8_t layer = 10;
    auto entityID = m_ECS.addEntity();
    m_player = entityID;

    int pos_x = m_playerConfig.x;
    int pos_y = m_playerConfig.y;
    int hp = m_playerConfig.HP;
    
    if (!m_newGame){
        std::ifstream file("config_files/game_save.txt");
        if (!file) {
            std::cerr << "Could not load game_save.txt file!\n";
            exit(-1);
        }

        std::string head;
        while (file >> head) {
            if (head == "Player_pos") {
                file >> pos_x >> pos_y;
            } else if (head == "Player_hp") {
                file >> hp;
            } else {
                std::cerr << "The game save file format is incorrect!\n";
                exit(-1);
            }
        }
    }
    
    Vec2 pos = Vec2{16*(float)pos_x, 16*(float)pos_y};
    Vec2 midGrid = gridToMidPixel(pos, entityID);

    m_ECS.addComponent<CTransform>(entityID, midGrid);
    m_ECS.addComponent<CVelocity>(entityID, m_playerConfig.SPEED);
    CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER | FRIENDLY_LAYER;
    m_ECS.addComponent<CCollisionBox>(entityID, Vec2 {8, 8}, PLAYER_LAYER, collisionMask);
    CollisionMask interactionMask = ENEMY_LAYER | FRIENDLY_LAYER | LOOT_LAYER;
    m_ECS.addComponent<CInteractionBox>(entityID, Vec2 {16, 16}, PLAYER_LAYER, interactionMask);
    m_ECS.addComponent<CName>(entityID, "demon");
    m_ECS.addComponent<CAnimation>(entityID, getAnimation("demon-sheet"));
    m_rendererManager.addEntityToLayer(entityID, layer);
    spawnShadow(entityID, Vec2{0,0}, 1, layer-1);
    m_ECS.addComponent<CInputs>(entityID);
    m_ECS.addComponent<CState>(entityID, PlayerState::STAND);
    m_ECS.addComponent<CHealth>(entityID, hp, m_playerConfig.HP, 60);
    
    auto& sc= m_ECS.addComponent<CScript>(entityID);
    InitiateScript<PlayerController>(sc, entityID);
    return entityID;
}

EntityID Scene_Play::spawnDwarf(Vec2 pos)
{
    uint8_t layer = 10;
    auto entityID = m_ECS.addEntity();

    Vec2 midGrid = gridToMidPixel(pos*16, entityID);
    m_ECS.addComponent<CTransform>(entityID, midGrid);
    m_ECS.addComponent<CVelocity>(entityID, m_playerConfig.SPEED);
    
    CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER | FRIENDLY_LAYER | PLAYER_LAYER;
    m_ECS.addComponent<CCollisionBox>(entityID, Vec2{8, 8}, FRIENDLY_LAYER, collisionMask);

    CollisionMask interactionMask = PLAYER_LAYER;
    m_ECS.addComponent<CInteractionBox>(entityID, Vec2{48, 32}, FRIENDLY_LAYER, interactionMask);

    m_ECS.addComponent<CName>(entityID, "NPC2");
    m_ECS.addComponent<CAnimation>(entityID, getAnimation("dwarf-sheet"));
    m_rendererManager.addEntityToLayer(entityID, layer);
    spawnShadow(entityID, Vec2{0,0}, 1, layer-1);
    m_ECS.addComponent<CState>(entityID, PlayerState::STAND);
    
    auto& sc= m_ECS.addComponent<CScript>(entityID);
    InitiateScript<NPCController>(sc, entityID);
    return entityID;
}

EntityID Scene_Play::spawnNPC(Vec2 pos)
{
    uint8_t layer = 10;
    auto entityID = m_ECS.addEntity();

    Vec2 midGrid = gridToMidPixel(pos*16, entityID);
    m_ECS.addComponent<CTransform>(entityID, midGrid);
    m_ECS.addComponent<CVelocity>(entityID, m_playerConfig.SPEED);
    
    CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER | FRIENDLY_LAYER | PLAYER_LAYER;
    m_ECS.addComponent<CCollisionBox>(entityID, Vec2 {8, 8}, FRIENDLY_LAYER, collisionMask);

    CollisionMask interactionMask = PLAYER_LAYER;
    m_ECS.addComponent<CInteractionBox>(entityID, Vec2 {48, 32}, FRIENDLY_LAYER, interactionMask);

    m_ECS.addComponent<CName>(entityID, "wizard");
    m_ECS.addComponent<CAnimation>(entityID, getAnimation("wiz-sheet"));
    m_rendererManager.addEntityToLayer(entityID, layer);
    spawnShadow(entityID, Vec2{0,0}, 1, layer-1);
    m_ECS.addComponent<CState>(entityID, PlayerState::STAND);
    
    auto& sc= m_ECS.addComponent<CScript>(entityID);
    InitiateScript<NPCController>(sc, entityID);
    return entityID;
}

EntityID Scene_Play::spawnShadow(EntityID parentID, Vec2 relPos, int size, int layer){
    auto shadowID = m_ECS.addEntity();
    // m_ECS.addComponent<CTransform>(shadowID);
    // m_ECS.getComponent<CTransform>(shadowID).scale *= size;
    // m_ECS.addComponent<CParent>(shadowID, parentID, relPos);
    // m_ECS.addComponent<CAnimation>(shadowID, getAnimation("shadow"));
    // m_rendererManager.addEntityToLayer(shadowID, layer);
    // m_ECS.addComponent<CChild>(parentID, shadowID);
    return shadowID;
}

EntityID Scene_Play::spawnWeapon(Vec2 pos, std::string weaponName){
    auto entityID = m_ECS.addEntity();
    int layer = 7;
    Vec2 midGrid = gridToMidPixel(pos, entityID);
    m_ECS.addComponent<CTransform>(entityID, midGrid);
    m_ECS.addComponent<CVelocity>(entityID);
    
    CollisionMask interactionMask = PLAYER_LAYER;
    m_ECS.addComponent<CInteractionBox>(entityID, Vec2 {6, 6}, LOOT_LAYER, interactionMask);

    m_ECS.addComponent<CName>(entityID, weaponName);
    m_ECS.addComponent<CAnimation>(entityID, getAnimation("staff"));
    m_rendererManager.addEntityToLayer(entityID, layer);
    m_ECS.addComponent<CWeapon>(entityID);
    spawnShadow(entityID, Vec2{0,0}, 1, layer-1);
    auto& sc = m_ECS.addComponent<CScript>(entityID);
    InitiateScript<WeaponController>(sc, entityID);
    return entityID;
}

EntityID Scene_Play::spawnSword(Vec2 pos, std::string weaponName){
    auto entity = m_ECS.addEntity();
    int layer = 7;
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid);
    m_ECS.addComponent<CVelocity>(entity, m_playerConfig.SPEED);
    
    CollisionMask interactionMask = PLAYER_LAYER;
    m_ECS.addComponent<CInteractionBox>(entity, Vec2 {6, 6}, LOOT_LAYER, interactionMask);
    
    m_ECS.addComponent<CName>(entity, "sword");
    m_ECS.addComponent<CAnimation>(entity, getAnimation("sword"));
    m_rendererManager.addEntityToLayer(entity, 5);
    // m_ECS.addComponent<CDamage>(entity, 1, 180, std::unordered_set<std::string> {"Fire", "Explosive"});
    m_ECS.addComponent<CWeapon>(entity);
    spawnShadow(entity, Vec2{0,0}, 1, layer-1);
    auto& sc = m_ECS.addComponent<CScript>(entity);
    InitiateScript<WeaponController>(sc, entity);
    return entity;
}

EntityID Scene_Play::spawnDecoration(Vec2 pos, Vec2 collisionBox, const size_t layer, std::string animation){
    EntityID id = m_ECS.addEntity();

    Vec2 midGrid = gridToMidPixel(pos, id);
    m_ECS.addComponent<CTransform>(id, midGrid);
    m_ECS.addComponent<CAnimation>(id, getAnimation(animation));
    CollisionMask collisionMask = ENEMY_LAYER | FRIENDLY_LAYER | PLAYER_LAYER;
    m_ECS.addComponent<CCollisionBox>(id, collisionBox, OBSTACLE_LAYER, collisionMask);
    if (animation == "house"){
        m_ECS.addComponent<CInteractionBox>(id, Vec2{64, 64}, OBSTACLE_LAYER, PLAYER_LAYER);
        CScript& sc = m_ECS.addComponent<CScript>(id);
        InitiateScript<HouseController>(sc, id);
    }
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

EntityID Scene_Play::spawnGrass(const Vec2 pos, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid);
    return entity;
}

EntityID Scene_Play::spawnDirt(const Vec2 pos, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid);
    return entity;
}

EntityID Scene_Play::spawnCampfire(const Vec2 pos, int layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity,getAnimation("campfire"));
    m_rendererManager.addEntityToLayer(entity, layer);
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid);
    return entity;
}

EntityID Scene_Play::spawnLava(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid);
    m_ECS.addComponent<CCollisionBox>(entity, Vec2{16, 16});
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

EntityID Scene_Play::spawnCoin(Vec2 pos, const size_t layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, getAnimation("coin"));
    m_rendererManager.addEntityToLayer(entity, layer);
    // Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, pos);

    CollisionMask interactionMask = PLAYER_LAYER;
    m_ECS.addComponent<CInteractionBox>(entity, Vec2 {8 ,8}, LOOT_LAYER, interactionMask);

    spawnShadow(entity, Vec2{0,0}, 1, layer-1);

    auto& sc= m_ECS.addComponent<CScript>(entity);
    InitiateScript<CoinController>(sc, entity);
    
    return entity;
}

EntityID Scene_Play::spawnSmallEnemy(Vec2 pos, const size_t layer, std::string type)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CName>(entity, type);
    m_ECS.addComponent<CAnimation>(entity, getAnimation(type));
    m_rendererManager.addEntityToLayer(entity, layer);
    m_ECS.addComponent<CState>(entity, PlayerState::STAND);
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid);
    m_ECS.addComponent<CVelocity>(entity, m_goblinConfig.SPEED);
    CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER | FRIENDLY_LAYER | PLAYER_LAYER | PROJECTILE_LAYER;
    m_ECS.addComponent<CCollisionBox>(entity, Vec2{8, 12}, ENEMY_LAYER, collisionMask);
    m_ECS.addComponent<CPathfind>(entity, m_ECS.getComponent<CTransform>(m_player).pos);

    m_ECS.addComponent<CHealth>(entity, 4, 4, 30);
    m_ECS.getComponent<CHealth>(entity).HPType = {"Grass", "Organic"};
    m_ECS.addComponent<CAttack>(entity, 1, 120, 30, 3*16, Vec2{16,16});

    spawnShadow(entity, Vec2{0, 4}, 1, layer-1);

    auto& sc= m_ECS.addComponent<CScript>(entity);
    InitiateScript<RooterController>(sc, entity);
    return entity;
}

std::vector<EntityID> Scene_Play::spawnDualTiles(const Vec2 pos, std::unordered_map<std::string, int> tileTextureMap)
{   
    std::vector<EntityID> entityIDs;
    for (const auto& [tileKey, textureIndex] : tileTextureMap) {
        std::string tile = tileKey;
        uint8_t layer = 3;
        
        if (tile == "water") {
            layer = layer + 1;
        } else if (tile == "dirt") {
            layer = layer + 1;
        } else if (tile == "cloud") {
            layer = layer + 1;
        } else if (tile == "obstacle") {
            layer = layer + 1;
            tile = "mountain";// Change the tile name for "obstacle"
        }
        EntityID entity = m_ECS.addEntity();
        entityIDs.push_back(entity);
        m_ECS.addComponent<CAnimation>(entity, getAnimation(tile + "_dual_sheet"));
        Vec2 tilePosition = Vec2{   (float)(textureIndex % 4), 
                                    (float)(int)(textureIndex / 4)};
        m_ECS.getComponent<CAnimation>(entity).animation.setTile(tilePosition); 
        m_rendererManager.addEntityToLayer(entity, layer);
        Vec2 midGrid = gridToMidPixel(pos, entity);
        m_ECS.addComponent<CTransform>(entity, midGrid);
    }
    return entityIDs;
}

void Scene_Play::changePlayerStateTo(EntityID entity, PlayerState s) {
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

template<typename T>
void Scene_Play::InitiateScript(CScript& sc, EntityID entityID){
    sc.Bind<T>();
    sc.Instance = sc.InstantiateScript();
    sc.Instance->m_entity = {entityID, &m_ECS};
    sc.Instance->m_ECS = &m_ECS;
    sc.Instance->m_physics = &m_physics;
    sc.Instance->m_game = m_game;
    sc.Instance->m_scene = this;
    sc.Instance->OnCreateFunction();
}

void Scene_Play::InitiateProjectileScript(CScript& sc, EntityID entityID){
    sc.Bind<ProjectileController>();
    sc.Instance = sc.InstantiateScript();
    sc.Instance->m_entity = {entityID, &m_ECS};
    sc.Instance->m_ECS = &m_ECS;
    sc.Instance->m_physics = &m_physics;
    sc.Instance->m_game = m_game;
    sc.Instance->m_scene = this;
    sc.Instance->OnCreateFunction();
}

void Scene_Play::onEnd() {
    m_game->changeScene("MAIN_MENU", std::make_shared<Scene_Menu>(m_game));
}

void Scene_Play::onFinish() {
    std::cout << "Warning, removing scene_play instance" << std::endl;
    m_game->changeScene("Finish", std::make_shared<Scene_Finish>(m_game), true);
}

void Scene_Play::setPaused(bool pause) {
    m_pause = pause;
}

void Scene_Play::togglePause() {
    m_pause = !m_pause;
}

Vec2 Scene_Play::gridSize(){
    return m_gridSize;
}

Vec2 Scene_Play::getCameraPosition() {
    return m_camera.position;
}
