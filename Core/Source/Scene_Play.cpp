#include "Scene_Menu.h"
#include "Scene_Play.h"
#include "Scene_Inventory.h"
#include "Sprite.h"
#include "Assets.h"
#include "Game.h"
#include "Components.h"
#include "Action.h"
#include "Level_Loader.h"
#include "Camera.h"
#include "ScriptableEntity.h"
#include "scripts/player.cpp"
#include "scripts/rooter.cpp"

#include "RandomArray.h"

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

Scene_Play::Scene_Play(Game* game, std::string levelPath, bool newGame)
    : Scene(game), m_levelPath(levelPath), m_newGame(newGame)
{
    init(m_levelPath);
    m_camera.calibrate(Vec2 {(float)width(), (float)height()}, m_levelSize, m_gridSize);
    m_inventory_scene =  std::make_shared<Scene_Inventory>(m_game);
}

void Scene_Play::init(const std::string& levelPath) {
    registerAction(SDLK_w, "UP");
    registerAction(SDLK_UP, "UP");
    registerAction(SDLK_s, "DOWN");
    registerAction(SDLK_DOWN, "DOWN");
    registerAction(SDLK_a, "LEFT");
    registerAction(SDLK_LEFT, "LEFT");
    registerAction(SDLK_d, "RIGHT");
    registerAction(SDLK_RIGHT, "RIGHT");
    
    registerAction(SDLK_i, "INVENTORY");
    registerAction(SDLK_e, "USE");
    registerAction(SDL_BUTTON_LEFT , "ATTACK");
    registerAction(SDL_MOUSEWHEEL , "ATTACK");
    registerAction(SDL_MOUSEWHEEL_NORMAL , "SCROLL");
    registerAction(SDLK_SPACE , "ATTACK");
    registerAction(SDLK_LSHIFT, "SHIFT");
    registerAction(SDLK_LCTRL, "CTRL");
    registerAction(SDLK_ESCAPE, "QUIT");
    registerAction(SDLK_q, "SETTINGS");
    registerAction(SDLK_u, "SAVE");

    registerAction(SDLK_f, "CAMERA FOLLOW");
    registerAction(SDLK_z, "CAMERA PAN");
    registerAction(SDLK_PLUS, "ZOOM IN");
    registerAction(SDLK_MINUS, "ZOOM OUT");
    registerAction(SDLK_r, "RESET");
    registerAction(SDLK_p, "PAUSE");
    registerAction(SDLK_t, "TOGGLE_TEXTURE");
    registerAction(SDLK_c, "TOGGLE_COLLISION");
    registerAction(SDLK_1, "TP1");
    registerAction(SDLK_2, "TP2");
    registerAction(SDLK_3, "TP3");

    loadConfig("config_files/config.txt");
    loadLevel(levelPath); 
    spawnPlayer();
    loadMobsNItems("config_files/mobs.txt"); // mobs have to spawn after player, so they can target the player
    spawnWeapon(Vec2{364, 90}*m_gridSize, 7);
}

void Scene_Play::loadMobsNItems(const std::string& path){
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Could not load mobs.txt file!\n";
        exit(-1);
    }
    std::string head;
    Vec2 pos;
    int layer;
    while (file >> head) {
        file >>  pos.x >> pos.y >> layer;           
        if (head == "rooter") {
            spawnSmallEnemy(pos*m_gridSize, layer, "rooter");
        }
        else if (head == "goblin") {
            spawnSmallEnemy(pos*m_gridSize, layer, "goblin");
        }
        else if (head == "coin") {
            spawnCoin(pos*m_gridSize, layer);
        }
        else if (head == "tree") {
            spawnDecoration(pos*m_gridSize, Vec2 {24, 32}, layer, "tree");
        }
        else if (head == "House1") {
            spawnDecoration(pos*m_gridSize, Vec2 {56, 44}*4, layer, "House1");
        }
        else if (head == "campfire") {
            spawnCampfire(pos*m_gridSize, layer);
        }
        else {
            std::cerr << "The mobs file format is incorrect!\n";
            exit(-1);
        }
    }
}

void Scene_Play::loadConfig(const std::string& confPath){
    std::ifstream file(confPath);
    if (!file) {
        std::cerr << "Could not load config.txt file!\n";
        exit(-1);
    }
    std::string head;
    while (file >> head) {
        if (head == "Player") {
            file >> m_playerConfig.x >> m_playerConfig.y >> m_playerConfig.SPEED >> m_playerConfig.MAXSPEED >> m_playerConfig.HP >> m_playerConfig.DAMAGE;           
        }
        else if (head == "Rooter") {
            file >> m_rooterConfig.SPEED >> m_rooterConfig.ATTACK_SPEED >> m_rooterConfig.HP >> m_rooterConfig.DAMAGE;           
        }
        else if (head == "Goblin") {
            file >> m_goblinConfig.SPEED >> m_goblinConfig.ATTACK_SPEED >> m_goblinConfig.HP >> m_goblinConfig.DAMAGE;           
        }
        else {
            std::cerr << "head to " << head << "\n";
            std::cerr << "The config file format is incorrect!\n";
            exit(-1);
        }
    }
}

// Function to save the game state to a file
void Scene_Play::saveGame(const std::string& filename) {
    std::ofstream saveFile(filename);

    if (saveFile.is_open()) {
        saveFile << "Player_pos " << (int)(m_ECS.getComponent<CTransform>(m_player).pos.x/m_gridSize.x) << " " << (int)(m_ECS.getComponent<CTransform>(m_player).pos.y/m_gridSize.y) << std::endl;
        saveFile << "Player_hp " << m_ECS.getComponent<CHealth>(m_player).HP << std::endl;
        saveFile.close();
    } else {
        std::cerr << "Unable to open file for saving!" << std::endl;
    }
}

void Scene_Play::loadLevel(const std::string& levelPath){

    const char* path = levelPath.c_str();
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (loadedSurface == nullptr) {
        std::cerr << "Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
    }

    // Lock the surface to access the pixels
    SDL_LockSurface(loadedSurface);
    Uint32* pixels = (Uint32*)loadedSurface->pixels;

    const int HEIGHT_PIX = loadedSurface->h;
    const int WIDTH_PIX = loadedSurface->w;
    m_levelSize = Vec2{ (float)WIDTH_PIX, (float)HEIGHT_PIX };
    m_pixelMatrix = m_levelLoader.createPixelMatrix(pixels, loadedSurface->format, WIDTH_PIX, HEIGHT_PIX);
    // Unlock and free the surface
    SDL_UnlockSurface(loadedSurface);
    SDL_FreeSurface(loadedSurface);
    m_levelLoader.init(this, WIDTH_PIX, HEIGHT_PIX);
    m_levelLoader.loadChunk(m_currentChunk);

}

void Scene_Play::sDoAction(const Action& action) {
    if ( action.type() == "START") {
        if ( action.name() == "TOGGLE_TEXTURE") {
            m_drawTextures = !m_drawTextures; 
        } else if ( action.name() == "TOGGLE_COLLISION") { 
            m_drawCollision = !m_drawCollision; 
        } else if ( action.name() == "TOGGLE_GRID") { 
            m_drawDrawGrid = !m_drawDrawGrid; 
        } else if ( action.name() == "PAUSE") { 
            setPaused( !m_pause || m_inventoryOpen );
        } else if ( action.name() == "QUIT") {
            if (m_pause_scene != nullptr) {
                m_pause_scene = nullptr;
            } else {
                onEnd();
            }
        } else if ( action.name() == "SETTINGS") { 
            togglePause();
            if (m_pause_scene != nullptr) {
                m_pause_scene = nullptr;
            } else {
                m_pause_scene = std::make_shared<Scene_Pause>(m_game);
            }
        } else if ( action.name() == "INVENTORY") { 
            m_inventoryOpen = !m_inventoryOpen;
            m_inventory_scene->toggleInventory();
            setPaused( !m_pause || m_inventoryOpen );
        } else if ( action.name() == "SCROLL"){
            if ( m_inventoryOpen )
            {
                m_inventory_scene->Scroll(m_mouseState.scroll);
            }
            else
            {
                if (m_mouseState.scroll > 0)
                {
                    cameraZoom += 0.25;
                }
                if (m_mouseState.scroll < 0)
                {
                    cameraZoom -= 0.25;
                }
                cameraZoom = std::max( 0.25f, std::min(4.0f, cameraZoom) );

            }
        } else if ( action.name() == "ZOOM IN"){
            m_chunkSize += Vec2{4,4};
            m_levelLoader.clearChunks(0);
        } else if ( action.name() == "ZOOM OUT"){
            m_chunkSize -= Vec2{4,4};
            m_levelLoader.clearChunks(0);
        } else if ( action.name() == "CAMERA FOLLOW"){
            m_camera.toggleCameraFollow();
        } else if ( action.name() == "CAMERA PAN"){
            m_pause = m_camera.startPan(2048, 1000, Vec2 {(float)(64*52+32 - width()/2), (float)(64*44+32 - height()/2)}, m_pause);
        } else if ( action.name() == "SAVE"){
            saveGame("config_files/game_save.txt");
        } else if ( action.name() == "TP1") { 
            m_ECS.getComponent<CTransform>(m_player).pos = Vec2{460*64, 460*64};
        } else if ( action.name() == "TP2") { 
            m_ECS.getComponent<CTransform>(m_player).pos = Vec2{292*64, 236*64};
        }else if ( action.name() == "TP3") {
            m_ECS.getComponent<CTransform>(m_player).pos = Vec2{801*64, 181*64};
        } else if ( action.name() == "RESET") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/levelStartingArea.png", true));
        }

        if ( action.name() == "UP") { m_ECS.getComponent<CInputs>(m_player).up = true; }
        if ( action.name() == "DOWN") { m_ECS.getComponent<CInputs>(m_player).down = true;  }
        if ( action.name() == "LEFT") { m_ECS.getComponent<CInputs>(m_player).left = true; }
        if ( action.name() == "RIGHT") { m_ECS.getComponent<CInputs>(m_player).right = true; }
        if ( action.name() == "SHIFT") { m_ECS.getComponent<CInputs>(m_player).shift = true; }
        if ( action.name() == "CTRL") { m_ECS.getComponent<CInputs>(m_player).ctrl = true; }

        if ( action.name() == "ATTACK" && m_ECS.hasComponent<CWeaponChild>(m_player)){
            EntityID weaponID = m_ECS.getComponent<CWeaponChild>(m_player).weaponID;
            spawnProjectile(weaponID, getMousePosition()-m_ECS.getComponent<CTransform>(weaponID).pos+m_camera.position, 8);
        }
    }
    else if ( action.type() == "END") {
        if ( action.name() == "DOWN") { m_ECS.getComponent<CInputs>(m_player).down = false; }
        if ( action.name() == "UP") { m_ECS.getComponent<CInputs>(m_player).up = false; }
        if ( action.name() == "LEFT") { m_ECS.getComponent<CInputs>(m_player).left = false; }
        if ( action.name() == "RIGHT") { m_ECS.getComponent<CInputs>(m_player).right = false; }
        if ( action.name() == "SHIFT") { m_ECS.getComponent<CInputs>(m_player).shift = false; }
        if ( action.name() == "CTRL") { m_ECS.getComponent<CInputs>(m_player).ctrl = false; }

        if ( action.name() == "ATTACK" && m_ECS.hasComponent<CWeaponChild>(m_player)) {
            EntityID weaponID = m_ECS.getComponent<CWeaponChild>(m_player).weaponID;
            if ( m_ECS.hasComponent<CProjectile>(weaponID) )
            {
                EntityID projectileID = m_ECS.getComponent<CProjectile>(weaponID).projectileID;
                m_ECS.queueRemoveComponent<CProjectile>(weaponID);
                m_ECS.queueRemoveComponent<CParent>(projectileID);
                if ( m_ECS.getComponent<CProjectileState>(projectileID).state == "Ready" )
                {
                    m_ECS.addComponent<CBoundingBox>(projectileID, Vec2{12, 12});
                    m_ECS.getComponent<CTransform>(projectileID).isMovable = true;
                    m_ECS.getComponent<CProjectileState>(projectileID).state = "Free";
                    m_ECS.getComponent<CTransform>(projectileID).vel = getMousePosition()-m_ECS.getComponent<CTransform>(weaponID).pos+m_camera.position;
                    m_ECS.getComponent<CTransform>(projectileID).angle = m_ECS.getComponent<CTransform>(projectileID).vel.angle();
                } else {
                    m_ECS.queueRemoveEntity(projectileID);
                }
            }
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
        sAnimation();
        sAudio();
        m_currentFrame++;
    }
    sRender();
    m_ECS.update();
    m_inventory_scene->update();
    if (m_pause_scene != nullptr) {
        m_pause_scene->update();
    }
}

void Scene_Play::sLoader()
{
    m_currentChunk = ( ( (m_ECS.getComponent<CTransform>(m_player).pos / m_gridSize).toInt() ) / m_chunkSize ).toInt();
    for (int dx = -3; dx <= 3; ++dx) 
    {
        for (int dy = -2; dy <= 2; ++dy) 
        {
            Vec2 neighborChunk = {m_currentChunk.x + dx, m_currentChunk.y + dy};
            if ( neighborChunk.smaller(Vec2{0,0}) || neighborChunk.greater(m_levelSize/m_chunkSize) )
            {
                continue;
            }
            if (std::find(m_loadedChunks.begin(), m_loadedChunks.end(), neighborChunk) == m_loadedChunks.end())
            {
                EntityID chunkID = m_levelLoader.loadChunk(neighborChunk);
                m_loadedChunkIDs.push_back(chunkID);
                m_loadedChunks.push_back(neighborChunk);
            }
        }
    }

    m_levelLoader.clearChunks(35); // Will leave 12 chunks
}

void Scene_Play::sScripting() 
{
    auto view = m_ECS.signatureView<CScript>();
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

    auto& pathfindPool = m_ECS.getComponentPool<CPathfind>();
    auto viewPathfind = m_ECS.signatureView<CPathfind, CTransform>();
    for (auto e : viewPathfind)
    {
        auto& transform = transformPool.getComponent(e);
        auto& pathfind = pathfindPool.getComponent(e);
        if ((pathfind.target - transform.pos).length() < 64*2) {
            transform.vel = pathfind.target - transform.pos;
        } else {
            transform.vel = Vec2 {0,0};
        }
        pathfind.target = transformPool.getComponent(m_player).pos;
    }

    auto& inputPool = m_ECS.getComponentPool<CInputs>();
    auto& viewInputs = m_ECS.view<CInputs>();
    for (auto e : viewInputs){
        auto &transform = transformPool.getComponent(e);
        auto &inputs = inputPool.getComponent(e);

        if ( e == m_player ){
            transform.vel = { 0,0 };
            if (inputs.up){
                transform.vel.y--;
            } if (inputs.down){
                transform.vel.y++;
            } if (inputs.left){
                transform.vel.x--;
            } if (inputs.right){
                transform.vel.x++;
            } if (inputs.shift){
                transform.tempo = 0.5f;
            } else if (inputs.ctrl){
                transform.tempo = 3.0f;
            } else{
                transform.tempo = 1.0f;
            }
        }
    }

    auto viewKnockback = m_ECS.signatureView<CKnockback, CTransform>();
    auto& knockbackPool = m_ECS.getComponentPool<CKnockback>();
    for (auto entityKnockback : viewKnockback){    
        auto &transform = transformPool.getComponent(entityKnockback);
        auto& knockback = knockbackPool.getComponent(entityKnockback);
        transform.pos += m_physics.knockback(knockback);
        if (knockback.duration <= 0){
            m_ECS.queueRemoveComponent<CKnockback>(entityKnockback);
        }
    }       

    auto& viewTransform = m_ECS.view<CTransform>();
    for (auto e : viewTransform){    
        auto &transform = transformPool.getComponent(e);
        
        // Update position
        transform.prevPos = transform.pos;
        if (!(transform.vel.isnull()) && transform.isMovable ){
            transform.pos += transform.vel.norm(transform.tempo*transform.speed/m_game->framerate());
        }
        
    }

    auto& viewParent = m_ECS.view<CParent>();
    auto& parentPool = m_ECS.getOrCreateComponentPool<CParent>();
    for (auto e : viewParent)
    {
        auto& transform = transformPool.getComponent(e);
        auto& parent = parentPool.getComponent(e);
        transform.pos = transformPool.getComponent(parent.parent).pos + parent.relativePos;
    }

}

void Scene_Play::sCollision() {
// ------------------------------- Player collisions -------------------------------------------------------------------------
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& BboxPool = m_ECS.getComponentPool<CBoundingBox>();
    auto& transformPlayer = transformPool.getComponent(m_player);
    auto& BboxPlayer = BboxPool.getComponent(m_player);

    auto& viewLoot = m_ECS.getComponentPool<CLoot>();
    for ( auto e : viewLoot ){
        auto& transform = transformPool.getComponent(e);
        auto& Bbox = BboxPool.getComponent(e);
        if ( m_physics.isCollided(transformPlayer, BboxPlayer, transform, Bbox) )
        {
            m_ECS.queueRemoveEntity(e);
            Mix_PlayChannel(-1, m_game->assets().getAudio("loot_pickup"), 0);
        }
    }

    auto& viewWeapon = m_ECS.getComponentPool<CWeapon>();
    for ( auto e : viewWeapon ){
        auto& transform = transformPool.getComponent(e);
        auto& Bbox = BboxPool.getComponent(e);
        if ( m_physics.isCollided(transformPlayer, BboxPlayer, transform, Bbox) )
        {
            m_ECS.queueRemoveComponent<CBoundingBox>(e);
            m_ECS.queueRemoveComponent<CWeapon>(e);
            m_ECS.addComponent<CWeaponChild>(m_player, e);
            m_ECS.addComponent<CParent>(e, m_player, Vec2{32, -16});
            Mix_PlayChannel(-1, m_game->assets().getAudio("loot_pickup"), 0);

        }
    }

    auto &viewImmovable = m_ECS.view<CImmovable>();
    for ( auto e : viewImmovable ){
        auto& transform = transformPool.getComponent(e);
        auto& Bbox = BboxPool.getComponent(e);
        if (m_physics.isCollided(transformPlayer, BboxPlayer, transform, Bbox))
        {
            Vec2 overlap = m_physics.overlap(transformPlayer, BboxPlayer, transform, Bbox);
            if ( m_ECS.hasComponent<CChild>(m_player) )
            {
                EntityID childID = m_ECS.getComponent<CChild>(m_player).childID;
                m_ECS.getComponent<CTransform>(childID).pos += overlap;
            }      
            m_ECS.getComponent<CTransform>(m_player).pos += overlap;
        }
    }

// ------------------------------- Enemy collisions -------------------------------------------------------------------------

    auto& viewP = m_ECS.view<CPathfind>();
    for ( auto enemy : viewP )
    {
        auto& transformEnemy = transformPool.getComponent(enemy);
        auto& BboxEnemy = BboxPool.getComponent(enemy);
        for (auto enemy2 : viewP)
        {
            auto& transformEnemy2 = transformPool.getComponent(enemy2);
            auto& BboxEnemy2 = BboxPool.getComponent(enemy2);
            if (m_physics.isCollided(transformEnemy, BboxEnemy, transformEnemy2, BboxEnemy2))
            {
                transformPool.getComponent(enemy).pos += m_physics.overlap(transformEnemy, BboxEnemy, transformEnemy2, BboxEnemy2);
            }
        }
        if (m_physics.isCollided(transformEnemy, BboxEnemy, transformPlayer, BboxPlayer))
            {
                transformPool.getComponent(enemy).pos += m_physics.overlap(transformEnemy, BboxEnemy, transformPlayer, BboxPlayer);
                // m_ECS.addComponent<CKnockback>(m_player, 120, 10, transformEnemy.vel);
            }
        auto &viewImmovable = m_ECS.view<CImmovable>();
        for ( auto e : viewImmovable ){
            auto& transform = transformPool.getComponent(e);
            auto& Bbox = BboxPool.getComponent(e);
            if (m_physics.isCollided(transformEnemy, BboxEnemy, transform, Bbox))
            {
                m_ECS.getComponent<CTransform>(enemy).pos += m_physics.overlap(transformEnemy, BboxEnemy, transform, Bbox);
            }
        }
    }

// ------------------------------- Projectile collisions ---------------------------------------------------------------------
    auto& healthPool = m_ECS.getComponentPool<CHealth>();
    std::vector<EntityID> viewSignatureBbox             = m_ECS.signatureView<CBoundingBox, CHealth>();
    std::vector<EntityID> viewSignatureImmovable        = m_ECS.signatureView<CBoundingBox, CImmovable>();
    std::vector<EntityID> viewSignatureProjectileState  = m_ECS.signatureView<CBoundingBox, CProjectileState>();
    for ( auto projectileID : viewSignatureProjectileState )
    {
        auto& transformProjectile = transformPool.getComponent(projectileID);
        auto& BboxProjectile = BboxPool.getComponent(projectileID);
        for ( auto enemyID : viewSignatureBbox )
        {   
            if (enemyID != m_player ) 
            {
                auto& transformEnemy = transformPool.getComponent(enemyID);
                auto& BboxEnemy = BboxPool.getComponent(enemyID);
                if (m_physics.isCollided(transformProjectile, BboxProjectile, transformEnemy, BboxEnemy))
                {
                    auto& animation     = m_ECS.getComponent<CAnimation>(projectileID);
                    animation.animation = m_game->assets().getAnimation("fireball_explode");
                    animation.repeat    = false;
                    transformProjectile.isMovable = false;
                    m_ECS.queueRemoveComponent<CBoundingBox>(projectileID);
                    m_ECS.queueRemoveComponent<CDamage>(projectileID);
                    // m_ECS.addComponent<CParent>(projectileID, enemyID, Vec2{32, 0}); // Need to remove child if parent dies
                    auto& health = healthPool.getComponent(enemyID);
                    health.HP--;
                    health.damage_frame = m_currentFrame;
                }
            }
        }
        for ( auto enemyID : viewSignatureImmovable )
        {   
            auto& transformObstacle = transformPool.getComponent(enemyID);
            auto& BboxObstacle = BboxPool.getComponent(enemyID);
            if (m_physics.isCollided(transformProjectile, BboxProjectile, transformObstacle, BboxObstacle))
            {
                auto& animation     = m_ECS.getComponent<CAnimation>(projectileID);
                animation.animation = m_game->assets().getAnimation("fireball_explode");
                animation.repeat    = false;
                transformProjectile.isMovable = false;
                m_ECS.queueRemoveComponent<CBoundingBox>(projectileID);
                m_ECS.queueRemoveComponent<CDamage>(projectileID);
                // m_ECS.addComponent<CParent>(projectileID, enemyID, Vec2{32, 0}); // Need to remove child if parent dies
            }
        }
    }
}

void Scene_Play::sStatus() {
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
  
    auto viewLifespan = m_ECS.signatureView<CLifespan>();
    auto& lifespanPool = m_ECS.getComponentPool<CLifespan>();
    for ( auto entityID : viewLifespan)
    {   
        auto& lifespan = lifespanPool.getComponent(entityID).lifespan;
        lifespan--;
        if ( lifespan <= 0)
        {
            m_ECS.queueRemoveEntity(entityID);
        }
    }

    auto viewHealth = m_ECS.signatureView<CHealth>();
    auto& healthPool = m_ECS.getComponentPool<CHealth>();
    for ( auto entityID : viewHealth)
    {
        auto& health = healthPool.getComponent(entityID);
        auto& transform = transformPool.getComponent(entityID);
        if (health.HP <= 0)
        {
            spawnCoin(transform.pos, 6);
            if ( m_player == entityID ){
                    m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/levelStartingArea.png", true));
            } else {
                m_ECS.queueRemoveEntity(entityID);
                Mix_PlayChannel(-1, m_game->assets().getAudio("enemy_death"), 0);
            }
        }
    }
    auto viewDamage = m_ECS.signatureView<CDamage, CBoundingBox, CTransform>();
    auto& BboxPool = m_ECS.getComponentPool<CBoundingBox>();
    auto& damagePool = m_ECS.getComponentPool<CDamage>();
    // for ( auto& [transformDamage, bboxDamage, damage] : viewDamage)
    for ( auto entityDamage : viewDamage)
    {
        auto& transformDamage = transformPool.getComponent(entityDamage);
        auto& bboxDamage = BboxPool.getComponent(entityDamage);
        auto& damage = damagePool.getComponent(entityDamage);
        Signature damageSignature = m_ECS.getSignature(entityDamage);
        // for ( auto& [transformHealth, bboxHealth, health] : viewHealth)
        for ( auto entityHealth : viewHealth )
        {
            if ( entityDamage == entityHealth ){continue;}
            if ( (entityHealth == m_player) && ((damageSignature & CProjectileStateMask) == CProjectileStateMask) ){continue;}
            
            auto& transforHealth = transformPool.getComponent(entityHealth);
            auto& bboxHealth = BboxPool.getComponent(entityHealth);
            auto& health = healthPool.getComponent(entityHealth);
            if ( m_physics.isCollided(transformDamage, bboxDamage, transforHealth, bboxHealth) )
            {
                if ( (int)(m_currentFrame-health.damage_frame) > health.i_frames ) {
                    int damageMultiplier = 1;
                    m_ECS.addComponent<CKnockback>(entityHealth, 50, 64, transformDamage.pos-transforHealth.pos);
                    health.HP = health.HP-(int)(damage.damage*damageMultiplier);
                    health.damage_frame = m_currentFrame;
                }
            }
        }
    }
}

void Scene_Play::sAnimation() {
    auto view = m_ECS.signatureView<CState, CAnimation, CTransform>();
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& statePool = m_ECS.getComponentPool<CState>();
    auto& animationPool = m_ECS.getComponentPool<CAnimation>();
    for ( auto e : view ){
        auto& transform = transformPool.getComponent(e);
        auto& state = statePool.getComponent(e);
        auto& animation = animationPool.getComponent(e).animation;
        if( transform.vel.isnull() ) {
            changePlayerStateTo(e, PlayerState::STAND);
        } else if( transform.vel.mainDir().x > 0 ) {
            changePlayerStateTo(e, PlayerState::RUN_RIGHT);
        } else if(transform.vel.mainDir().x < 0) {
            changePlayerStateTo(e, PlayerState::RUN_LEFT);
        } else if(transform.vel.mainDir().y > 0) {
            changePlayerStateTo(e, PlayerState::RUN_DOWN);
        } else if(transform.vel.mainDir().y < 0) {
            changePlayerStateTo(e, PlayerState::RUN_UP);
        }
        // // change player animation
        if (state.changeAnimate) {
            animation.setRow((int)state.state);
        }
    }

    auto viewProjectileState = m_ECS.signatureView<CProjectileState, CAnimation>();
    auto& projectileStatePool = m_ECS.getComponentPool<CProjectileState>();
    for ( auto e : viewProjectileState ) {
        auto& animation = animationPool.getComponent(e);
        auto& projectileState = projectileStatePool.getComponent(e);
        if ( projectileState.state == "Create" ) {
            if ( animation.animation.hasEnded() ) {
                projectileState.state = "Ready";
                animation.animation = m_game->assets().getAnimation("fireball");
                animation.repeat = true;
                m_camera.startShake(4, 50);
            }
        }
    }

    auto& view2 = m_ECS.view<CAnimation>();
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
    SDL_RenderClear(m_game->renderer());
    Vec2 screenCenter = Vec2{(float)width(), (float)height()}/2;
    
    if (m_drawTextures)
    {
        auto& transformPool = m_ECS.getComponentPool<CTransform>();
        auto& animationPool = m_ECS.getComponentPool<CAnimation>();
        
        auto layers = m_rendererManager.getLayers();
        for (const auto& layer : layers)
        {
            for (const auto& e : layer)
            {                
                if (!transformPool.hasComponent(e) || !animationPool.hasComponent(e)) {
                    continue;
                }
                auto& transform = transformPool.getComponent(e);
                auto& animation = animationPool.getComponent(e).animation;

                // Adjust the entity's position based on the camera position
                Vec2 adjustedPosition = transform.pos - m_camera.position;
                auto distanceToCenter = adjustedPosition - screenCenter;
                adjustedPosition += distanceToCenter * (cameraZoom - 1);

                animation.setScale(transform.scale * cameraZoom);
                animation.setAngle(transform.angle);
                animation.setDestRect(adjustedPosition - animation.getDestSize() / 2);
                spriteRender(animation);
            }
        }

        auto& viewHealth = m_ECS.getComponentPool<CHealth>();
        for (auto entityID : viewHealth)
        {   
            if (entityID == m_player) { continue; }
            auto& health = viewHealth.getComponent(entityID);
            if ((int)(m_currentFrame - health.damage_frame) < health.i_frames)
            {
                auto& transform = transformPool.getComponent(entityID);
                Vec2 adjustedPosition = transform.pos - m_camera.position;
                auto distanceToCenter = adjustedPosition - screenCenter;
                adjustedPosition += distanceToCenter * (cameraZoom - 1);

                Animation animation;
                auto hearts = float(health.HP) / 2;

                for (int i = 1; i <= health.HP_max / 2; i++)
                {   
                    if (hearts >= i)
                    {
                        animation = health.animation_full;
                    }
                    else if (i - hearts == 0.5f)
                    {
                        animation = health.animation_half;
                    }
                    else
                    {
                        animation = health.animation_empty;
                    }

                    animation.setScale(transform.scale * cameraZoom);
                    animation.setDestRect(Vec2{
                        adjustedPosition.x + (float)(i - 1 - (float)health.HP_max / 4) * animation.getSize().x * animation.getScale().x, 
                        adjustedPosition.y - m_ECS.getComponent<CAnimation>(entityID).animation.getSize().y * m_ECS.getComponent<CAnimation>(entityID).animation.getScale().y / 2
                    });
                    spriteRender(animation);
                }
            }
        }
        Animation animation;
        auto hearts = float(m_ECS.getComponent<CHealth>(m_player).HP) / 2;

        for (int i = 1; i <= m_ECS.getComponent<CHealth>(m_player).HP_max / 2; i++)
        {   
            if (hearts >= i)
            {
                animation = m_ECS.getComponent<CHealth>(m_player).animation_full;
            }
            else if (i - hearts == 0.5f)
            {
                animation = m_ECS.getComponent<CHealth>(m_player).animation_half;
            }
            else
            {
                animation = m_ECS.getComponent<CHealth>(m_player).animation_empty;
            }
            animation.setScale(Vec2{4, 4});
            animation.setDestRect(Vec2{(float)(i - 1) * animation.getSize().x * animation.getScale().x, 0});
            spriteRender(animation);
        }
    }
    
    if (m_drawCollision)
    {
        auto& view = m_ECS.view<CBoundingBox>();
        auto& transformPool = m_ECS.getComponentPool<CTransform>();
        auto& BboxPool = m_ECS.getComponentPool<CBoundingBox>();
        for (auto e : view)
        {      
            auto& transform = transformPool.getComponent(e);
            auto& box = BboxPool.getComponent(e);

            // Adjust the collision box position based on the camera position
            SDL_Rect collisionRect;
            collisionRect.x = (int)(transform.pos.x - box.halfSize.x - m_camera.position.x);
            collisionRect.y = (int)(transform.pos.y - box.halfSize.y - m_camera.position.y);
            collisionRect.w = (int)(box.size.x);
            collisionRect.h = (int)(box.size.y);

            SDL_SetRenderDrawColor(m_game->renderer(), box.red, box.green, box.blue, 255);
            SDL_RenderDrawRect(m_game->renderer(), &collisionRect);
        }
    }
}

void Scene_Play::spriteRender(Animation &animation){
    SDL_RenderCopyEx(
        m_game->renderer(), 
        animation.getTexture(), 
        animation.getSrcRect(), 
        animation.getDestRect(),
        animation.getAngle(),
        NULL,
        SDL_FLIP_NONE
    );
}

void Scene_Play::sAudio(){
    if( Mix_PlayingMusic() == 0 )
    {
        Mix_PlayMusic(m_game->assets().getMusic("AbbeGameTrack1"), -1);
    }
}

EntityID Scene_Play::spawnPlayer(){
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
            }
            if (head == "Player_hp") {
                file >> hp;
            }        
            else {
                std::cerr << "The game save file format is incorrect!\n";
                exit(-1);
            }
        }
    }
    Vec2 pos = Vec2{64*(float)pos_x, 64*(float)pos_y};
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entityID);

    m_ECS.addComponent<CTransform>(entityID, midGrid, Vec2{0,0}, Vec2{4, 4}, 0.0f, m_playerConfig.SPEED, true);
    m_ECS.addComponent<CBoundingBox>(entityID, Vec2 {32, 32});
    m_ECS.addComponent<CName>(entityID, "wiz");
    m_ECS.addComponent<CAnimation>(entityID, m_game->assets().getAnimation("wiz"), true, layer);
    m_rendererManager.addEntityToLayer(entityID, layer);
    spawnShadow(entityID, Vec2{0,0}, 1, layer-1);
    m_ECS.addComponent<CInputs>(entityID);
    m_ECS.addComponent<CState>(entityID, PlayerState::STAND);
    m_ECS.addComponent<CHealth>(entityID, hp, m_playerConfig.HP, 60, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));

    m_ECS.addComponent<CScript>(entityID).Bind<PlayerController>();
    auto& scriptPool = m_ECS.getComponentPool<CScript>();
    auto& sc = scriptPool.getComponent(entityID);    
    sc.Instance = sc.InstantiateScript();
    sc.Instance->m_entity = {entityID, &m_ECS};
    sc.Instance->m_ECS = &m_ECS;
    sc.Instance->OnCreateFunction();
    return entityID;
}

EntityID Scene_Play::spawnShadow(EntityID parentID, Vec2 relPos, int size, int layer){
    auto shadowID = m_ECS.addEntity();
    m_ECS.addComponent<CTransform>(shadowID);
    m_ECS.getComponent<CTransform>(shadowID).scale *= size;
    m_ECS.addComponent<CParent>(shadowID, parentID, relPos);
    m_ECS.addComponent<CAnimation>(shadowID, m_game->assets().getAnimation("shadow"), true, layer);
    m_rendererManager.addEntityToLayer(shadowID, layer);
    m_ECS.addComponent<CChild>(parentID, shadowID, true);
    return shadowID;
}

EntityID Scene_Play::spawnWeapon(Vec2 pos, int layer){
    auto entity = m_ECS.addEntity();

    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0,0}, Vec2{4, 4}, 0.0f, 0.0f, true);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2 {24, 24});
    m_ECS.addComponent<CName>(entity, "staff");
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("staff"), true, 2);
    m_rendererManager.addEntityToLayer(entity, 5);
    // m_ECS.addComponent<CDamage>(entity, 1, 180, std::unordered_set<std::string> {"Fire", "Explosive"});
    m_ECS.addComponent<CWeapon>(entity);
    spawnShadow(entity, Vec2{0,0}, 1, layer-1);
    return entity;
}

EntityID Scene_Play::spawnDecoration(Vec2 pos, Vec2 collisionBox, const size_t layer, std::string animation){
    auto entity = m_ECS.addEntity();

    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0,0}, Vec2{4, 4}, 0.0f, 0.0f, true);
    m_ECS.addComponent<CBoundingBox>(entity, collisionBox);
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation(animation), true, layer);
    m_rendererManager.addEntityToLayer(entity, layer);
    m_ECS.addComponent<CImmovable>(entity);
    spawnShadow(entity, Vec2{0,-16}, 3, layer-1);
    return entity;
}

EntityID Scene_Play::spawnObstacle(const Vec2 pos, bool movable, const int frame){
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2 {0.5,0.5}, 0.0f, movable);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2 {64, 64});
    m_ECS.addComponent<CImmovable>(entity);
    return entity;
}

EntityID Scene_Play::spawnDragon(const Vec2 pos, bool movable, const std::string &ani) {
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation(ani), true, 3);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, Vec2 {2, 2}, 0.0f, movable);
    m_ECS.addComponent<CHealth>(entity, (int)10, (int)10, 30, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
    m_ECS.getComponent<CHealth>(entity).HPType = {""};
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{96, 96});
    return entity;
}

EntityID Scene_Play::spawnGrass(const Vec2 pos, const int frame)
{
    auto entity = m_ECS.addEntity();
    // std::vector<int> ranArray = generateRandomArray(1, m_ECS.getNumEntities(), 0, 15);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2{1, 1}, 0.0f, false);
    return entity;
}

EntityID Scene_Play::spawnDirt(const Vec2 pos, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2{1, 1}, 0.0f, false);
    return entity;
}

EntityID Scene_Play::spawnCampfire(const Vec2 pos, int layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity,m_game->assets().getAnimation("campfire"), true, layer);
    m_rendererManager.addEntityToLayer(entity, layer);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0,0}, Vec2{4,4}, 0.0f, 0.0f, false);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{32, 32});
    return entity;
}

EntityID Scene_Play::spawnLava(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, false);
    m_ECS.addComponent<CImmovable>(entity);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{64, 64});
    return entity;
}

EntityID Scene_Play::spawnWater(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, false);
    m_ECS.addComponent<CImmovable>(entity);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{64, 64});
    return entity;
}

EntityID Scene_Play::spawnBridge(const Vec2 pos, const int frame)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("bridge"), true, 3);
    m_ECS.getComponent<CAnimation>(entity).animation.setTile(Vec2{(float)(frame % 4), (float)(int)(frame / 4)});
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, Vec2{2,2}, 0.0f, false);
    return entity;
}

EntityID Scene_Play::spawnProjectile(EntityID creator, Vec2 vel, int layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("fireball_create"), false, layer);
    m_rendererManager.addEntityToLayer(entity, layer);
    m_ECS.addComponent<CTransform>(entity, m_ECS.getComponent<CTransform>(creator).pos, vel, Vec2{2, 2}, vel.angle(), 400.0f, false);
    m_ECS.addComponent<CDamage>(entity, 1); // damage speed 6 = frames between attacking
    m_ECS.getComponent<CDamage>(entity).damageType = {"Fire", "Explosive"};
    m_ECS.addComponent<CProjectileState>(entity, "Create");
    m_ECS.addComponent<CParent>(entity, creator);
    m_ECS.addComponent<CProjectile>(creator, entity);
    spawnShadow(entity, Vec2{0,0}, 1, layer-1);
    return entity;
}

EntityID Scene_Play::spawnCoin(Vec2 pos, const size_t layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("coin"), true, layer);
    m_rendererManager.addEntityToLayer(entity, layer);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0,0}, Vec2{4,4}, 0.0f, false);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{32, 32});
    m_ECS.addComponent<CLoot>(entity);
    spawnShadow(entity, Vec2{0,0}, 1, layer-1);
    return entity;
}

EntityID Scene_Play::spawnSmallEnemy(Vec2 pos, const size_t layer, std::string type)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CName>(entity, type);
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation(type), true, 3);
    m_rendererManager.addEntityToLayer(entity, layer);
    m_ECS.addComponent<CState>(entity, PlayerState::STAND);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0,0}, Vec2{4,4}, 0.0f, 150.0f, true);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{32, 48});
    m_ECS.addComponent<CPathfind>(entity, m_ECS.getComponent<CTransform>(m_player).pos);

    m_ECS.addComponent<CHealth>(entity, 4, 4, 30, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
    m_ECS.getComponent<CHealth>(entity).HPType = {"Grass", "Organic"};
    m_ECS.addComponent<CAttack>(entity, 1, 120, 30, 3*64, Vec2{64,64});

    spawnShadow(entity, Vec2{0, 16}, 1, layer-1);

    m_ECS.addComponent<CScript>(entity).Bind<RooterController>();
    auto& scriptPool = m_ECS.getComponentPool<CScript>();
    auto& sc = scriptPool.getComponent(entity);    
    sc.Instance = sc.InstantiateScript();
    sc.Instance->m_entity = {entity, &m_ECS};
    sc.Instance->m_ECS = &m_ECS;
    sc.Instance->OnCreateFunction();
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
            tile = "mountain";  // Change the tile name for "obstacle"
        }
        EntityID entity = m_ECS.addEntity();
        entityIDs.push_back(entity);
        m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation(tile + "_dual_sheet"), true, 10);
        m_ECS.getComponent<CAnimation>(entity).animation.setTile(Vec2{(float)(textureIndex % 4), (float)(int)(textureIndex / 4)});   
        m_rendererManager.addEntityToLayer(entity, layer);
        Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
        m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0, 0}, Vec2{4, 4}, 0.0f, false);
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

void Scene_Play::onEnd() {
    m_game->changeScene("Menu", std::make_shared<Scene_Menu>(m_game));
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

Vec2 Scene_Play::levelSize(){
    return m_levelSize;
}

Vec2 Scene_Play::gridToMidPixel(float gridX, float gridY, EntityID entity) {
    Vec2 offset;
    Vec2 grid = Vec2{gridX, gridY};
    Vec2 eSize;
    if ( m_ECS.hasComponent<CAnimation>(entity) ){
        eSize = m_ECS.getComponent<CAnimation>(entity).animation.getSize();
    } else {
        eSize = m_gridSize/4;
    }
    
    Vec2 eScale = {4.0f, 4.0f};
    // switch ((int)eSize.y) {
    //     case 270:
    //         eScale.x = 0.15f;
    //         eScale.y = 0.18f;
    //         break;
    //     case 225:
    //         eScale.x = 0.18f;
    //         eScale.y = 0.18f;
    //         break;
    //     case 192:
    //         eScale.x = 1.0f;
    //         eScale.y = 1.0f;
    //         eSize.x = 64.0f;
    //         eSize.y = 64.0f;
    //         break;
    //     case 128:
    //         eScale.x = 2.0f;
    //         eScale.y = 2.0f;
    //         eSize.x = 32.0f;
    //         eSize.y = 32.0f;
    //         break;
    //     case 64:
    //         eScale.x = 1.0f;
    //         eScale.y = 1.0f;
    //         break;
    //     case 32:
    //         eScale.x = 2.0f;
    //         eScale.y = 2.0f;
    //         break;
    //     case 16:
    //         eScale.x = 4.0f;
    //         eScale.y = 4.0f;
    //         break;
    //     case 24:
    //         eScale.x = 2.0f;
    //         eScale.y = 2.0f;
    //         break; 
    //     default:
    //         eScale.x = 1.0f;
    //         eScale.y = 1.0f;
    // }
    offset = (m_gridSize - eSize * eScale) / 2.0;

    return grid + m_gridSize / 2 - offset;
}